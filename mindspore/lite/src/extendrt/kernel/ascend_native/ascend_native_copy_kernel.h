/**
 * Copyright 2023 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_LITE_SRC_EXTENDRT_KERNEL_ASCEND_NATIVE_COPY_KERNEL_H_
#define MINDSPORE_LITE_SRC_EXTENDRT_KERNEL_ASCEND_NATIVE_COPY_KERNEL_H_

#include <string>
#include <vector>
#include <memory>
#include "extendrt/delegate/ascend_native/ascend_native_base_kernel.h"
#include "extendrt/delegate/ascend_native/ascend_native_impl/utils.h"
#include "extendrt/delegate/ops/copy.h"

namespace mindspore::kernel {
class AscendNativeCopyKernel : public AscendNativeBaseKernel {
 public:
  AscendNativeCopyKernel(const std::vector<InferTensor *> &inputs, const std::vector<InferTensor *> &outputs,
                         InferPrimitive prim, const InferContext *ctx, const void *stream, std::string name)
      : AscendNativeBaseKernel(inputs, outputs, prim, ctx, stream, name) {}

  int InferShape() override;

  int Prepare() override;

  int Run() override;

  int PreProcess() override;

  int PostProcess() override;

  int ReSize() override;

 private:
  ops::Copy::CopyFormatType copy_type_;
};
}  // namespace mindspore::kernel
#endif  // MINDSPORE_LITE_SRC_EXTENDRT_KERNEL_ASCEND_NATIVE_COPY_KERNEL_H_
