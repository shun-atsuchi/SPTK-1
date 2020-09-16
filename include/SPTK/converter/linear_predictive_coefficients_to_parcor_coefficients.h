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

#ifndef SPTK_CONVERTER_LINEAR_PREDICTIVE_COEFFICIENTS_TO_PARCOR_COEFFICIENTS_H_
#define SPTK_CONVERTER_LINEAR_PREDICTIVE_COEFFICIENTS_TO_PARCOR_COEFFICIENTS_H_

#include <vector>  // std::vector

#include "SPTK/utils/sptk_utils.h"

namespace sptk {

/**
 * Transform LPC coefficients to PARCOR coefficients.
 *
 * The input is the \f$M\f$-th order LPC coefficients:
 * \f[
 *   \begin{array}{cccc}
 *     K, & a(1), & \ldots, & a(M),
 *   \end{array}
 * \f]
 * and the output is the \f$M\f$-th order PARCOR coefficients:
 * \f[
 *   \begin{array}{cccc}
 *     K, & k(1), & \ldots, & k(M),
 *   \end{array}
 * \f]
 * where \f$K\f$ is the gain. The transformation is given by the following
 * recursion formula:
 * \f{eqnarray}{
 *   k(i) &=& a^{(i)}(i), \\
 *   a^{(i-1)}(m) &=& \frac{a^{(i)}(m) + a^{(i)}(i) a^{(i)}(i-m)}{1-k^2(i)}, \\
 *   && i = M,\ldots,1
 * \f}
 * with the initial condition \f$a^{(M)}(i)=a(i)\f$ for \f$i = 1,\ldots,M\f$.
 *
 * The input can be the \f$M\f$-th order normalized generalized cepstral
 * coefficients:
 * \f[
 *   \begin{array}{cccc}
 *     K, & c'_\gamma(1), & \ldots, & c'_\gamma(M).
 *   \end{array}
 * \f]
 * In the case, the initial condition is \f$a^{(M)}(i)=\gamma \, c'_\gamma(i)\f$
 * for \f$i = 1,\ldots,M\f$.
 */
class LinearPredictiveCoefficientsToParcorCoefficients {
 public:
  /**
   * Buffer for LinearPredictiveCoefficientsToParcorCoefficients class.
   */
  class Buffer {
   public:
    Buffer() {
    }

    virtual ~Buffer() {
    }

   private:
    std::vector<double> a_;

    friend class LinearPredictiveCoefficientsToParcorCoefficients;
    DISALLOW_COPY_AND_ASSIGN(Buffer);
  };

  /**
   * @param[in] num_order Order of coefficients.
   * @param[in] gamma Gamma.
   */
  LinearPredictiveCoefficientsToParcorCoefficients(int num_order, double gamma);

  virtual ~LinearPredictiveCoefficientsToParcorCoefficients() {
  }

  /**
   * @return Order of coefficients.
   */
  int GetNumOrder() const {
    return num_order_;
  }

  /**
   * @return Gamma.
   */
  double GetGamma() const {
    return gamma_;
  }

  /**
   * @return True if this obejct is valid.
   */
  bool IsValid() const {
    return is_valid_;
  }

  /**
   * @param[in] linear_predictive_coefficients \f$M\f$-th order LPC
   *            coefficients.
   * @param[out] parcor_coefficients \f$M\f$-th order PARCOR coefficients.
   * @param[out] is_stable True if given coefficients are stable.
   * @param[out] buffer Buffer.
   * @return True on success, false on failure.
   */
  bool Run(
      const std::vector<double>& linear_predictive_coefficients,
      std::vector<double>* parcor_coefficients, bool* is_stable,
      LinearPredictiveCoefficientsToParcorCoefficients::Buffer* buffer) const;

  /**
   * @param[in,out] input_and_output \f$M\f$-th order coefficients.
   * @param[out] is_stable True if given coefficients are stable.
   * @param[out] buffer Buffer.
   * @return True on success, false on failure.
   */
  bool Run(
      std::vector<double>* input_and_output, bool* is_stable,
      LinearPredictiveCoefficientsToParcorCoefficients::Buffer* buffer) const;

 private:
  const int num_order_;
  const double gamma_;

  bool is_valid_;

  DISALLOW_COPY_AND_ASSIGN(LinearPredictiveCoefficientsToParcorCoefficients);
};

}  // namespace sptk

#endif  // SPTK_CONVERTER_LINEAR_PREDICTIVE_COEFFICIENTS_TO_PARCOR_COEFFICIENTS_H_
