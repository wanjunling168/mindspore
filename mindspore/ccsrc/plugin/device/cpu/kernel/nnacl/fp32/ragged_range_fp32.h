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
#ifndef NNACL_FP32_RAGGED_RANGE_FP32_H_
#define NNACL_FP32_RAGGED_RANGE_FP32_H_

#include "nnacl/kernel/ragged_range.h"

void RaggedRangeFp32(const float *starts, const float *limits, const float *deltas, int *splits, float *value,
                     RaggedRangeStruct *ragged_range);
void RaggedRangeInt(const int *starts, const int *limits, const int *deltas, int *splits, int *value,
                    RaggedRangeStruct *ragged_range);

#endif  // NNACL_FP32_RAGGED_RANGE_FP32_H_
