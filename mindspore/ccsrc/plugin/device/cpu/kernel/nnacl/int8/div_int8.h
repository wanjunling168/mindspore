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

#ifndef NNACL_INT8_DIV_INT8_H_
#define NNACL_INT8_DIV_INT8_H_

#include "nnacl/op_base.h"
#include "nnacl/errorcode.h"
#include "nnacl/int8/quantize.h"
#include "nnacl/int8/fixed_point.h"

#ifdef __cplusplus
extern "C" {
#endif
int DivInt8(const int8_t *input0_data, const int8_t *input1_data, int8_t *output_data, int64_t real_dst_count,
            const DivQuantArg *para);

int DivScalarInt8(const int8_t *input0_data, const int8_t *input1_data, int8_t *output_data, int64_t real_dst_count,
                  const DivQuantArg *para);
#ifdef __cplusplus
}
#endif

#endif  // NNACL_INT8_DIV_INT8_H_
