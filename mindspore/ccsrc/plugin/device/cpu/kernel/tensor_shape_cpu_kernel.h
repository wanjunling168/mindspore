/**
 * Copyright 2021-2023 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_TENSOR_SHAPE_CPU_KERNEL_H_
#define MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_TENSOR_SHAPE_CPU_KERNEL_H_

#include <map>
#include <vector>
#include "plugin/device/cpu/kernel/cpu_kernel.h"

namespace mindspore::kernel {
class TensorShapeCpuKernelMod : public NativeCpuKernelMod {
 public:
  TensorShapeCpuKernelMod() = default;
  ~TensorShapeCpuKernelMod() override = default;

  bool Init(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &outputs) override;
  int Resize(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &outputs) override;
  bool Launch(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &workspace,
              const std::vector<KernelTensor *> &outputs) override;
  std::vector<KernelAttr> GetOpSupport() override;
  std::vector<size_t> GetLaunchIgnoredInputAddressIdx() const override { return {kIndex0}; }

 private:
  ShapeVector input_shape_;
  ShapeVector output_shape_;
};
}  // namespace mindspore::kernel

#endif  // MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_TENSOR_SHAPE_CPU_KERNEL_H_
