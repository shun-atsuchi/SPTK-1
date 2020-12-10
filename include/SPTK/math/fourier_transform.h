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

#ifndef SPTK_MATH_FOURIER_TRANSFORM_H_
#define SPTK_MATH_FOURIER_TRANSFORM_H_

#include <vector>  // std::vector

#include "SPTK/utils/sptk_utils.h"

namespace sptk {

class FourierTransform {
 public:
  class FourierTransformInterface {
   public:
    virtual ~FourierTransformInterface() {
    }

    virtual int GetLength() const = 0;

    virtual bool IsValid() const = 0;

    virtual bool Run(const std::vector<double>& real_part_input,
                     const std::vector<double>& imag_part_input,
                     std::vector<double>* real_part_output,
                     std::vector<double>* imag_part_output) const = 0;

    virtual bool Run(std::vector<double>* real_part_output,
                     std::vector<double>* imag_part_output) const = 0;
  };

  explicit FourierTransform(int length);

  ~FourierTransform() {
    delete fourier_transform_;
  }

  int GetLength() const {
    return fourier_transform_->GetLength();
  }

  bool IsValid() const {
    return fourier_transform_->IsValid();
  }

  bool Run(const std::vector<double>& real_part_input,
           const std::vector<double>& imag_part_input,
           std::vector<double>* real_part_output,
           std::vector<double>* imag_part_output) const {
    return fourier_transform_->Run(real_part_input, imag_part_input,
                                   real_part_output, imag_part_output);
  }

  bool Run(std::vector<double>* real_part,
           std::vector<double>* imag_part) const {
    return fourier_transform_->Run(real_part, imag_part);
  }

 private:
  FourierTransformInterface* fourier_transform_;

  DISALLOW_COPY_AND_ASSIGN(FourierTransform);
};

}  // namespace sptk

#endif  // SPTK_MATH_FOURIER_TRANSFORM_H_
