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
//                1996-2020  Nagoya Institute of Technology          //
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

#include <getopt.h>  // getopt_long

#include <fstream>   // std::ifstream
#include <iomanip>   // std::setw
#include <iostream>  // std::cerr, std::cin, std::cout, std::endl, etc.
#include <sstream>   // std::ostringstream
#include <vector>    // std::vector

#include "SPTK/generation/recursive_maximum_likelihood_parameter_generation.h"
#include "SPTK/input/input_source_from_stream.h"
#include "SPTK/input/input_source_interface.h"
#include "SPTK/utils/sptk_utils.h"

namespace {

enum InputFormats {
  kMeanAndVariance = 0,
  kMeanAndPrecision,
  kMeanTimesPrecisionAndPrecision,
  kNumInputFormats
};

const int kDefaultNumOrder(25);
const int kDefaultNumPastFrame(30);
const InputFormats kDefaultInputFormat(kMeanAndVariance);

void PrintUsage(std::ostream* stream) {
  // clang-format off
  *stream << std::endl;
  *stream << " mlpg - maximum-likelihood parameter generation" << std::endl;
  *stream << std::endl;
  *stream << "  usage:" << std::endl;
  *stream << "       mlpg [ options ] [ infile ] > stdout" << std::endl;
  *stream << "  options:" << std::endl;
  *stream << "       -l l          : length of vector        (   int)[" << std::setw(5) << std::right << kDefaultNumOrder + 1 << "][ 1 <= l <=   ]" << std::endl;  // NOLINT
  *stream << "       -m m          : order of vector         (   int)[" << std::setw(5) << std::right << "l-1"                << "][ 0 <= m <=   ]" << std::endl;  // NOLINT
  *stream << "       -s s          : number of past frames   (   int)[" << std::setw(5) << std::right << kDefaultNumPastFrame << "][ r <= s <=   ]" << std::endl;  // NOLINT
  *stream << "       -q q          : input format            (   int)[" << std::setw(5) << std::right << kDefaultInputFormat  << "][ 0 <= q <= 2 ]" << std::endl;  // NOLINT
  *stream << "                         0 (mean and variance)" << std::endl;
  *stream << "                         1 (mean and precision)" << std::endl;
  *stream << "                         2 (mean x precision and precision)" << std::endl;  // NOLINT
  *stream << "       -d d1 d2 ...  : delta coefficients      (double)[" << std::setw(5) << std::right << "N/A"                << "]" << std::endl;  // NOLINT
  *stream << "       -D D          : filename of double type (string)[" << std::setw(5) << std::right << "N/A"                << "]" << std::endl;  // NOLINT
  *stream << "                       delta coefficients" << std::endl;
  *stream << "       -r r1 (r2)    : width of regression     (   int)[" << std::setw(5) << std::right << "N/A"                << "]" << std::endl;  // NOLINT
  *stream << "                       coefficients" << std::endl;
  *stream << "       -h    : print this message" << std::endl;
  *stream << "  infile:" << std::endl;
  *stream << "       mean and variance parameter sequence    (double)[stdin]" << std::endl;  // NOLINT
  *stream << "  stdout:" << std::endl;
  *stream << "       static parameter sequence               (double)" << std::endl;  // NOLINT
  *stream << "  notice:" << std::endl;
  *stream << "       -d and -D options can be given multiple times" << std::endl;  // NOLINT
  *stream << "       implmented algorithm is recursive using kalman filter" << std::endl;  // NOLINT
  *stream << "       magic number is not supported currently" << std::endl;  // NOLINT
  *stream << std::endl;
  *stream << " SPTK: version " << sptk::kVersion << std::endl;
  *stream << std::endl;
  // clang-format on
}

class InputSourcePreprocessing : public sptk::InputSourceInterface {
 public:
  InputSourcePreprocessing(InputFormats input_format,
                           sptk::InputSourceInterface* source)
      : input_format_(input_format),
        half_read_size_(source ? source->GetSize() / 2 : 0),
        source_(source),
        is_valid_(true) {
    if (NULL == source || !source->IsValid()) {
      is_valid_ = false;
      return;
    }
  }

  virtual ~InputSourcePreprocessing() {
  }

  virtual int GetSize() const {
    return source_ ? source_->GetSize() : 0;
  }

  virtual bool IsValid() const {
    return is_valid_;
  }

