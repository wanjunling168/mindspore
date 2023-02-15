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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_ASCEND_BISHENG_KERNEL_FACTORY_H
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_ASCEND_BISHENG_KERNEL_FACTORY_H

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <memory>
#include "include/backend/visible.h"
#include "plugin/factory/ms_factory.h"

namespace mindspore {
namespace kernel {
class BiShengKernelMod;

class BACKEND_EXPORT BiShengKernelFactory : public Factory<BiShengKernelMod> {
 public:
  static BiShengKernelFactory &GetInstance();
};
}  // namespace kernel
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_ASCEND_BISHENG_KERNEL_FACTORY_H
