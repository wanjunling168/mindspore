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
 * limitations under the License
 */

#ifndef MINDSPORE_CORE_OPS_LUUNPACK_H_
#define MINDSPORE_CORE_OPS_LUUNPACK_H_

#include <map>
#include <vector>
#include <set>
#include <string>
#include <memory>

#include "ops/base_operator.h"
#include "mindapi/base/types.h"

namespace mindspore {
namespace ops {
constexpr auto kNameLuUnpack = "LuUnpack";
class MIND_API LuUnpack : public BaseOperator {
 public:
  MIND_API_BASE_MEMBER(LuUnpack);
  LuUnpack() : BaseOperator(kNameLuUnpack) { InitIOName({"LU_data", "LU_pivots"}, {"pivots", "L", "U"}); }
};
abstract::AbstractBasePtr LuUnpackInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                        const std::vector<abstract::AbstractBasePtr> &input_args);
using PrimLuUnpackPtr = std::shared_ptr<LuUnpack>;
}  // namespace ops
}  // namespace mindspore
#endif  // MINDSPORE_CORE_OPS_LUUNPACK_H_
