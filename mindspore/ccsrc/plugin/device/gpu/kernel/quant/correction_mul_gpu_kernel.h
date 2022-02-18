/**
 * Copyright 2020-2022 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_CORRECTIONMUL_GPUKERNEL_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_CORRECTIONMUL_GPUKERNEL_H_

#include <vector>
#include "plugin/device/gpu/kernel/gpu_kernel.h"
#include "plugin/device/gpu/kernel/gpu_kernel_factory.h"
#include "plugin/device/gpu/kernel/cuda_impl/correction_mul_impl.cuh"

namespace mindspore {
namespace kernel {
template <typename T>
class CorrectionMulGpuKernelMod : public NativeGpuKernelMod {
 public:
  CorrectionMulGpuKernelMod() : is_null_input_(false), batch_size_(0), channel_(0), height_(0), width_(0) {}
  ~CorrectionMulGpuKernelMod() override { DestroyResource(); }

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &,
              const std::vector<AddressPtr> &outputs, void *stream_ptr) override {
    if (is_null_input_) {
      return true;
    }
    auto *weight = GetDeviceAddress<T>(inputs, 0);
    auto *gamma = GetDeviceAddress<T>(inputs, 1);
    auto *running_std = GetDeviceAddress<T>(inputs, 2);
    auto *output = GetDeviceAddress<T>(outputs, 0);

    CalCorrectionMul(weight, gamma, running_std, batch_size_, channel_, height_, width_, output,
                     reinterpret_cast<cudaStream_t>(stream_ptr));
    return true;
  }
  bool Init(const CNodePtr &kernel_node) override {
    auto kernel_name = AnfAlgo::GetCNodeName(kernel_node);
    InitResource();

    size_t input_num = AnfAlgo::GetInputTensorNum(kernel_node);
    if (input_num != 3) {
      MS_LOG(EXCEPTION) << "For '" << kernel_name << "', the number of inputs should be 3, but got " << input_num;
    }

    auto input_shape = AnfAlgo::GetPrevNodeOutputInferShape(kernel_node, 0);
    is_null_input_ = CHECK_SHAPE_NULL(input_shape, kernel_name, "input");
    if (is_null_input_) {
      InitSizeLists();
      return true;
    }
    if (input_shape.size() != 4) {
      MS_LOG(EXCEPTION) << "For '" << kernel_name << "', the dimension of input should be 4, but got "
                        << input_shape.size();
    }
    batch_size_ = input_shape[0];
    channel_ = input_shape[1];
    height_ = input_shape[2];
    width_ = input_shape[3];

    InitSizeLists();
    return true;
  }

 protected:
  void InitSizeLists() override {
    size_t input_size = batch_size_ * channel_ * height_ * width_ * sizeof(T);
    size_t weight_size = batch_size_ * sizeof(T);
    input_size_list_.push_back(input_size);   // weight
    input_size_list_.push_back(weight_size);  // gamma
    input_size_list_.push_back(weight_size);  // running_std
    output_size_list_.push_back(input_size);
  }

  void InitResource() override {}

 private:
  bool is_null_input_;
  size_t batch_size_;
  size_t channel_;
  size_t height_;
  size_t width_;
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_CORRECTIONMUL_GPUKERNEL_H_
