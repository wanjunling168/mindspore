/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "src/ops/batch_norm.h"

namespace mindspore {
namespace lite {
#ifdef PRIMITIVE_WRITEABLE
float BatchNorm::GetEpsilon() const { return this->primitive_->value.AsBatchNorm()->epsilon; }

void BatchNorm::SetEpsilon(float epsilon) { this->primitive_->value.AsBatchNorm()->epsilon = epsilon; }

#else

float BatchNorm::GetEpsilon() const { return this->primitive_->value_as_BatchNorm()->epsilon(); }

void BatchNorm::SetEpsilon(float epsilon) {}
#endif
}  // namespace lite
}  // namespace mindspore
