// ----------------------------------------------------------------- //
//             The Speech Signal Processing Toolkit (SPTK)           //
//             developed by SPTK Working Group                       //
//             http://sp-tk.sourceforge.net/                         //
// ----------------------------------------------------------------- //
//                                                                   //
//  Copyright (c) 1984-2007  Tokyo Institute of Technology           //
//                           Interdisciplinary Graduate School of    //
//                           Science and Engineering                 //
//                                                                   //
//                1996-2019  Nagoya Institute of Technology          //
//                           Department of Computer Science          //
//                                                                   //
// All rights reserved.                                              //
//                                                                   //
// Redistribution and use in source and binary forms, with or        //
// without modification, are permitted provided that the following   //
// conditions are met:                                               //
//                                                                   //
// - Redistributions of source code must retain the above copyright  //
//   notice, this list of conditions and the following disclaimer.   //
// - Redistributions in binary form must reproduce the above         //
//   copyright notice, this list of conditions and the following     //
//   disclaimer in the documentation and/or other materials provided //
//   with the distribution.                                          //
// - Neither the name of the SPTK working group nor the names of its //
//   contributors may be used to endorse or promote products derived //
//   from this software without specific prior written permission.   //
//                                                                   //
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            //
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       //
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          //
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          //
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS //
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          //
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   //
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     //
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON //
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   //
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    //
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           //
// POSSIBILITY OF SUCH DAMAGE.                                       //
// ----------------------------------------------------------------- //

#include "SPTK/utils/sptk_utils.h"

#include <algorithm>  // std::fill_n, std::max, std::min, std::transform
#include <cctype>     // std::tolower
#include <cerrno>     // errno, ERANGE
#include <cmath>      // std::ceil, std::exp, std::log, std::sqrt, etc.
#include <cstddef>    // std::size_t
#include <cstdint>    // int8_t, int16_t, int32_t, int64_t, etc.
#include <cstdio>     // std::snprintf
#include <cstdlib>    // std::strtod, std::strtol
#include <iomanip>    // std::setw
#include <iostream>   // std::cerr, std::endl, std::left

#include "SPTK/utils/int24_t.h"
#include "SPTK/utils/uint24_t.h"

namespace {

// 34 is a reasonable number near log(1e-15)
static const double kThresholdOfInformationLossInLogSpace(-34.0);

}  // namespace

