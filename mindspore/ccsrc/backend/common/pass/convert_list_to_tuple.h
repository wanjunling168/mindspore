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
#ifndef MINDSPORE_CCSRC_BACKEND_OPTIMIZER_PASS_CONVERT_LIST_TO_TUPLE_H_
#define MINDSPORE_CCSRC_BACKEND_OPTIMIZER_PASS_CONVERT_LIST_TO_TUPLE_H_

#include <memory>
#include <string>
#include <vector>
#include "backend/common/optimizer/optimizer.h"

namespace mindspore {
namespace opt {
class BACKEND_EXPORT ConvertListToTuple : public PatternProcessPass {
 public:
  explicit ConvertListToTuple(bool multigraph = true) : PatternProcessPass("convert_list_to_tuple", multigraph) {}
  ~ConvertListToTuple() override = default;
  const AnfNodePtr Process(const FuncGraphPtr &, const AnfNodePtr &node, const EquivPtr &) const override;

 private:
  // ValueSequence --> ValueTuple.
  ValuePtr ConvertValueSequenceToValueTuple(const ValuePtr &value, bool *need_convert, size_t depth = 0) const;
  // AbstractSequence --> AbstractTuple.
  AbstractBasePtr ConvertSequenceAbsToTupleAbs(const AbstractBasePtr &abs, size_t depth = 0) const;
};
}  // namespace opt
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_OPTIMIZER_PASS_CONVERT_LIST_TO_TUPLE_H_
