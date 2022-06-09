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

#ifndef MINDSPORE_CCSRC_PLUGIN_DEVICE_GPU_KERNEL_NN_COL2IM_GPU_KERNEL_H_
#define MINDSPORE_CCSRC_PLUGIN_DEVICE_GPU_KERNEL_NN_COL2IM_GPU_KERNEL_H_

#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <utility>
#include "plugin/device/gpu/kernel/gpu_kernel.h"
#include "plugin/device/gpu/kernel/gpu_kernel_factory.h"
#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/pad_impl.cuh"
#include "plugin/device/gpu/kernel/kernel_constants.h"

namespace mindspore {
namespace kernel {
class Col2ImFwdGpuKernelMod : public NativeGpuKernelMod {
 public:
  Col2ImFwdGpuKernelMod() { ResetResource(); }
  ~Col2ImFwdGpuKernelMod() = default;

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
              const std::vector<AddressPtr> &outputs, void *stream_ptr) override {
    if (is_null_input_) {
      return true;
    }
    cuda_stream_ = stream_ptr;
    return kernel_func_(this, inputs, workspace, outputs);
  }

  bool Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
            const std::vector<KernelTensorPtr> &outputs) override;

  int Resize(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
             const std::vector<KernelTensorPtr> &outputs, const std::map<uint32_t, tensor::TensorPtr> &) override;
  std::vector<KernelAttr> GetOpSupport() override;

 protected:
  void ResetResource() noexcept;

  template <typename T>
  bool LaunchKernel(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
                    const std::vector<AddressPtr> &outputs);
  using Col2ImFunc =
    std::function<bool(Col2ImFwdGpuKernelMod *, const std::vector<kernel::AddressPtr> &,
                       const std::vector<kernel::AddressPtr> &, const std::vector<kernel::AddressPtr> &)>;

 private:
  size_t batch_size_{0};
  size_t channels_{0};
  size_t out_height_{0};
  size_t out_width_{0};
  size_t in_height_{0};
  size_t in_width_{0};
  size_t pad_width_{0};
  size_t pad_height_{0};
  size_t kernel_height_{0};
  size_t kernel_width_{0};
  size_t stride_height_{0};
  size_t stride_width_{0};
  size_t dilation_height_{0};
  size_t dilation_width_{0};
  bool is_null_input_{false};
  void *cuda_stream_{nullptr};
  Col2ImFunc kernel_func_{};
  static std::vector<std::pair<KernelAttr, Col2ImFunc>> func_list_;
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PLUGIN_DEVICE_GPU_KERNEL_NN_COL2IM_GPU_KERNEL_H_
