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

#include <getopt.h>  // getopt_long
#include <cfloat>    // DBL_MAX
#include <fstream>   // std::ifstream
#include <iomanip>   // std::setw
#include <iostream>  // std::cerr, std::cin, std::cout, std::endl, etc.
#include <sstream>   // std::ostringstream
#include <vector>    // std::vector

#include "SPTK/conversion/filter_coefficients_to_spectrum.h"
#include "SPTK/conversion/waveform_to_spectrum.h"
#include "SPTK/utils/sptk_utils.h"

namespace {

const int kDefaultFftLength(256);
const int kDefaultNumNumeratorOrder(0);
const int kDefaultNumDenominatorOrder(0);
const double kDefaultEpsilonForCalculatingLogarithms(0.0);
const double kDefaultRelativeFloorInDecibels(-DBL_MAX);
const sptk::FilterCoefficientsToSpectrum::OutputFormats kDefaultOutputFormat(
    sptk::FilterCoefficientsToSpectrum::kLogAmplitudeSpectrumInDecibels);

void PrintUsage(std::ostream* stream) {
  // clang-format off
  *stream << std::endl;
  *stream << " spec - transform real sequence to spectrum" << std::endl;
  *stream << std::endl;
  *stream << "  usage:" << std::endl;
  *stream << "       spec [ options ] [ infile ] > stdout" << std::endl;
  *stream << "  options:" << std::endl;
  *stream << "       -l l  : FFT length                             (   int)[" << std::setw(5) << std::right << kDefaultFftLength           << "][   2 <= l <=     ]" << std::endl;  // NOLINT
  *stream << "       -m m  : order of numerator coefficients        (   int)[" << std::setw(5) << std::right << kDefaultNumNumeratorOrder   << "][   0 <= m <  l   ]" << std::endl;  // NOLINT
  *stream << "       -n n  : order of denominator coefficients      (   int)[" << std::setw(5) << std::right << kDefaultNumDenominatorOrder << "][   0 <= n <  l   ]" << std::endl;  // NOLINT
  *stream << "       -z z  : name of file containing                (string)[" << std::setw(5) << std::right << "N/A"                       << "]" << std::endl;  // NOLINT
  *stream << "               numerator coefficients" << std::endl;
  *stream << "       -p p  : name of file containing                (string)[" << std::setw(5) << std::right << "N/A"                       << "]" << std::endl;  // NOLINT
  *stream << "               denominator coefficients" << std::endl;
  *stream << "       -e e  : small value for calculating logarithms (double)[" << std::setw(5) << std::right << "N/A"                       << "][ 0.0 <  e <=     ]" << std::endl;  // NOLINT
  *stream << "       -E E  : relative floor in decibels             (double)[" << std::setw(5) << std::right << "N/A"                       << "][     <= E <  0.0 ]" << std::endl;  // NOLINT
  *stream << "       -o o  : output format                          (   int)[" << std::setw(5) << std::right << kDefaultOutputFormat        << "][   0 <= o <= 3   ]" << std::endl;  // NOLINT
  *stream << "                 0 (20*log|H(z)|)" << std::endl;
  *stream << "                 1 (ln|H(z)|)" << std::endl;
  *stream << "                 2 (|H(z)|)" << std::endl;
  *stream << "                 3 (|H(z)|^2)" << std::endl;
  *stream << "       -h    : print this message" << std::endl;
  *stream << "  infile:" << std::endl;
  *stream << "       data sequence                                  (double)[stdin]" << std::endl;  // NOLINT
  *stream << "  stdout:" << std::endl;
  *stream << "       spectrum                                       (double)" << std::endl;  // NOLINT
  *stream << "  notice:" << std::endl;
  *stream << "       value of l must be a power of 2" << std::endl;
  *stream << std::endl;
  *stream << " SPTK: version " << sptk::kVersion << std::endl;
  *stream << std::endl;
  // clang-format on
}

}  // namespace

