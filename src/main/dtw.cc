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
#include <fstream>   // std::ifstream, std::ofstream
#include <iomanip>   // std::setw
#include <iostream>  // std::cerr, std::cin, std::cout, std::endl, etc.
#include <sstream>   // std::ostringstream
#include <utility>   // std::pair
#include <vector>    // std::vector

#include "SPTK/math/distance_calculation.h"
#include "SPTK/math/dynamic_time_warping.h"
#include "SPTK/utils/sptk_utils.h"

namespace {

const int kDefaultNumOrder(25);
const sptk::DynamicTimeWarping::LocalPathConstraints
    kDefaultLocalPathConstraint(
        sptk::DynamicTimeWarping::LocalPathConstraints::kType5);
const sptk::DistanceCalculation::DistanceMetrics kDefaultDistanceMetric(
    sptk::DistanceCalculation::DistanceMetrics::kSquaredEuclidean);

void PrintUsage(std::ostream* stream) {
  // clang-format off
  *stream << std::endl;
  *stream << " dtw - dynamic time warping" << std::endl;
  *stream << std::endl;
  *stream << "  usage:" << std::endl;
  *stream << "       dtw [ options ] file1 [ infile ] > stdout" << std::endl;
  *stream << "  options:" << std::endl;
  *stream << "       -l l  : length of vector               (   int)[" << std::setw(5) << std::right << kDefaultNumOrder + 1        << "][ 0 <  l <=   ]" << std::endl;  // NOLINT
  *stream << "       -m m  : order of vector                (   int)[" << std::setw(5) << std::right << "l-1"                       << "][ 0 <= m <=   ]" << std::endl;  // NOLINT
  *stream << "       -p p  : type of local path constraints (   int)[" << std::setw(5) << std::right << kDefaultLocalPathConstraint << "][ 0 <= p <= 6 ]" << std::endl;  // NOLINT
  *stream << "       -d d  : distance metric                (   int)[" << std::setw(5) << std::right << kDefaultDistanceMetric      << "][ 0 <= d <= 3 ]" << std::endl;  // NOLINT
  *stream << "                 0 (Manhattan)" << std::endl;
  *stream << "                 1 (Euclidean)" << std::endl;
  *stream << "                 2 (squared Euclidean)" << std::endl;
  *stream << "                 3 (symmetric Kullback-Leibler)" << std::endl;
  *stream << "       -P P  : output filename of int type    (string)[" << std::setw(5) << std::right << "N/A"                       << "]" << std::endl;  // NOLINT
  *stream << "               Viterbi path" << std::endl;
  *stream << "       -S S  : output filename of double type (string)[" << std::setw(5) << std::right << "N/A"                       << "]" << std::endl;  // NOLINT
  *stream << "               total score" << std::endl;
  *stream << "       -h    : print this message" << std::endl;
  *stream << "  file1:" << std::endl;
  *stream << "       reference vector sequence              (double)" << std::endl;  // NOLINT
  *stream << "  infile:" << std::endl;
  *stream << "       query vector sequence                  (double)[stdin]" << std::endl;  // NOLINT
  *stream << "  stdout:" << std::endl;
  *stream << "       warped vector sequence                 (double)" << std::endl;  // NOLINT
  *stream << std::endl;
  *stream << " SPTK: version " << sptk::kVersion << std::endl;
  *stream << std::endl;
  // clang-format on
}

}  // namespace

