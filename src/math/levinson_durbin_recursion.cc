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

#include "SPTK/math/levinson_durbin_recursion.h"

#include <cmath>    // std::fabs, std::isnan, std::sqrt
#include <cstddef>  // std::size_t

namespace sptk {

LevinsonDurbinRecursion::LevinsonDurbinRecursion(int num_order)
    : num_order_(num_order), is_valid_(true) {
  if (num_order_ < 0) {
    is_valid_ = false;
    return;
  }
}

bool LevinsonDurbinRecursion::Run(
    const std::vector<double>& autocorrelation,
    std::vector<double>* linear_predictive_coefficients, bool* is_stable,
    LevinsonDurbinRecursion::Buffer* buffer) const {
  // Check inputs.
  const int length(num_order_ + 1);
  if (!is_valid_ ||
      autocorrelation.size() != static_cast<std::size_t>(length) ||
      NULL == linear_predictive_coefficients || NULL == is_stable ||
      NULL == buffer) {
    return false;
  }

  // Prepare memories.
  if (linear_predictive_coefficients->size() !=
      static_cast<std::size_t>(length)) {
    linear_predictive_coefficients->resize(length);
  }
  if (buffer->c_.size() != static_cast<std::size_t>(length)) {
    buffer->c_.resize(length);
  }

  *is_stable = true;

  const double* r(&(autocorrelation[0]));
  double* a(&((*linear_predictive_coefficients)[0]));
  double* c(&buffer->c_[0]);

  // Set initial condition.
  a[0] = 0.0;
  double e(r[0]);
  if (0.0 == e || std::isnan(e)) {
    return false;
  }

  // Perform Durbin's iterative algorithm.
  for (int i(1); i < length; ++i) {
    double k(-r[i]);
    for (int j(1); j < i; ++j) {
      k -= c[j] * r[i - j];
    }
    k /= e;

    if (1.0 <= std::fabs(k)) {
      *is_stable = false;
    }

    for (int j(1); j < i; ++j) {
      a[j] = c[j] + k * c[i - j];
    }
    a[i] = k;

    e *= 1.0 - k * k;
    if (0.0 == e || std::isnan(e)) {
      return false;
    }

    for (int j(0); j <= i; ++j) {
      c[j] = a[j];
    }
  }

  // Set gain.
  a[0] = std::sqrt(e);

  return true;
}

bool LevinsonDurbinRecursion::Run(
    std::vector<double>* input_and_output, bool* is_stable,
    LevinsonDurbinRecursion::Buffer* buffer) const {
  if (NULL == input_and_output) return false;
  std::vector<double> input(*input_and_output);
  return Run(input, input_and_output, is_stable, buffer);
}

}  // namespace sptk
