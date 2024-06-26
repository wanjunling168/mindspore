/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NNACL_FP16_LOG_SOFTMAX_FP16_H_
#define NNACL_FP16_LOG_SOFTMAX_FP16_H_

#include "nnacl/op_base.h"
#include "nnacl/softmax_parameter.h"
#ifdef ENABLE_NEON
#include <arm_neon.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
void LogSoftmaxLastAxisFp16(const float16_t *src, float16_t *dst, float16_t *exp_data, int batch, int channel);
void LogSoftmaxFp16(const float16_t *input_ptr, float16_t *output_ptr, float16_t *sum_data, int *input_shape, int n_dim,
                    int axis);
#ifdef __cplusplus
}
#endif

#endif  // NNACL_FP16_LOG_SOFTMAX_FP16_H_
