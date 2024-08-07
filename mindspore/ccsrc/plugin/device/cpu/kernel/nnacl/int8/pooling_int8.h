/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#ifndef NNACL_INT8_POOLING_H_
#define NNACL_INT8_POOLING_H_

#ifdef ENABLE_NEON
#include <arm_neon.h>
#endif
#include "nnacl/op_base.h"
#include "nnacl/fp32/pooling_fp32.h"
#include "nnacl/kernel/pooling.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_MAXPOOL_SIZE 256

int AvgPoolingInt8(const int8_t *input_ptr, int8_t *output_ptr, const PoolingParameter *pooling_param,
                   PoolingComputeParam *compute_args, QuantArg **quant_args);

int AvgPoolingOptInt8(const int8_t *input_ptr, int8_t *output_ptr, const PoolingParameter *pooling_param,
                      PoolingComputeParam *compute_args, QuantArg **quant_args, int task_id, int thread_num);

void MaxPoolingInt8(const int8_t *input_ptr, int8_t *output_ptr, const PoolingParameter *pooling_param,
                    PoolingComputeParam *compute_args, QuantArg **quant_args);

void MaxPoolingWithQuantInt8(const int8_t *input_ptr, int8_t *output_ptr, PoolingParameter *pooling_param,
                             PoolingComputeParam *compute_args, QuantArg **quant_args, int task_id, int thread_num);

void MaxPoolingOptInt8(const int8_t *input_ptr, int8_t *output_ptr, PoolingParameter *pooling_param,
                       PoolingComputeParam *compute_args, int task_id, int thread_num);
#ifdef __cplusplus
}
#endif

#endif  // NNACL_INT8_POOLING_H_
