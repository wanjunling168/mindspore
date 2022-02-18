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

#ifndef MINDSPORE_CCSRC_KERNEL_GPU_CUDA_IMP_BOUNDINGBOX_DECODE_IMPL_H_
#define MINDSPORE_CCSRC_KERNEL_GPU_CUDA_IMP_BOUNDINGBOX_DECODE_IMPL_H_

#include "plugin/device/gpu/hal/device/cuda_common.h"
template <typename T>
void BoundingBoxDecode(const size_t size, const T *rois, const T *deltas, T *bboxes, const float &m1, const float &m2,
                       const float &m3, const float &m4, const float &s1, const float &s2, const float &s3,
                       const float &s4, const int &max_height, const int &max_width, const float &ratio_clip,
                       cudaStream_t cuda_stream);

#endif  // MINDSPORE_CCSRC_KERNEL_GPU_CUDA_IMP_BOUNDINGBOX_DECODE_IMPL_H_
