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
#ifndef MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_MAXUNPOOL2DGRAD_CPU_KERNEL_H_
#define MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_MAXUNPOOL2DGRAD_CPU_KERNEL_H_
#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include "plugin/device/cpu/kernel/cpu_kernel.h"
#include "plugin/factory/ms_factory.h"

namespace mindspore {
namespace kernel {
class MaxUnpool2DGradCpuKernelMod : public NativeCpuKernelMod {
 public:
  MaxUnpool2DGradCpuKernelMod() = default;
  ~MaxUnpool2DGradCpuKernelMod() override = default;

  bool Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
            const std::vector<KernelTensorPtr> &outputs) override;

  int Resize(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
             const std::vector<KernelTensorPtr> &outputs, const std::map<uint32_t, tensor::TensorPtr> &) override;

  bool Launch(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &,
              const std::vector<KernelTensor *> &outputs) override {
    return kernel_func_(this, inputs, outputs);
  };

 protected:
  std::vector<KernelAttr> GetOpSupport() override;

 private:
  template <typename DATA_T, typename INDICES_T>
  bool LaunchKernel(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &outputs);
  using MaxUnpool2DGradFunc =
    std::function<bool(MaxUnpool2DGradCpuKernelMod *, const std::vector<kernel::KernelTensor *> &,
                       const std::vector<kernel::KernelTensor *> &)>;
  static std::vector<std::pair<KernelAttr, MaxUnpool2DGradFunc>> func_list_;
  MaxUnpool2DGradFunc kernel_func_;

  template <typename DATA_T>
  void OutPutInitKernel(DATA_T *rawOutput, size_t length);
  ShapeVector input_shape_;
  ShapeVector grads_shape_;
  ShapeVector indices_shape_;
  ShapeVector output_shape_;
  std::string data_format_;
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_KERNEL_MAXUNPOOL2DGRAD_CPU_KERNEL_H_
