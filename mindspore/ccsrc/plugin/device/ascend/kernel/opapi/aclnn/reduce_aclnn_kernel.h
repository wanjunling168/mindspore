/**
 * Copyright 2023-2024 Huawei Technologies Co., Ltd
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
#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_REDUCE_ACLNN_KERNEL_MOD_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_REDUCE_ACLNN_KERNEL_MOD_H_
#include <vector>
#include <utility>
#include <string>
#include "ops/base_operator.h"
#include "plugin/device/ascend/kernel/opapi/aclnn_kernel_mod.h"
#include "transform/acl_ir/acl_convert.h"

namespace mindspore {
namespace kernel {
using TensorParams = transform::TensorParams;

class ReduceAclnnKernelMod : public AclnnKernelMod {
 public:
  explicit ReduceAclnnKernelMod(std::string &&op_type) : AclnnKernelMod(std::move(op_type)) {}
  ~ReduceAclnnKernelMod() = default;

  void GetWorkSpaceInfo(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &outputs) override;
  bool Launch(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &workspace,
              const std::vector<KernelTensor *> &outputs, void *stream_ptr) override;

  DEFINE_GET_WORKSPACE_FOR_RESIZE()

 protected:
  std::vector<int64_t> dims_{};
  bool keep_dim_{false};
};

class ReduceAllAclnnKernelMod : public ReduceAclnnKernelMod {
 public:
  ReduceAllAclnnKernelMod() : ReduceAclnnKernelMod("aclnnAll") {}

 private:
  DEFINE_GET_WORKSPACE_FOR_RESIZE()
};

class ReduceAnyAclnnKernelMod : public ReduceAclnnKernelMod {
 public:
  ReduceAnyAclnnKernelMod() : ReduceAclnnKernelMod("aclnnAny") {}

 private:
  DEFINE_GET_WORKSPACE_FOR_RESIZE()
};

class ReduceMathAclnnKernelMod : public ReduceAclnnKernelMod {
 public:
  explicit ReduceMathAclnnKernelMod(std::string &&op_type) : ReduceAclnnKernelMod(std::move(op_type)) {}
  ~ReduceMathAclnnKernelMod() = default;

  void GetWorkSpaceInfo(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &outputs) override;
  bool Launch(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &workspace,
              const std::vector<KernelTensor *> &outputs, void *stream_ptr) override;

 private:
  TypeId dtype_;
};

class ReduceSumAclnnKernelMod : public ReduceMathAclnnKernelMod {
 public:
  ReduceSumAclnnKernelMod() : ReduceMathAclnnKernelMod("aclnnReduceSum") {}
  void GetWorkSpaceInfo(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &outputs) override;
  bool Launch(const std::vector<KernelTensor *> &inputs, const std::vector<KernelTensor *> &workspace,
              const std::vector<KernelTensor *> &outputs, void *stream_ptr) override;

 private:
  DEFINE_GET_WORKSPACE_FOR_RESIZE()
  TypeId dtype_;
  bool need_skip_execute_{false};
};

class ReduceMeanAclnnKernelMod : public ReduceMathAclnnKernelMod {
 public:
  ReduceMeanAclnnKernelMod() : ReduceMathAclnnKernelMod("aclnnMean") {}

 private:
  DEFINE_GET_WORKSPACE_FOR_RESIZE()
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_REDUCE_ACLNN_KERNEL_MOD_H_
