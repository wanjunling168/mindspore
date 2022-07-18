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

#include "plugin/device/cpu/kernel/uniform_candidate_sampler_cpu_kernel.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <string>
#include <random>
#include <memory>
#include <functional>
#include <limits>
#include "plugin/device/cpu/hal/device/cpu_device_address.h"
#include "abstract/utils.h"
#include "mindspore/core/ops/uniform_candidate_sampler.h"
#include "mindspore/core/ops/core_ops.h"
namespace mindspore {
namespace kernel {
namespace {
template <typename S>
S Probability(int64_t range_max) {
  if (range_max == 0) {
    return S(0);
  }
  return static_cast<S>(1.0f / range_max);
}

template <typename S>
S ApproximateExpectedCount(S p, int64_t sampled_size, int64_t counter) {
  if (sampled_size == counter) {
    return p * sampled_size;
  }
  return -std::expm1(counter * std::log1p(-p));  // 1 - (1-p)^counter, expm1 := exp(x)-1, log1p := log(x+1)
}

const size_t kInputsNum = 1;
const size_t kOutputsNum = 3;
const size_t kInputRank = 2;
}  // namespace

template <typename T>
int64_t UniformCandidateSamplerCpuKernelMod::Sampling(T *sampled_candidates_, const size_t length) {
  int64_t counter = 0;
  if (length != static_cast<size_t>(num_sampled_ * sizeof(T))) {
    return 0;
  }
  // pick between [0, range_max_-1]
  T range = static_cast<T>(range_max_);
  std::uniform_int_distribution<T> distribution(0, range - 1);
  if (!unique_) {
    auto task = [this, &sampled_candidates_, &distribution](size_t start, size_t end) {
      for (size_t i = start; i < end; i++) {
        sampled_candidates_[i] = distribution(generator_);
      }
    };
    ParallelLaunchAutoSearch(task, num_sampled_, this, &parallel_search_info_, pool_);
    counter = num_sampled_;
    std::ostringstream oss;
    for (int64_t i = 0; i < num_sampled_; i++) {
      oss << sampled_candidates_[i] << ", ";
    }
    MS_LOG(DEBUG) << "For UniformCandidateSampler, sampled_candidates: " << oss.str();
    return counter;
  }

  int64_t picked = 0;
  std::unordered_set<T> set_container;
  while (picked < num_sampled_) {
    T tmp = distribution(generator_);
    counter++;
    if ((set_container.find(tmp) == set_container.end()) &&
        ((!remove_accidental_hits_) || set_input_.find(tmp) == set_input_.end())) {
      set_container.insert(tmp);
      sampled_candidates_[picked] = tmp;
      picked++;
    }
  }
  std::ostringstream oss;
  for (int64_t i = 0; i < num_sampled_; i++) {
    oss << sampled_candidates_[i] << ", ";
  }
  MS_LOG(DEBUG) << "For UniformCandidateSampler, sampled_candidates: " << oss.str();
  return counter;
}

template <typename S>
void UniformCandidateSamplerCpuKernelMod::ExpectedLanuch(const int64_t counter, S *true_expected_count,
                                                         S *sampled_expected_count) {
  S prob = Probability<S>(range_max_);
  S value = ApproximateExpectedCount(prob, num_sampled_, counter);
  MS_LOG(DEBUG) << "output val: " << value;
  auto task1 = [this, &true_expected_count, &value](size_t start, size_t end) {
    for (size_t i = start; i < end; i++) {
      true_expected_count[i] = value;
    }
  };
  ParallelLaunchAutoSearch(task1, input_size_, this, &parallel_search_info_, pool_);

  auto task2 = [this, &sampled_expected_count, &value](size_t start, size_t end) {
    for (size_t i = start; i < end; i++) {
      sampled_expected_count[i] = value;
    }
  };
  ParallelLaunchAutoSearch(task2, num_sampled_, this, &parallel_search_info_, pool_);
}

bool UniformCandidateSamplerCpuKernelMod::CheckAttribute() {
  // check attrs
  if (num_true_ <= 0 || num_sampled_ <= 0 || range_max_ <= 0) {
    MS_EXCEPTION(ValueError) << "For 'UniformCandidateSampler', the parameters must be larger than 0, but got "
                             << "'num_true' = " << num_true_ << ", 'num_sampled' = " << num_sampled_
                             << ", 'range_max' = " << range_max_;
  }

  if (unique_ && (num_sampled_ > range_max_)) {
    MS_EXCEPTION(ValueError) << "For 'UniformCandidateSampler', the parameter 'num_sampled' can not be larger than"
                             << "'range_max', but got 'num_sampled' = " << num_sampled_
                             << ", 'range_max' = " << range_max_;
  }
  return true;
}

bool UniformCandidateSamplerCpuKernelMod::CheckInputsAndOutputs(const std::vector<KernelTensorPtr> &inputs,
                                                                const std::vector<KernelTensorPtr> &outputs) {
  if (inputs.empty() || outputs.empty()) {
    MS_LOG(EXCEPTION) << "For 'UniformCandidateSampler', inputs or outputs can not be empty.";
  }

  if (inputs.size() != kInputsNum || outputs.size() != kOutputsNum) {
    MS_EXCEPTION(ValueError) << "For 'UniformCandidateSampler', the sizes of inputs and outputs must be " << kInputsNum
                             << " and " << kOutputsNum << ", but got inputs' size : " << inputs.size()
                             << ", outputs' size: " << outputs.size();
  }
  auto input_shape = inputs.at(kIndex0)->GetShapeVector();
  auto input_rank = static_cast<size_t>(batch_rank_ + kInputRank);
  if (input_shape.size() != input_rank) {
    MS_EXCEPTION(ValueError) << "For 'UniformCandidateSampler', the dimension of input 'true_classes' must be "
                             << input_rank << ", but got " << input_shape.size();
  }
  auto kindex = static_cast<size_t>(batch_rank_ + kIndex1);
  if (input_shape[kindex] != num_true_) {
    MS_EXCEPTION(ValueError) << "For 'UniformCandidateSampler', the input 'true_classes' must have 'num_true' columns, "
                             << "but got 'true_classes': (" << input_shape[0] << ", " << input_shape[1] << ")"
                             << "'num_true': " << num_true_;
  }

  auto output_kIndex0_type = outputs.at(kIndex0)->GetDtype();
  if (output_kIndex0_type == kNumberTypeInt32) {
    if (range_max_ > static_cast<int64_t>(std::numeric_limits<int>::max())) {
      MS_EXCEPTION(ValueError) << "For '" << kernel_name_ << "', 'range_max' can not exceed the range of int32, but "
                               << "got" << range_max_ << ". The input data type should be changed to int64.";
    }
  }

  if (std::any_of(input_shape.begin(), input_shape.end(), [](size_t i) { return i == 0; })) {
    is_null_input_ = true;
  }

  if (is_null_input_) {
    return true;
  }
  auto output_shape = outputs.at(kIndex0)->GetShapeVector();

  batch_size_ = std::accumulate(output_shape.begin(), output_shape.end(), 1, std::multiplies<int64_t>());
  batch_size_ = batch_size_ / num_sampled_;
  if (batch_size_ == 0) {
    MS_EXCEPTION(ValueError) << "For '" << kernel_name_ << "', the shape of output 'sampled_candidates' can not be 0";
  }
  input_size_ = std::accumulate(input_shape.begin(), input_shape.end(), 1, std::multiplies<int64_t>());
  input_size_ = input_size_ / batch_size_;

  output_sizes_.emplace_back(num_sampled_);
  output_sizes_.emplace_back(input_size_);
  output_sizes_.emplace_back(num_sampled_);

  return true;
}

bool UniformCandidateSamplerCpuKernelMod::Init(const BaseOperatorPtr &base_operator,
                                               const std::vector<KernelTensorPtr> &inputs,
                                               const std::vector<KernelTensorPtr> &outputs) {
  if (!base_operator) {
    MS_LOG(ERROR) << "UniformCandiadataSampler ops is null.";
    return false;
  }
  kernel_name_ = base_operator->name();
  batch_rank_ = base_operator->get_batch_rank();
  // getting attrs
  if (kernel_name_ == prim::kPrimUniformCandidateSampler->name()) {
    auto kernel_ptr_ = std::make_shared<ops::UniformCandidateSampler>(base_operator->GetPrim());
    num_true_ = kernel_ptr_->get_num_true();
    num_sampled_ = kernel_ptr_->get_num_sampled();
    unique_ = kernel_ptr_->get_unique();
    range_max_ = kernel_ptr_->get_range_max();
    init_seed_ = kernel_ptr_->get_seed();
    if (init_seed_ == 0) {
      cur_seed_ = time(nullptr);
      generator_.seed(cur_seed_);
    } else {
      generator_.seed(init_seed_);
    }
    remove_accidental_hits_ = kernel_ptr_->get_remove_accidental_hits();
  } else {
    MS_LOG(EXCEPTION) << "For UniformCandidateSamplerCpuKernelMod, it's name must be UniformCandidateSampler, but got "
                      << "invalid kernel name " << prim::kPrimUniformCandidateSampler->name();
  }

  (void)CheckAttribute();
  (void)CheckInputsAndOutputs(inputs, outputs);

  if (!MatchKernelFunc(base_operator, inputs, outputs)) {
    return false;
  }
  return true;
}

int UniformCandidateSamplerCpuKernelMod::Resize(const BaseOperatorPtr &base_operator,
                                                const std::vector<KernelTensorPtr> &inputs,
                                                const std::vector<KernelTensorPtr> &outputs,
                                                const std::map<uint32_t, tensor::TensorPtr> &) {
  int ret = KernelMod::Resize(base_operator, inputs, outputs);
  if (ret != KRET_OK) {
    return ret;
  }
  auto output_shape = outputs.at(kIndex0)->GetShapeVector();

  batch_size_ = std::accumulate(output_shape.begin(), output_shape.end(), 1, std::multiplies<int64_t>());
  batch_size_ = batch_size_ / num_sampled_;
  if (batch_size_ == 0) {
    MS_EXCEPTION(ValueError) << "For '" << kernel_name_ << "', the shape of output 'sampled_candidates' can not be 0";
  }

  auto input_shape = inputs.at(kIndex0)->GetShapeVector();
  input_size_ = std::accumulate(input_shape.begin(), input_shape.end(), 1, std::multiplies<int64_t>());
  input_size_ = input_size_ / batch_size_;

  output_sizes_.emplace_back(num_sampled_);
  output_sizes_.emplace_back(input_size_);
  output_sizes_.emplace_back(num_sampled_);
  return KRET_OK;
}

template <typename T, typename S>
bool UniformCandidateSamplerCpuKernelMod::LaunchKernel(const std::vector<AddressPtr> &inputs,
                                                       const std::vector<AddressPtr> &workspaces,
                                                       const std::vector<AddressPtr> &outputs) {
  if (is_null_input_) {
    MS_LOG(WARNING) << "For 'UniformCandidateSampler', the input 'true_classes' was empty.";
    return true;
  }
  if (init_seed_ == 0 && cur_seed_ == 0) {
    cur_seed_ = time(nullptr);
    generator_.seed(cur_seed_);
  } else if (init_seed_ != 0) {
    generator_.seed(init_seed_);
  }
  MS_LOG(DEBUG) << "For UniformCandidateSampler, generator seed : init_seed_ = " << init_seed_
                << "cur_seed_ = " << cur_seed_;
  T *sampled_candidates = GetDeviceAddress<T>(outputs, kIndex0);
  S *true_expected_count = GetDeviceAddress<S>(outputs, kIndex1);
  S *sampled_expected_count = GetDeviceAddress<S>(outputs, kIndex2);
  T *input = GetDeviceAddress<T>(inputs, kIndex0);

  for (int64_t j = 0; j < batch_size_; ++j) {
    if (remove_accidental_hits_) {
      for (size_t i = 0; i < input_size_; i++) {
        set_input_.insert(static_cast<int64_t>(input[i]));
      }
      if (num_sampled_ + static_cast<int64_t>(set_input_.size()) > range_max_) {
        MS_LOG(WARNING) << "For 'UniformCandidateSampler', the parameter 'range_max' can not be less than the sum of "
                        << "'num_sampled' and the num of unrepeat elements of input 'true_classes', "
                        << " set remove_accidental_hits = false.";
        remove_accidental_hits_ = false;
      }
    }
    size_t sampled_candidate_size = static_cast<size_t>(num_sampled_ * sizeof(T));
    int64_t counter = Sampling<T>(sampled_candidates, sampled_candidate_size);
    ExpectedLanuch<S>(counter, true_expected_count, sampled_expected_count);

    input = input + input_size_;
    sampled_candidates = sampled_candidates + output_sizes_[kIndex0];
    true_expected_count = true_expected_count + output_sizes_[kIndex1];
    sampled_expected_count = sampled_expected_count + output_sizes_[kIndex2];
  }
  return true;
}

using USCKernelRunFunc = UniformCandidateSamplerCpuKernelMod::KernelRunFunc;
const std::vector<std::pair<KernelAttr, USCKernelRunFunc>> &UniformCandidateSamplerCpuKernelMod::GetFuncList() const {
  static const std::vector<std::pair<KernelAttr, USCKernelRunFunc>> func_list = {
    {KernelAttr()
       .AddInputAttr(kNumberTypeInt32)
       .AddOutputAttr(kNumberTypeInt32)
       .AddOutputAttr(kNumberTypeFloat32)
       .AddOutputAttr(kNumberTypeFloat32),
     &UniformCandidateSamplerCpuKernelMod::LaunchKernel<int32_t, float>},
    {KernelAttr()
       .AddInputAttr(kNumberTypeInt64)
       .AddOutputAttr(kNumberTypeInt64)
       .AddOutputAttr(kNumberTypeFloat32)
       .AddOutputAttr(kNumberTypeFloat32),
     &UniformCandidateSamplerCpuKernelMod::LaunchKernel<int64_t, float>}};
  return func_list;
}

MS_KERNEL_FACTORY_REG(NativeCpuKernelMod, UniformCandidateSampler, UniformCandidateSamplerCpuKernelMod);
}  // namespace kernel
}  // namespace mindspore
