/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_HSWISH_CPU_KERNEL_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_HSWISH_CPU_KERNEL_H_

#include <map>
#include <vector>
#include <utility>
#include "plugin/device/cpu/kernel/cpu_kernel.h"
#include "plugin/factory/ms_factory.h"

namespace mindspore {
namespace kernel {
class HSwishCpuKernelMod : public NativeCpuKernelMod {
 public:
  HSwishCpuKernelMod() = default;
  ~HSwishCpuKernelMod() override = default;

  bool Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
            const std::vector<KernelTensorPtr> &outputs) override;

  int Resize(
    const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
    const std::vector<KernelTensorPtr> &outputs,
    const std::map<uint32_t, tensor::TensorPtr> &inputsOnHost = std::map<uint32_t, tensor::TensorPtr>()) override;

  bool Launch(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &,
              const std::vector<KernelTensor *> &outputs) override {
    return kernel_func_(this, inputs, outputs);
  }

 protected:
  std::vector<KernelAttr> GetOpSupport() override;

 private:
  template <typename T>
  bool LaunchKernel(const std::vector<kernel::KernelTensor *> &inputs,
                    const std::vector<kernel::KernelTensor *> &outputs);
  using HSwishFunc = std::function<bool(HSwishCpuKernelMod *, const std::vector<kernel::KernelTensor *> &,
                                        const std::vector<kernel::KernelTensor *> &)>;
  static std::vector<std::pair<KernelAttr, HSwishFunc>> func_list_;
  HSwishFunc kernel_func_;
  ShapeVector x_shape_;
  uint64_t tensor_size_ = 1;
};
}  // namespace kernel
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_HSWISH_CPU_KERNEL_H_
