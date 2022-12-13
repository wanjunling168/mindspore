/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CORE_OPS_FILL_DIAGONAL_H_
#define MINDSPORE_CORE_OPS_FILL_DIAGONAL_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ops/base_operator.h"
#include "mindapi/base/types.h"

namespace mindspore {
namespace ops {
constexpr auto kNameFillDiagonal = "FillDiagonal";
/// \brief Fill the main diagonal of a tensor that has at least 2-dimensions.
/// Refer to Python API @ref mindspore.ops.FillDiagonal for more details.
class MIND_API FillDiagonal : public BaseOperator {
 public:
  MIND_API_BASE_MEMBER(FillDiagonal);
  /// \brief Constructor.
  FillDiagonal() : BaseOperator(kNameFillDiagonal) { InitIOName({"input_x"}, {"y"}); }

  /// \brief Init.
  void Init(const float fill_value = 0.0, const bool wrap = false);
  /// \brief Set fill_value & wrap.
  void set_fill_value(const float fill_value);
  void set_wrap(const bool wrap);

  /// \brief Get fill_value & wrap.
  float get_fill_value() const;
  bool get_wrap() const;
};

MIND_API abstract::AbstractBasePtr FillDiagonalInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                                     const std::vector<abstract::AbstractBasePtr> &input_args);
}  // namespace ops
}  // namespace mindspore

#endif  // MINDSPORE_CORE_OPS_FILL_DIAGONAL_H_
