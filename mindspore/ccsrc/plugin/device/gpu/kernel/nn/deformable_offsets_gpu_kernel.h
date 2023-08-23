/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_NN_DEFORMABLE_OFFSET_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_NN_DEFORMABLE_OFFSET_H_

#include <vector>
#include <string>
#include <functional>
#include <map>
#include <utility>
#include <memory>
#include "ops/deformable_offsets.h"
#include "plugin/device/gpu/kernel/gpu_kernel.h"
#include "plugin/device/gpu/kernel/gpu_kernel_factory.h"

namespace mindspore {
namespace kernel {
class DeformableOffsetsGpuKernelMod : public NativeGpuKernelMod {
 public:
  DeformableOffsetsGpuKernelMod() {}
  ~DeformableOffsetsGpuKernelMod() override {}

  bool Launch(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &workspace,
              const std::vector<KernelTensor *> &outputs, void *stream_ptr) override;
  bool Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
            const std::vector<KernelTensorPtr> &outputs) override;
  std::vector<KernelAttr> GetOpSupport() override;

 private:
  using LaunchKernelFunc = std::function<bool(
    DeformableOffsetsGpuKernelMod *, const std::vector<kernel::KernelTensor *> &,
    const std::vector<kernel::KernelTensor *> &, const std::vector<kernel::KernelTensor *> &, void *)>;
  static std::vector<std::pair<KernelAttr, LaunchKernelFunc>> func_list_;
  template <class T>
  bool LaunchKernel(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &workspace,
                    const std::vector<KernelTensor *> &outputs, void *stream_ptr);
  bool CheckParam(const std::shared_ptr<ops::DeformableOffsets> &kernel);

  int Resize(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
             const std::vector<KernelTensorPtr> &outputs,
             const std::map<uint32_t, tensor::TensorPtr> &others = std::map<uint32_t, tensor::TensorPtr>()) override;

  // attrs
  std::vector<uint32_t> strides_;
  std::vector<uint32_t> pads_;
  std::vector<uint32_t> kernel_size_;
  std::vector<uint32_t> dilations_;
  std::string data_format_;
  uint32_t deformable_groups_;
  bool modulated_;

  // Constant value
  LaunchKernelFunc kernel_func_{};
  // axis
  size_t n_axis_;
  size_t c_axis_;
  size_t h_axis_;
  size_t w_axis_;

  // Dynamic value
  uint32_t position_grid_num_;
  // x shape
  uint32_t n_;
  uint32_t c_;
  uint32_t x_h_;
  uint32_t x_w_;
  // output shape
  uint32_t output_h_;
  uint32_t output_w_;
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_NN_DEFORMABLE_OFFSET_H_