  virtual bool Get(std::vector<double>* buffer) {
    if (!is_valid_) {
      return false;
    }
    if (!source_->Get(buffer)) {
      return false;
    }
    switch (input_format_) {
      case kMeanAndVariance: {
        // Nothing to do.
        break;
      }
      case kMeanAndPrecision: {
        double* variance(&((*buffer)[half_read_size_]));
        for (int i(0); i < half_read_size_; ++i) {
          variance[i] = 1.0 / variance[i];
        }
        break;
      }
      case kMeanTimesPrecisionAndPrecision: {
        double* variance(&((*buffer)[half_read_size_]));
        for (int i(0); i < half_read_size_; ++i) {
          variance[i] = 1.0 / variance[i];
        }
        double* mean(&((*buffer)[0]));
        for (int i(0); i < half_read_size_; ++i) {
          mean[i] *= variance[i];
        }
        break;
      }
      default: { break; }
    }
    return true;
  }

 private:
  const InputFormats input_format_;
  const int half_read_size_;

  InputSourceInterface* source_;
  bool is_valid_;

  DISALLOW_COPY_AND_ASSIGN(InputSourcePreprocessing);
};

}  // namespace

/**
 * @a mlpg [ @e option ] [ @e infile ]
 *
 * - @b -l @e int
 *   - length of vector @f$(1 \le M + 1)@f$
 * - @b -m @e int
 *   - order of vector @f$(0 \le M)@f$
 * - @b -s @e int
 *   - number of past frames @f$(0 \le S)@f$
 * - @b -q @e int
 *   - input format
 *     \arg @c 0 @f$\boldsymbol{\mu}@f$, @f$\boldsymbol{\varSigma}@f$
 *     \arg @c 1 @f$\boldsymbol{\mu}@f$, @f$\boldsymbol{\varSigma}^{-1}@f$
 *     \arg @c 2 @f$\boldsymbol{\mu\varSigma}^{-1}@f$,
 *               @f$\boldsymbol{\varSigma}^{-1}@f$
 * - @b -d @e double+
 *   - delta coefficients
 * - @b -D @e string
 *   - filename of double-type delta coefficients
 * - @b -r @e int+
 *   - width of 1st (and 2nd) regression coefficients
 * - @b infile @e str
 *   - double-type mean and variance parameter sequence
 * - @b stdout
 *   - double-type static parameter sequence
 */
