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
#ifndef MINDSPORE_LITE_SRC_EXTENDRT_GRAPH_RUNTIME_DEFAULT_GRAPH_RUNTIME_H_
#define MINDSPORE_LITE_SRC_EXTENDRT_GRAPH_RUNTIME_DEFAULT_GRAPH_RUNTIME_H_

#include <vector>
#include <memory>

#include "infer/graph_runtime.h"

namespace mindspore {
class DefaultGraphRuntime : public mindspore::infer::abstract::GraphRuntime {
 public:
  DefaultGraphRuntime() = default;
  virtual ~DefaultGraphRuntime() = default;

  Status Prepare(std::shared_ptr<abstract::ExecutionPlan> execution_plan) override;

  Status Execute() override;

  Status Execute(const std::vector<abstract::Tensor *> &inputs, const std::vector<abstract::Tensor *> &outputs,
                 abstract::KernelCallBack before = nullptr, abstract::KernelCallBack after = nullptr) override;

 private:
  std::shared_ptr<abstract::Executor> SelectExecutor(const std::shared_ptr<abstract::ExecutionFlow> &execution_flow);

 private:
  std::shared_ptr<abstract::ExecutionPlan> execution_plan_ = nullptr;
  mindspore::HashMap<std::shared_ptr<abstract::ExecutionFlow>, std::shared_ptr<abstract::Executor>> executor_map_;
};
}  // namespace mindspore

#endif  // MINDSPORE_LITE_SRC_EXTENDRT_GRAPH_RUNTIME_DEFAULT_GRAPH_RUNTIME_H_