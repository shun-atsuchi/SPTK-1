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

#ifndef SPTK_UTILS_LINEAR_PREDICTIVE_COEFFICIENTS_STABILITY_CHECK_H_
#define SPTK_UTILS_LINEAR_PREDICTIVE_COEFFICIENTS_STABILITY_CHECK_H_

#include <vector>  // std::vector

#include "SPTK/conversion/linear_predictive_coefficients_to_parcor_coefficients.h"
#include "SPTK/conversion/parcor_coefficients_to_linear_predictive_coefficients.h"
#include "SPTK/utils/sptk_utils.h"

namespace sptk {

class LinearPredictiveCoefficientsStabilityCheck {
 public:
  //
  class Buffer {
   public:
    Buffer() {
    }
    virtual ~Buffer() {
    }

   private:
    LinearPredictiveCoefficientsToParcorCoefficients::Buffer conversion_buffer_;
    ParcorCoefficientsToLinearPredictiveCoefficients::Buffer
        reconversion_buffer_;
    std::vector<double> parcor_coefficients_;
    friend class LinearPredictiveCoefficientsStabilityCheck;
    DISALLOW_COPY_AND_ASSIGN(Buffer);
  };

  //
  LinearPredictiveCoefficientsStabilityCheck(int num_order, double margin);

  //
  virtual ~LinearPredictiveCoefficientsStabilityCheck() {
  }

  //
  int GetNumOrder() const {
    return num_order_;
  }

  //
  double GetMargin() const {
    return margin_;
  }

  //
  bool IsValid() const {
    return is_valid_;
  }

  // Check stability of linear predictive coefficients,
  // The 2nd argument of this function is allowed to be NULL.
  bool Run(const std::vector<double>& linear_predictive_coefficients,
           std::vector<double>* modified_linear_predictive_coefficients,
           bool* is_stable,
           LinearPredictiveCoefficientsStabilityCheck::Buffer* buffer) const;

 private:
  //
  const int num_order_;

  //
  const double margin_;

  //
  const LinearPredictiveCoefficientsToParcorCoefficients
      linear_predictive_coefficients_to_parcor_coefficients_;

  //
  const ParcorCoefficientsToLinearPredictiveCoefficients
      parcor_coefficients_to_linear_predictive_coefficients_;

  //
  bool is_valid_;

  //
  DISALLOW_COPY_AND_ASSIGN(LinearPredictiveCoefficientsStabilityCheck);
};

}  // namespace sptk

#endif  // SPTK_UTILS_LINEAR_PREDICTIVE_COEFFICIENTS_STABILITY_CHECK_H_