int main(int argc, char* argv[]) {
  int num_order(kDefaultNumOrder);
  int num_past_frame(kDefaultNumPastFrame);
  InputFormats input_format(kDefaultInputFormat);
  std::vector<std::vector<double> > window_coefficients;
  bool is_regression_specified(false);

  for (;;) {
    const int option_char(
        getopt_long(argc, argv, "l:m:s:q:d:D:r:h", NULL, NULL));
    if (-1 == option_char) break;

    switch (option_char) {
      case 'l': {
        if (!sptk::ConvertStringToInteger(optarg, &num_order) ||
            num_order <= 0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -l option must be a positive integer";
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }
        --num_order;
        break;
      }
      case 'm': {
        if (!sptk::ConvertStringToInteger(optarg, &num_order) ||
            num_order < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -m option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }
        break;
      }
      case 's': {
        if (!sptk::ConvertStringToInteger(optarg, &num_past_frame) ||
            num_past_frame < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -s option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }
        break;
      }
      case 'q': {
        const int min(0);
        const int max(static_cast<int>(kNumInputFormats) - 1);
        int tmp;
        if (!sptk::ConvertStringToInteger(optarg, &tmp) ||
            !sptk::IsInRange(tmp, min, max)) {
          std::ostringstream error_message;
          error_message << "The argument for the -q option must be an integer "
                        << "in the range of " << min << " to " << max;
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }
        input_format = static_cast<InputFormats>(tmp);
        break;
      }
      case 'd': {
        if (is_regression_specified) {
          std::ostringstream error_message;
          error_message << "-d and -r options cannot be specified at the same";
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }

        std::vector<double> coefficients;
        double coefficient;
        if (!sptk::ConvertStringToDouble(optarg, &coefficient)) {
          std::ostringstream error_message;
          error_message << "The argument for the -d option must be numeric";
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }
        coefficients.push_back(coefficient);
        while (optind < argc &&
               sptk::ConvertStringToDouble(argv[optind], &coefficient)) {
          coefficients.push_back(coefficient);
          ++optind;
        }
        window_coefficients.push_back(coefficients);
        break;
      }
      case 'D': {
        if (is_regression_specified) {
          std::ostringstream error_message;
          error_message << "-D and -r options cannot be specified at the same";
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }

        std::ifstream ifs;
        ifs.open(optarg, std::ios::in | std::ios::binary);
        if (ifs.fail()) {
          std::ostringstream error_message;
          error_message << "Cannot open file " << optarg;
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }
        std::vector<double> coefficients;
        double coefficient;
        while (sptk::ReadStream(&coefficient, &ifs)) {
          coefficients.push_back(coefficient);
        }
        window_coefficients.push_back(coefficients);
        break;
      }
      case 'r': {
        if (is_regression_specified) {
          std::ostringstream error_message;
          error_message << "-r option cannot be specified multiple times";
          sptk::PrintErrorMessage("mlpg", error_message);
          return 1;
        }

        int n;
        // Set first order coefficients.
        {
          if (!sptk::ConvertStringToInteger(optarg, &n) || n <= 0) {
            std::ostringstream error_message;
            error_message
                << "The argument for the -r option must be positive integer(s)";
            sptk::PrintErrorMessage("mlpg", error_message);
            return 1;
          }

          std::vector<double> coefficients(2 * n + 1);
          const int a1(n * (n + 1) * (2 * n + 1) / 3);
          const double norm(1.0 / a1);
          for (int j(-n), i(0); j <= n; ++j, ++i) {
            coefficients[i] = j * norm;
          }
          window_coefficients.push_back(coefficients);
        }

        // Set second order coefficients.
        if (optind < argc && sptk::ConvertStringToInteger(argv[optind], &n)) {
          if (n <= 0) {
            std::ostringstream error_message;
            error_message
                << "The argument for the -r option must be positive integer(s)";
            sptk::PrintErrorMessage("mlpg", error_message);
            return 1;
          }
          std::vector<double> coefficients(2 * n + 1);
          const int a0(2 * n + 1);
          const int a1(a0 * n * (n + 1) / 3);
          const int a2(a1 * (3 * n * n + 3 * n - 1) / 5);
          const double norm(2.0 / (a2 * a0 - a1 * a1));
          for (int j(-n), i(0); j <= n; ++j, ++i) {
            coefficients[i] = (a0 * j * j - a1) * norm;
          }
          window_coefficients.push_back(coefficients);
          ++optind;
        }
        is_regression_specified = true;
        break;
      }
      case 'h': {
        PrintUsage(&std::cout);
        return 0;
      }
      default: {
        PrintUsage(&std::cerr);
        return 1;
      }
    }
  }

  const int num_input_files(argc - optind);
  if (1 < num_input_files) {
    std::ostringstream error_message;
    error_message << "Too many input files";
    sptk::PrintErrorMessage("mlpg", error_message);
    return 1;
  }
  const char* input_file(0 == num_input_files ? NULL : argv[optind]);

  std::ifstream ifs;
  ifs.open(input_file, std::ios::in | std::ios::binary);
  if (ifs.fail() && NULL != input_file) {
    std::ostringstream error_message;
    error_message << "Cannot open file " << input_file;
    sptk::PrintErrorMessage("mlpg", error_message);
    return 1;
  }
  std::istream& input_stream(ifs.fail() ? std::cin : ifs);

  const int static_size(num_order + 1);
  const int read_size(2 * static_size * (window_coefficients.size() + 1));
  sptk::InputSourceFromStream input_source(false, read_size, &input_stream);
  InputSourcePreprocessing preprocessed_source(input_format, &input_source);

  sptk::RecursiveMaximumLikelihoodParameterGeneration generator(
      num_order, num_past_frame, window_coefficients, &preprocessed_source);
  if (!generator.IsValid()) {
    std::ostringstream error_message;
    error_message
        << "Failed to initialize RecursiveMaximumLikelihoodParameterGeneration";
    sptk::PrintErrorMessage("mlpg", error_message);
    return 1;
  }

  std::vector<double> smoothed_static_parameters(static_size);
  while (generator.Get(&smoothed_static_parameters)) {
    if (!sptk::WriteStream(0, static_size, smoothed_static_parameters,
                           &std::cout, NULL)) {
      std::ostringstream error_message;
      error_message << "Failed to write static parameters";
      sptk::PrintErrorMessage("mlpg", error_message);
      return 1;
    }
  }

  return 0;
}