namespace sptk {

template <typename T>
bool ReadStream(T* data_to_read, std::istream* input_stream) {
  if (NULL == data_to_read || NULL == input_stream || input_stream->eof()) {
    return false;
  }

  const int type_byte(sizeof(*data_to_read));
  input_stream->read(reinterpret_cast<char*>(data_to_read), type_byte);

  return (type_byte == input_stream->gcount()) ? !input_stream->fail() : false;
}

template <>
bool ReadStream(sptk::Matrix* matrix_to_read, std::istream* input_stream) {
  if (NULL == matrix_to_read || 0 == matrix_to_read->GetNumRow() ||
      0 == matrix_to_read->GetNumColumn() || NULL == input_stream ||
      input_stream->eof()) {
    return false;
  }

  const int type_byte(sizeof((*matrix_to_read)[0][0]));

  const int num_read_bytes(type_byte * matrix_to_read->GetNumRow() *
                           matrix_to_read->GetNumColumn());
  input_stream->read(reinterpret_cast<char*>(&((*matrix_to_read)[0][0])),
                     num_read_bytes);

  return (num_read_bytes == input_stream->gcount()) ? !input_stream->fail()
                                                    : false;
}

template <typename T>
bool ReadStream(bool zero_padding, int stream_skip, int read_point,
                int read_size, std::vector<T>* sequence_to_read,
                std::istream* input_stream, int* actual_read_size) {
  if (stream_skip < 0 || read_point < 0 || read_size <= 0 ||
      NULL == sequence_to_read || NULL == input_stream || input_stream->eof()) {
    return false;
  }

  const int type_byte(sizeof((*sequence_to_read)[0]));

  if (0 < stream_skip) {
    input_stream->ignore(type_byte * stream_skip);
    if (input_stream->eof()) return false;
  }

  const int end(read_point + read_size);
  if (sequence_to_read->size() < static_cast<std::size_t>(end)) {
    sequence_to_read->resize(end);
  }

  const int num_read_bytes(type_byte * read_size);
  input_stream->read(
      reinterpret_cast<char*>(&((*sequence_to_read)[0]) + read_point),
      num_read_bytes);

  const int gcount(input_stream->gcount());
  if (NULL != actual_read_size) {
    *actual_read_size = gcount / type_byte;
  }

  if (num_read_bytes == gcount) {
    return !input_stream->fail();
  } else if (zero_padding && 0 < gcount) {
    // Use std::ceil to zero incomplete data
    // as gcount may not be a multiple of sizeof(double).
    const int num_zeros(
        std::ceil(static_cast<double>(num_read_bytes - gcount) / type_byte));
    if (num_zeros < 0) {
      return false;  // Something wrong!
    }

    std::fill_n(sequence_to_read->begin() + end - num_zeros, num_zeros, 0.0);

    return !input_stream->bad();
  }

  return false;
}

template <typename T>
bool WriteStream(T data_to_write, std::ostream* output_stream) {
  if (NULL == output_stream) {
    return false;
  }

  output_stream->write(reinterpret_cast<const char*>(&data_to_write),
                       sizeof(data_to_write));

  return !output_stream->fail();
}

bool WriteStream(const sptk::Matrix& matrix_to_write,
                 std::ostream* output_stream) {
  if (0 == matrix_to_write.GetNumRow() || 0 == matrix_to_write.GetNumColumn() ||
      NULL == output_stream) {
    return false;
  }

  output_stream->write(reinterpret_cast<const char*>(&(matrix_to_write[0][0])),
                       sizeof(matrix_to_write[0][0]) *
                           matrix_to_write.GetNumRow() *
                           matrix_to_write.GetNumColumn());

  return !output_stream->fail();
}

template <typename T>
bool WriteStream(int write_point, int write_size,
                 const std::vector<T>& sequence_to_write,
                 std::ostream* output_stream, int* actual_write_size) {
  if (write_point < 0 || write_size <= 0 || NULL == output_stream) {
    return false;
  }

  const int end(write_point + write_size);
  if (sequence_to_write.size() < static_cast<std::size_t>(end)) {
    return false;
  }

  const int before((NULL == actual_write_size)
                       ? 0
                       : static_cast<int>(output_stream->tellp()));

  output_stream->write(
      reinterpret_cast<const char*>(&(sequence_to_write[0]) + write_point),
      sizeof(sequence_to_write[0]) * write_size);

  // When output_stream is cout, actual_write_size is always zero.
  if (NULL != actual_write_size) {
    const int after(output_stream->tellp());
    const int type_byte(sizeof(sequence_to_write[0]));
    *actual_write_size = (after - before) / type_byte;
  }

  return !output_stream->fail();
}

template <>
bool WriteStream(int write_point, int write_size,
                 const std::vector<std::string>& sequence_to_write,
                 std::ostream* output_stream, int* actual_write_size) {
  if (write_point < 0 || write_size <= 0 || NULL == output_stream) {
    return false;
  }

  const int end(write_point + write_size);
  if (sequence_to_write.size() < static_cast<std::size_t>(end)) {
    return false;
  }

  const int before((NULL == actual_write_size)
                       ? 0
                       : static_cast<int>(output_stream->tellp()));

  for (int i(write_point); i < end; ++i) {
    *output_stream << sequence_to_write[i] << std::endl;
  }

  if (NULL != actual_write_size) {
    const int after(output_stream->tellp());
    *actual_write_size = after - before;
  }

  return !output_stream->fail();
}

template <typename T>
bool SnPrintf(T data, const std::string& print_format, std::size_t buffer_size,
              char* buffer) {
  if (print_format.empty() || buffer_size <= 0 || NULL == buffer) {
    return false;
  }

  return (std::snprintf(buffer, buffer_size, print_format.c_str(), data) < 0)
             ? false
             : true;
}

template <>
bool SnPrintf(int24_t data, const std::string& print_format,
              std::size_t buffer_size, char* buffer) {
  if (print_format.empty() || buffer_size <= 0 || NULL == buffer) {
    return false;
  }

  return (std::snprintf(buffer, buffer_size, print_format.c_str(),
                        static_cast<int>(data)) < 0)
             ? false
             : true;
}

template <>
bool SnPrintf(uint24_t data, const std::string& print_format,
              std::size_t buffer_size, char* buffer) {
  if (print_format.empty() || buffer_size <= 0 || NULL == buffer) {
    return false;
  }

  return (std::snprintf(buffer, buffer_size, print_format.c_str(),
                        static_cast<int>(data)) < 0)
             ? false
             : true;
}

const char* ConvertBooleanToString(bool input) {
  return input ? "TRUE" : "FALSE";
}

bool ConvertStringToInteger(const std::string& input, int* output) {
  if (input.empty() || NULL == output) {
    return false;
  }

  char* end;
  int converted_value(static_cast<int>(std::strtol(input.c_str(), &end, 10)));

  if (0 == input.compare(end) || '\0' != *end || ERANGE == errno) {
    return false;
  }

  *output = converted_value;
  return true;
}

bool ConvertStringToDouble(const std::string& input, double* output) {
  if (input.empty() || NULL == output) {
    return false;
  }

  char* end;
  double converted_value(std::strtod(input.c_str(), &end));

  if (0 == input.compare(end) || '\0' != *end || ERANGE == errno) {
    return false;
  }

  *output = converted_value;
  return true;
}

bool ConvertSpecialStringToDouble(const std::string& input, double* output) {
  if (input.empty() || NULL == output) {
    return false;
  }

  std::string lowercase_input(input);
  std::transform(input.begin(), input.end(), lowercase_input.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if ("pi" == lowercase_input) {
    *output = sptk::kPi;
    return true;
  } else if ("db" == lowercase_input) {
    *output = sptk::kNeper;
    return true;
  } else if ("cent" == lowercase_input) {
    *output = 1200.0 * sptk::kOctave;
    return true;
  } else if ("semitone" == lowercase_input) {
    *output = 12.0 * sptk::kOctave;
    return true;
  } else if ("octave" == lowercase_input) {
    *output = sptk::kOctave;
    return true;
  } else if (lowercase_input.find("sqrt") == 0) {
    double tmp;
    if (ConvertStringToDouble(lowercase_input.substr(4), &tmp) && 0.0 <= tmp) {
      *output = std::sqrt(tmp);
      return true;
    }
  } else if (lowercase_input.find("ln") == 0) {
    double tmp;
    if (ConvertStringToDouble(lowercase_input.substr(2), &tmp) && 0.0 < tmp) {
      *output = std::log(tmp);
      return true;
    }
  } else if (lowercase_input.find("exp") == 0) {
    double tmp;
    if (ConvertStringToDouble(lowercase_input.substr(3), &tmp)) {
      *output = std::exp(tmp);
      return true;
    }
  }

  return false;
}

bool IsEven(int num) {
  return (0 == num % 2);
}

bool IsInRange(int num, int min, int max) {
  return (min <= num && num <= max);
}

bool IsInRange(double num, double min, double max) {
  return (min <= num && num <= max);
}

// check whether the given number is a power of two, 2^p where p is a
// non-negative integer.
bool IsPowerOfTwo(int num) {
  return !((num < 1) || (num & (num - 1)));
}

bool IsValidAlpha(double alpha) {
  return (std::fabs(alpha) < 1.0);
}

bool IsValidGamma(double gamma) {
  return (std::fabs(gamma) <= 1.0);
}

int NextPow(int num) {
  num--;
  num |= num >> 1;
  num |= num >> 2;
  num |= num >> 4;
  num |= num >> 8;
  num |= num >> 16;
  num++;
  return num;
}

int ExtractSign(double x) {
  if (0.0 < x) return 1;
  if (x < 0.0) return -1;
  return 0;
}

double FloorLog(double x) {
  return (x <= 0.0) ? kLogZero : std::log(x);
}

double FloorLog2(double x) {
  return (x <= 0.0) ? kLogZero : std::log2(x);
}

double FloorLog10(double x) {
  return (x <= 0.0) ? kLogZero : std::log10(x);
}

// compute log(x + y) given log(x) and log(y).
double AddInLogSpace(double log_x, double log_y) {
  if (log_x == log_y) return log_x + kLogTwo;

  const double smaller((log_x < log_y) ? log_x : log_y);
  const double greater((log_x < log_y) ? log_y : log_x);
  const double diff(smaller - greater);
  if (diff < kThresholdOfInformationLossInLogSpace) return greater;
  return greater + std::log(std::exp(diff) + 1.0);
}

void PrintDataType(const std::string& symbol, std::ostream* stream) {
  std::string data_type("");
  std::size_t data_size(0);

  if ("c" == symbol) {
    data_type = "char";
    data_size = sizeof(int8_t);
  } else if ("s" == symbol) {
    data_type = "short";
    data_size = sizeof(int16_t);
  } else if ("h" == symbol) {
    data_type = "int";
    data_size = sizeof(int24_t);
  } else if ("i" == symbol) {
    data_type = "int";
    data_size = sizeof(int32_t);
  } else if ("l" == symbol) {
    data_type = "long";
    data_size = sizeof(int64_t);
  } else if ("C" == symbol) {
    data_type = "unsigned char";
    data_size = sizeof(uint8_t);
  } else if ("S" == symbol) {
    data_type = "unsigned short";
    data_size = sizeof(uint16_t);
  } else if ("H" == symbol) {
    data_type = "unsigned int";
    data_size = sizeof(uint24_t);
  } else if ("I" == symbol) {
    data_type = "unsigned int";
    data_size = sizeof(uint32_t);
  } else if ("L" == symbol) {
    data_type = "unsigned long";
    data_size = sizeof(uint64_t);
  } else if ("f" == symbol) {
    data_type = "float";
    data_size = sizeof(float);
  } else if ("d" == symbol) {
    data_type = "double";
    data_size = sizeof(double);
  } else if ("e" == symbol) {
    data_type = "long double";
    data_size = sizeof(long double);
  } else if ("a" == symbol) {
    data_type = "ascii";
  }

  std::ostringstream oss;
  oss << std::setw(2) << std::left << symbol;
  if (0 < data_size) {
    oss << "(" << data_type << ", " << data_size << "byte)";
  } else {
    oss << "(" << data_type << ")";
  }
  *stream << std::setw(27) << std::left << oss.str();
}

void PrintErrorMessage(const std::string& program_name,
                       const std::ostringstream& message) {
  std::ostringstream stream;
  stream << program_name << ": " << message.str() << "!" << std::endl;
  std::cerr << stream.str();
}

// clang-format off
template bool ReadStream<bool>(bool*, std::istream*);
template bool ReadStream<int8_t>(int8_t*, std::istream*);
template bool ReadStream<int16_t>(int16_t*, std::istream*);
template bool ReadStream<int24_t>(int24_t*, std::istream*);
template bool ReadStream<int32_t>(int32_t*, std::istream*);
template bool ReadStream<int64_t>(int64_t*, std::istream*);
template bool ReadStream<uint8_t>(uint8_t*, std::istream*);
template bool ReadStream<uint16_t>(uint16_t*, std::istream*);
template bool ReadStream<uint24_t>(uint24_t*, std::istream*);
template bool ReadStream<uint32_t>(uint32_t*, std::istream*);
template bool ReadStream<uint64_t>(uint64_t*, std::istream*);
template bool ReadStream<float>(float*, std::istream*);
template bool ReadStream<double>(double*, std::istream*);
template bool ReadStream<long double>(long double*, std::istream*);
template bool ReadStream<int8_t>(bool, int, int, int, std::vector<int8_t>*, std::istream*, int*);            // NOLINT
template bool ReadStream<int16_t>(bool, int, int, int, std::vector<int16_t>*, std::istream*, int*);          // NOLINT
template bool ReadStream<int24_t>(bool, int, int, int, std::vector<int24_t>*, std::istream*, int*);          // NOLINT
template bool ReadStream<int32_t>(bool, int, int, int, std::vector<int32_t>*, std::istream*, int*);          // NOLINT
template bool ReadStream<int64_t>(bool, int, int, int, std::vector<int64_t>*, std::istream*, int*);          // NOLINT
template bool ReadStream<uint8_t>(bool, int, int, int, std::vector<uint8_t>*, std::istream*, int*);          // NOLINT
template bool ReadStream<uint16_t>(bool, int, int, int, std::vector<uint16_t>*, std::istream*, int*);        // NOLINT
template bool ReadStream<uint24_t>(bool, int, int, int, std::vector<uint24_t>*, std::istream*, int*);        // NOLINT
template bool ReadStream<uint32_t>(bool, int, int, int, std::vector<uint32_t>*, std::istream*, int*);        // NOLINT
template bool ReadStream<uint64_t>(bool, int, int, int, std::vector<uint64_t>*, std::istream*, int*);        // NOLINT
template bool ReadStream<float>(bool, int, int, int, std::vector<float>*, std::istream*, int*);              // NOLINT
template bool ReadStream<double>(bool, int, int, int, std::vector<double>*, std::istream*, int*);            // NOLINT
template bool ReadStream<long double>(bool, int, int, int, std::vector<long double>*, std::istream*, int*);  // NOLINT
template bool WriteStream<bool>(bool, std::ostream*);
template bool WriteStream<int8_t>(int8_t, std::ostream*);
template bool WriteStream<int16_t>(int16_t, std::ostream*);
template bool WriteStream<int24_t>(int24_t, std::ostream*);
template bool WriteStream<int32_t>(int32_t, std::ostream*);
template bool WriteStream<int64_t>(int64_t, std::ostream*);
template bool WriteStream<uint8_t>(uint8_t, std::ostream*);
template bool WriteStream<uint16_t>(uint16_t, std::ostream*);
template bool WriteStream<uint24_t>(uint24_t, std::ostream*);
template bool WriteStream<uint32_t>(uint32_t, std::ostream*);
template bool WriteStream<uint64_t>(uint64_t, std::ostream*);
template bool WriteStream<float>(float, std::ostream*);
template bool WriteStream<double>(double, std::ostream*);
template bool WriteStream<long double>(long double, std::ostream*);
template bool WriteStream<int8_t>(int, int, const std::vector<int8_t>&, std::ostream*, int*);            // NOLINT
template bool WriteStream<int16_t>(int, int, const std::vector<int16_t>&, std::ostream*, int*);          // NOLINT
template bool WriteStream<int24_t>(int, int, const std::vector<int24_t>&, std::ostream*, int*);          // NOLINT
template bool WriteStream<int32_t>(int, int, const std::vector<int32_t>&, std::ostream*, int*);          // NOLINT
template bool WriteStream<int64_t>(int, int, const std::vector<int64_t>&, std::ostream*, int*);          // NOLINT
template bool WriteStream<uint8_t>(int, int, const std::vector<uint8_t>&, std::ostream*, int*);          // NOLINT
template bool WriteStream<uint16_t>(int, int, const std::vector<uint16_t>&, std::ostream*, int*);        // NOLINT
template bool WriteStream<uint24_t>(int, int, const std::vector<uint24_t>&, std::ostream*, int*);        // NOLINT
template bool WriteStream<uint32_t>(int, int, const std::vector<uint32_t>&, std::ostream*, int*);        // NOLINT
template bool WriteStream<uint64_t>(int, int, const std::vector<uint64_t>&, std::ostream*, int*);        // NOLINT
template bool WriteStream<float>(int, int, const std::vector<float>&, std::ostream*, int*);              // NOLINT
template bool WriteStream<double>(int, int, const std::vector<double>&, std::ostream*, int*);            // NOLINT
template bool WriteStream<long double>(int, int, const std::vector<long double>&, std::ostream*, int*);  // NOLINT
template bool SnPrintf(int8_t, const std::string&, std::size_t, char*);
template bool SnPrintf(int16_t, const std::string&, std::size_t, char*);
template bool SnPrintf(int32_t, const std::string&, std::size_t, char*);
template bool SnPrintf(int64_t, const std::string&, std::size_t, char*);
template bool SnPrintf(uint8_t, const std::string&, std::size_t, char*);
template bool SnPrintf(uint16_t, const std::string&, std::size_t, char*);
template bool SnPrintf(uint32_t, const std::string&, std::size_t, char*);
template bool SnPrintf(uint64_t, const std::string&, std::size_t, char*);
template bool SnPrintf(float, const std::string&, std::size_t, char*);
template bool SnPrintf(double, const std::string&, std::size_t, char*);
template bool SnPrintf(long double, const std::string&, std::size_t, char*);
// clang-format on

}  // namespace sptk