int main(int argc, char* argv[]) {
  int num_order(kDefaultNumOrder);
  sptk::DynamicTimeWarping::LocalPathConstraints local_path_constraint(
      kDefaultLocalPathConstraint);
  sptk::DistanceCalculation::DistanceMetrics distance_metric(
      kDefaultDistanceMetric);
  const char* total_score_file(NULL);
  const char* viterbi_path_file(NULL);

  for (;;) {
    const int option_char(getopt_long(argc, argv, "l:m:p:d:P:S:h", NULL, NULL));
    if (-1 == option_char) break;

    switch (option_char) {
      case 'l': {
        if (!sptk::ConvertStringToInteger(optarg, &num_order) ||
            num_order <= 0) {
          std::ostringstream error_message;
          error_message
              << "The argument for the -l option must be a positive integer";
          sptk::PrintErrorMessage("dtw", error_message);
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
          sptk::PrintErrorMessage("dtw", error_message);
          return 1;
        }
        break;
      }
      case 'p': {
        const int min(0);
        const int max(
            static_cast<int>(
                sptk::DynamicTimeWarping::LocalPathConstraints::kNumTypes) -
            1);
        int tmp;
        if (!sptk::ConvertStringToInteger(optarg, &tmp) ||
            !sptk::IsInRange(tmp, min, max)) {
          std::ostringstream error_message;
          error_message << "The argument for the -p option must be an integer "
                        << "in the range of " << min << " to " << max;
          sptk::PrintErrorMessage("dtw", error_message);
          return 1;
        }
        local_path_constraint =
            static_cast<sptk::DynamicTimeWarping::LocalPathConstraints>(tmp);
        break;
      }
      case 'd': {
        const int min(0);
        const int max(
            static_cast<int>(
                sptk::DistanceCalculation::DistanceMetrics::kNumMetrics) -
            1);
        int tmp;
        if (!sptk::ConvertStringToInteger(optarg, &tmp) ||
            !sptk::IsInRange(tmp, min, max)) {
          std::ostringstream error_message;
          error_message << "The argument for the -d option must be an integer "
                        << "in the range of " << min << " to " << max;
          sptk::PrintErrorMessage("dtw", error_message);
          return 1;
        }
        distance_metric =
            static_cast<sptk::DistanceCalculation::DistanceMetrics>(tmp);
        break;
      }
      case 'P': {
        viterbi_path_file = optarg;
        break;
      }
      case 'S': {
        total_score_file = optarg;
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

  // get input file
  const char* reference_file;
  const char* query_file;
  const int num_input_files(argc - optind);
  if (2 == num_input_files) {
    reference_file = argv[argc - 2];
    query_file = argv[argc - 1];
  } else if (1 == num_input_files) {
    reference_file = argv[argc - 1];
    query_file = NULL;
  } else {
    std::ostringstream error_message;
    error_message << "Just two input files, file1 and infile, are required";
    sptk::PrintErrorMessage("dtw", error_message);
    return 1;
  }

  const int length(num_order + 1);

  std::vector<std::vector<double> > reference_vectors;
  {
    std::ifstream ifs;
    ifs.open(reference_file, std::ios::in | std::ios::binary);
    if (ifs.fail()) {
      std::ostringstream error_message;
      error_message << "Cannot open file " << reference_file;
      sptk::PrintErrorMessage("dtw", error_message);
      return 1;
    }
    std::istream& input_stream(ifs);

    std::vector<double> tmp(length);
    while (sptk::ReadStream(false, 0, 0, length, &tmp, &input_stream, NULL)) {
      reference_vectors.push_back(tmp);
    }
  }

  std::vector<std::vector<double> > query_vectors;
  {
    std::ifstream ifs;
    ifs.open(query_file, std::ios::in | std::ios::binary);
    if (ifs.fail() && NULL != query_file) {
      std::ostringstream error_message;
      error_message << "Cannot open file " << query_file;
      sptk::PrintErrorMessage("dtw", error_message);
      return 1;
    }
    std::istream& input_stream(ifs.fail() ? std::cin : ifs);

    std::vector<double> tmp(length);
    while (sptk::ReadStream(false, 0, 0, length, &tmp, &input_stream, NULL)) {
      query_vectors.push_back(tmp);
    }
  }

  std::ofstream ofs1;
  if (NULL != total_score_file) {
    ofs1.open(total_score_file, std::ios::out | std::ios::binary);
    if (ofs1.fail()) {
      std::ostringstream error_message;
      error_message << "Cannot open file " << total_score_file;
      sptk::PrintErrorMessage("dtw", error_message);
      return 1;
    }
  }
  std::ostream& output_stream_for_score(ofs1);

  std::ofstream ofs2;
  if (NULL != viterbi_path_file) {
    ofs2.open(viterbi_path_file, std::ios::out | std::ios::binary);
    if (ofs2.fail()) {
      std::ostringstream error_message;
      error_message << "Cannot open file " << viterbi_path_file;
      sptk::PrintErrorMessage("dtw", error_message);
      return 1;
    }
  }
  std::ostream& output_stream_for_path(ofs2);

  sptk::DynamicTimeWarping dynamic_time_warping(
      num_order, local_path_constraint, distance_metric);
  if (!dynamic_time_warping.IsValid()) {
    std::ostringstream error_message;
    error_message << "Failed to set the condition for dynamic time warping";
    sptk::PrintErrorMessage("dtw", error_message);
    return 1;
  }

  std::vector<std::pair<int, int> > viterbi_path;
  double total_score;
  if (!dynamic_time_warping.Run(query_vectors, reference_vectors, &viterbi_path,
                                &total_score)) {
    std::ostringstream error_message;
    error_message << "Failed to run dynamic time warping";
    sptk::PrintErrorMessage("dtw", error_message);
    return 1;
  }

  for (std::vector<std::pair<int, int> >::iterator itr(viterbi_path.begin());
       itr != viterbi_path.end(); ++itr) {
    if (!sptk::WriteStream(0, length, query_vectors[itr->first], &std::cout,
                           NULL) ||
        !sptk::WriteStream(0, length, reference_vectors[itr->second],
                           &std::cout, NULL)) {
      std::ostringstream error_message;
      error_message << "Failed to write warped vector";
      sptk::PrintErrorMessage("dtw", error_message);
      return 1;
    }
  }

  if (NULL != viterbi_path_file) {
    for (std::vector<std::pair<int, int> >::iterator itr(viterbi_path.begin());
         itr != viterbi_path.end(); ++itr) {
      if (!sptk::WriteStream(itr->first, &output_stream_for_path) ||
          !sptk::WriteStream(itr->second, &output_stream_for_path)) {
        std::ostringstream error_message;
        error_message << "Failed to write Viterbi path";
        sptk::PrintErrorMessage("dtw", error_message);
        return 1;
      }
    }
  }

  if (NULL != total_score_file) {
    if (!sptk::WriteStream(total_score, &output_stream_for_score)) {
      std::ostringstream error_message;
      error_message << "Failed to write total score";
      sptk::PrintErrorMessage("dtw", error_message);
      return 1;
    }
  }

  return 0;
}
