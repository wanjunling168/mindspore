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
#ifndef MINDSPORE_LITE_INFER_CALLBACK_H_
#define MINDSPORE_LITE_INFER_CALLBACK_H_

#include <memory>

#include "executor/kernel_exec.h"

namespace mindspore::infer::abstract {
using KernelCallBack = mindspore::lite::KernelCallBack;
// class CallBack : public std::enable_shared_from_this<CallBack> {
//  public:
//   virtual ~CallBack() = default;
// };
}  // namespace mindspore::infer::abstract

#endif  // MINDSPORE_LITE_INFER_CALLBACK_H_