int main(int argc, char* argv[]) {
  int fft_length(kDefaultFftLength);
  int num_numerator_order(kDefaultNumNumeratorOrder);
  int num_denominator_order(kDefaultNumDenominatorOrder);
  const char* numerator_coefficients_file(NULL);
  const char* denominator_coefficients_file(NULL);
  bool is_numerator_specified(false);
  bool is_denominator_specified(false);
  double epsilon_for_calculating_logarithms(
      kDefaultEpsilonForCalculatingLogarithms);
  double relative_floor_in_decibels(kDefaultRelativeFloorInDecibels);
  sptk::FilterCoefficientsToSpectrum::OutputFormats output_format(
      kDefaultOutputFormat);

  for (;;) {
    const int option_char(
        getopt_long(argc, argv, "l:m:n:z:p:e:E:o:h", NULL, NULL));
    if (-1 == option_char) break;

    switch (option_char) {
      case 'l': {
        if (!sptk::ConvertStringToInteger(optarg, &fft_length)) {
          std::ostringstream error_message;
          error_message << "The argument for the -l option must be an integer";
          sptk::PrintErrorMessage("spec", error_message);
          return 1;
        }
        break;
      }
      case 'm': {
        if (!sptk::ConvertStringToInteger(optarg, &num_numerator_order) ||
            num_numerator_order < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -m option must be a "
                           "non-negative integer";
          sptk::PrintErrorMessage("spec", error_message);
          return 1;
        }
        is_numerator_specified = true;
        break;
      }
      case 'n': {
        if (!sptk::ConvertStringToInteger(optarg, &num_denominator_order) ||
            num_denominator_order < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -n option must be a "
                           "non-negative integer";
          sptk::PrintErrorMessage("spec", error_message);
          return 1;
        }
        is_denominator_specified = true;
        break;
      }
      case 'z': {
        numerator_coefficients_file = optarg;
        is_numerator_specified = true;
        break;
      }
      case 'p': {
        denominator_coefficients_file = optarg;
        is_denominator_specified = true;
        break;
      }
      case 'e': {
        if (!sptk::ConvertStringToDouble(optarg,
                                         &epsilon_for_calculating_logarithms) ||
            epsilon_for_calculating_logarithms <= 0.0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -e option must be a positive number";
          sptk::PrintErrorMessage("spec", error_message);
          return 1;
        }
        break;
      }
      case 'E': {
        if (!sptk::ConvertStringToDouble(optarg, &relative_floor_in_decibels) ||
            0.0 <= relative_floor_in_decibels) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -E option must be a negative number";
          sptk::PrintErrorMessage("spec", error_message);
          return 1;
        }
        break;
      }
      case 'o': {
        const int min(0);
        const int max(
            static_cast<int>(
                sptk::FilterCoefficientsToSpectrum::kNumOutputFormats) -
            1);
        int tmp;
        if (!sptk::ConvertStringToInteger(optarg, &tmp) ||
            !sptk::IsInRange(tmp, min, max)) {
          std::ostringstream error_message;
          error_message << "The argument for the -o option must be an integer "
                        << "in the range of " << min << " to " << max;
          sptk::PrintErrorMessage("spec", error_message);
          return 1;
        }
        output_format =
            static_cast<sptk::FilterCoefficientsToSpectrum::OutputFormats>(tmp);
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

  if (is_numerator_specified || is_denominator_specified) {
    const int numerator_length(num_numerator_order + 1);
    const int denominator_length(num_denominator_order + 1);
    std::vector<double> numerator_coefficients(numerator_length);
    std::vector<double> denominator_coefficients(denominator_length);

    if (is_numerator_specified && is_denominator_specified &&
        (NULL == numerator_coefficients_file ||
         NULL == denominator_coefficients_file)) {
      std::ostringstream error_message;
      error_message
          << "Numerator and denominator coefficient files are required";
      sptk::PrintErrorMessage("spec", error_message);
      return 1;
    }

    const int num_input_files(argc - optind);
    if (0 < num_input_files) {
      std::ostringstream error_message;
      error_message << "Too many input files";
      sptk::PrintErrorMessage("spec", error_message);
      return 1;
    }

    // open stream
    std::ifstream ifs_for_numerator;
    if (is_numerator_specified) {
      ifs_for_numerator.open(numerator_coefficients_file,
                             std::ios::in | std::ios::binary);
      if (ifs_for_numerator.fail() && NULL != numerator_coefficients_file) {
        std::ostringstream error_message;
        error_message << "Cannot open file " << numerator_coefficients_file;
        sptk::PrintErrorMessage("spec", error_message);
        return 1;
      }
    } else {
      numerator_coefficients[0] = 1.0;
    }
    std::istream& input_stream_for_numerator(
        (is_numerator_specified && ifs_for_numerator.fail())
            ? std::cin
            : ifs_for_numerator);

    std::ifstream ifs_for_denominator;
    if (is_denominator_specified) {
      ifs_for_denominator.open(denominator_coefficients_file,
                               std::ios::in | std::ios::binary);
      if (ifs_for_denominator.fail() && NULL != denominator_coefficients_file) {
        std::ostringstream error_message;
        error_message << "Cannot open file " << denominator_coefficients_file;
        sptk::PrintErrorMessage("spec", error_message);
        return 1;
      }
    } else {
      denominator_coefficients[0] = 1.0;
    }
    std::istream& input_stream_for_denominator(
        (is_denominator_specified && ifs_for_denominator.fail())
            ? std::cin
            : ifs_for_denominator);

    sptk::FilterCoefficientsToSpectrum filter_coefficients_to_spectrum(
        num_numerator_order, num_denominator_order, fft_length, output_format,
        epsilon_for_calculating_logarithms, relative_floor_in_decibels);
    sptk::FilterCoefficientsToSpectrum::Buffer buffer;
    if (!filter_coefficients_to_spectrum.IsValid()) {
      std::ostringstream error_message;
      error_message << "Failed to set condition for transformation";
      sptk::PrintErrorMessage("spec", error_message);
      return 1;
    }

    const int output_length(fft_length / 2 + 1);
    std::vector<double> output(output_length);
    while ((!is_numerator_specified ||
            sptk::ReadStream(false, 0, 0, numerator_length,
                             &numerator_coefficients,
                             &input_stream_for_numerator, NULL)) &&
           (!is_denominator_specified ||
            sptk::ReadStream(false, 0, 0, denominator_length,
                             &denominator_coefficients,
                             &input_stream_for_denominator, NULL))) {
      if (!filter_coefficients_to_spectrum.Run(numerator_coefficients,
                                               denominator_coefficients,
                                               &output, &buffer)) {
        std::ostringstream error_message;
        error_message << "Failed to transform filter coefficients to spectrum";
        sptk::PrintErrorMessage("spec", error_message);
        return 1;
      }

      if (!sptk::WriteStream(0, output_length, output, &std::cout, NULL)) {
        std::ostringstream error_message;
        error_message << "Failed to write spectrum";
        sptk::PrintErrorMessage("spec", error_message);
        return 1;
      }
    }

  } else {
    // get input file
    const int num_input_files(argc - optind);
    if (1 < num_input_files) {
      std::ostringstream error_message;
      error_message << "Too many input files";
      sptk::PrintErrorMessage("spec", error_message);
      return 1;
    }
    const char* input_file(0 == num_input_files ? NULL : argv[optind]);

    // open stream
    std::ifstream ifs;
    ifs.open(input_file, std::ios::in | std::ios::binary);
    if (ifs.fail() && NULL != input_file) {
      std::ostringstream error_message;
      error_message << "Cannot open file " << input_file;
      sptk::PrintErrorMessage("spec", error_message);
      return 1;
    }
    std::istream& input_stream(ifs.fail() ? std::cin : ifs);

    sptk::WaveformToSpectrum waveform_to_spectrum(
        fft_length, fft_length, output_format,
        epsilon_for_calculating_logarithms, relative_floor_in_decibels);
    sptk::WaveformToSpectrum::Buffer buffer;
    if (!waveform_to_spectrum.IsValid()) {
      std::ostringstream error_message;
      error_message << "Failed to set condition for transformation";
      sptk::PrintErrorMessage("spec", error_message);
      return 1;
    }

    const int output_length(fft_length / 2 + 1);
    std::vector<double> waveform(fft_length);
    std::vector<double> output(output_length);
    while (sptk::ReadStream(true, 0, 0, fft_length, &waveform, &input_stream,
                            NULL)) {
      if (!waveform_to_spectrum.Run(waveform, &output, &buffer)) {
        std::ostringstream error_message;
        error_message << "Failed to transform waveform to spectrum";
        sptk::PrintErrorMessage("spec", error_message);
        return 1;
      }

      if (!sptk::WriteStream(0, output_length, output, &std::cout, NULL)) {
        std::ostringstream error_message;
        error_message << "Failed to write spectrum";
        sptk::PrintErrorMessage("spec", error_message);
        return 1;
      }
    }
  }

  return 0;
}
