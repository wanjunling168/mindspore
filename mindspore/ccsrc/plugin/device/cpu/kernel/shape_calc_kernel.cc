/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#include "plugin/device/cpu/kernel/shape_calc_kernel.h"
#include <functional>
#include "plugin/device/cpu/hal/device/cpu_device_address.h"

namespace mindspore {
namespace kernel {
bool ShapeCalcCpuKernelMod::Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
                                 const std::vector<KernelTensorPtr> &outputs) {
  MS_EXCEPTION_IF_NULL(base_operator);
  auto operator_ptr = std::dynamic_pointer_cast<ops::ShapeCalc>(base_operator);
  if (!operator_ptr) {
    MS_LOG(ERROR) << "cast ShapeCalc ops failed!";
    return false;
  }
  kernel_name_ = operator_ptr->name();
  return true;
}

int ShapeCalcCpuKernelMod::Resize(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
                                  const std::vector<KernelTensorPtr> &outputs,
                                  const std::map<uint32_t, tensor::TensorPtr> &) {
  auto ret = KernelMod::Resize(base_operator, inputs, outputs);
  if (ret != KRET_OK) {
    return ret;
  }
  auto operator_ptr = std::dynamic_pointer_cast<ops::ShapeCalc>(base_operator);
  if (!operator_ptr) {
    MS_LOG(ERROR) << "cast ShapeCalc ops failed!";
    return KRET_RESIZE_FAILED;
  }
  outs_shape_ = operator_ptr->get_calc_result();
  return ret;
}

bool ShapeCalcCpuKernelMod::Launch(const std::vector<kernel::KernelTensor *> &inputs,
                                   const std::vector<kernel::KernelTensor *> &,
                                   const std::vector<kernel::KernelTensor *> &outputs) {
  if (functor_ == nullptr) {
    MS_LOG(ERROR) << "For '" << kernel_name_ << "', shape func pointer is nullptr";
    return false;
  }
  if (inputs.size() != inputs_size_.size() || inputs.size() != inputs_type_.size()) {
    MS_LOG(ERROR)
      << "For '" << kernel_name_
      << "', inputs address list size must be equal to inputs shape list size and inputs dtype list size, but got "
      << inputs.size() << " vs " << inputs_size_.size() << " vs " << inputs_type_.size();
    return false;
  }

  ShapeArray args;
  args.reserve(inputs.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (inputs_type_.at(i) == kNumberTypeInt32) {
      auto input_addr = reinterpret_cast<int32_t *>(inputs[i]->device_ptr());
      (void)args.emplace_back(input_addr, input_addr + inputs_size_[i]);
    } else {
      auto input_addr = reinterpret_cast<int64_t *>(inputs[i]->device_ptr());
      (void)args.emplace_back(input_addr, input_addr + inputs_size_[i]);
    }
  }
  outs_shape_ = functor_->Calc(args);
  if (outputs.size() != outs_shape_.size()) {
    MS_LOG(ERROR) << "For '" << kernel_name_
                  << "', outputs address list size must be equal to the number of outputs of shape func, but got "
                  << outputs.size() << " vs " << outs_shape_.size();
    return false;
  }

  for (size_t i = 0; i < outputs.size(); ++i) {
    auto output_addr = reinterpret_cast<int64_t *>(outputs[i]->device_ptr());
    for (size_t j = 0; j < outs_shape_[i].size(); ++j) {
      output_addr[j] = outs_shape_[i][j];
    }
  }
  return true;
}

std::vector<KernelAttr> ShapeCalcCpuKernelMod::GetOpSupport() {
  static std::vector<KernelAttr> support_list = {KernelAttr().AddSkipCheckAttr(true).AddRealTuple(true)};
  return support_list;
}

std::vector<size_t> ShapeCalcCpuKernelMod::GetLaunchIgnoredInputAddressIdx() const {
  std::vector<size_t> ignored_idx(inputs_.size());
  std::iota(ignored_idx.begin(), ignored_idx.end(), kIndex0);
  return ignored_idx;
}

MS_KERNEL_FACTORY_REG(NativeCpuKernelMod, ShapeCalc, ShapeCalcCpuKernelMod);
}  // namespace kernel
}  // namespace mindspore
