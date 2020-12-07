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

#include "SPTK/filter/pseudo_quadrature_mirror_filter_banks.h"
#include "SPTK/utils/sptk_utils.h"

namespace {

const int kDefaultNumSubband(4);
const int kDefaultNumFilterOrder(47);
const double kDefaultAttenuation(100.0);
const int kDefaultNumIteration(100);
const double kDefaultConvergenceThreshold(1e-6);
const double kDefaultInitialStepSize(1e-2);

void PrintUsage(std::ostream* stream) {
  // clang-format off
  *stream << std::endl;
  *stream << " pqmf - pseudo quadrature mirror filter banks analysis" << std::endl;  // NOLINT
  *stream << std::endl;
  *stream << "  usage:" << std::endl;
  *stream << "       pqmf [ options ] [ infile ] > stdout" << std::endl;
  *stream << "  options:" << std::endl;
  *stream << "       -k k  : number of subbands         (   int)[" << std::setw(5) << std::right << kDefaultNumSubband           << "][   1 <= k <=   ]" << std::endl;  // NOLINT
  *stream << "       -m m  : order of filter            (   int)[" << std::setw(5) << std::right << kDefaultNumFilterOrder       << "][   2 <= m <=   ]" << std::endl;  // NOLINT
  *stream << "     (level 2)" << std::endl;
  *stream << "       -a a  : stopband attenuation in dB (double)[" << std::setw(5) << std::right << kDefaultAttenuation          << "][   0 <  a <=   ]" << std::endl;  // NOLINT
  *stream << "       -i i  : number of iterations       (   int)[" << std::setw(5) << std::right << kDefaultNumIteration         << "][   0 <  i <=   ]" << std::endl;  // NOLINT
  *stream << "       -d d  : convergence threshold      (double)[" << std::setw(5) << std::right << kDefaultConvergenceThreshold << "][ 0.0 <= d <=   ]" << std::endl;  // NOLINT
  *stream << "       -s s  : initial step size          (dobule)[" << std::setw(5) << std::right << kDefaultInitialStepSize      << "][   0 <  s <=   ]" << std::endl;  // NOLINT
  *stream << "       -h    : print this message" << std::endl;
  *stream << "  infile:" << std::endl;
  *stream << "       filter-bank input                  (double)[stdin]" << std::endl;  // NOLINT
  *stream << "  stdout:" << std::endl;
  *stream << "       filter-bank output                 (double)" << std::endl;
  *stream << std::endl;
  *stream << " SPTK: version " << sptk::kVersion << std::endl;
  *stream << std::endl;
  // clang-format on
}

}  // namespace

/**
 * @a pqmf [ @e option ] [ @e infile ]
 *
 * - @b -k @e int
 *   - number of subbands @f$(1 \le K)@f$
 * - @b -m @e int
 *   - order of filter @f$(2 \le M)@f$
 * - @b -a @e double
 *   - stopband attenuation @f$(0 < \alpha)@f$
 * - @b -i @e int
 *   - number of iterations @f$(1 \le N)@f$
 * - @b -d @e double
 *   - convergence threshold @f$(0 \le \epsilon)@f$
 * - @b -s @e double
 *   - initial step size @f$(0 < \Delta)@f$
 * - @b infile @e str
 *   - double-type filter-bank input
 * - @b stdout
 *   - double-type filter-bank output
 *
 * In the below example, the signal in @c data.short is decomposed into four
 * subband signals.
 *
 * @code{.sh}
 *   x2x +sd data.short | pqmf -k 4 | decimate -l 4 -p 4 > data.sub
 * @endcode
 *
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument vector.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char* argv[]) {
  int num_subband(kDefaultNumSubband);
  int num_filter_order(kDefaultNumFilterOrder);
  double attenuation(kDefaultAttenuation);
  int num_iteration(kDefaultNumIteration);
  double convergence_threshold(kDefaultConvergenceThreshold);
  double initial_step_size(kDefaultInitialStepSize);

  for (;;) {
    const int option_char(getopt_long(argc, argv, "k:m:a:i:d:s:h", NULL, NULL));
    if (-1 == option_char) break;

    switch (option_char) {
      case 'k': {
        if (!sptk::ConvertStringToInteger(optarg, &num_subband) ||
            num_subband <= 0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -k option must be a positive integer";
          sptk::PrintErrorMessage("pqmf", error_message);
          return 1;
        }
        break;
      }
      case 'm': {
        if (!sptk::ConvertStringToInteger(optarg, &num_filter_order) ||
            num_filter_order <= 1) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -m option must be greater than 1";
          sptk::PrintErrorMessage("pqmf", error_message);
          return 1;
        }
        break;
      }
      case 'a': {
        if (!sptk::ConvertStringToDouble(optarg, &attenuation) ||
            attenuation <= 0.0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -a option must be a positive number";
          sptk::PrintErrorMessage("pqmf", error_message);
          return 1;
        }
        break;
      }
      case 'i': {
        if (!sptk::ConvertStringToInteger(optarg, &num_iteration) ||
            num_iteration <= 0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -i option must be a positive integer";
          sptk::PrintErrorMessage("pqmf", error_message);
          return 1;
        }
        break;
      }
      case 'd': {
        if (!sptk::ConvertStringToDouble(optarg, &convergence_threshold) ||
            convergence_threshold < 0.0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -d option must be a non-negative number";
          sptk::PrintErrorMessage("pqmf", error_message);
          return 1;
        }
        break;
      }
      case 's': {
        if (!sptk::ConvertStringToDouble(optarg, &initial_step_size) ||
            initial_step_size <= 0.0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -s option must be a positive number";
          sptk::PrintErrorMessage("pqmf", error_message);
          return 1;
        }
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
    sptk::PrintErrorMessage("pqmf", error_message);
    return 1;
  }
  const char* input_file(0 == num_input_files ? NULL : argv[optind]);

  std::ifstream ifs;
  ifs.open(input_file, std::ios::in | std::ios::binary);
  if (ifs.fail() && NULL != input_file) {
    std::ostringstream error_message;
    error_message << "Cannot open file " << input_file;
    sptk::PrintErrorMessage("pqmf", error_message);
    return 1;
  }
  std::istream& input_stream(ifs.fail() ? std::cin : ifs);

  sptk::PseudoQuadratureMirrorFilterBanks analysis(
      num_subband, num_filter_order, attenuation, num_iteration,
      convergence_threshold, initial_step_size);
  sptk::PseudoQuadratureMirrorFilterBanks::Buffer buffer;
  if (!analysis.IsValid()) {
    std::ostringstream error_message;
    error_message << "Failed to initialize PseudoQuadratureMirrorFilterBanks";
    sptk::PrintErrorMessage("pqmf", error_message);
    return 1;
  }

  double input;
  std::vector<double> output(num_subband);

  while (sptk::ReadStream(&input, &input_stream)) {
    if (!analysis.Run(input, &output, &buffer)) {
      std::ostringstream error_message;
      error_message << "Failed to perform PQMF analysis";
      sptk::PrintErrorMessage("pqmf", error_message);
      return 1;
    }
    if (!sptk::WriteStream(0, num_subband, output, &std::cout, NULL)) {
      std::ostringstream error_message;
      error_message << "Failed to write subband signals";
      sptk::PrintErrorMessage("pqmf", error_message);
      return 1;
    }
  }

  return 0;
}
