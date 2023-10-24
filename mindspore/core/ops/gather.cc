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

#include "ops/gather.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>

#include "abstract/abstract_value.h"
#include "abstract/dshape.h"
#include "abstract/ops/op_infer.h"
#include "abstract/ops/primitive_infer_map.h"
#include "abstract/utils.h"
#include "base/base.h"
#include "ir/anf.h"
#include "ir/dtype.h"
#include "ir/dtype/number.h"
#include "ir/primitive.h"
#include "ir/tensor.h"
#include "ir/value.h"
#include "mindapi/base/shape_vector.h"
#include "mindapi/src/helper.h"
#include "mindspore/core/ops/array_ops.h"
#include "mindspore/core/ops/op_utils.h"
#include "ops/op_name.h"
#include "ops/primitive_c.h"
#include "utils/check_convert_utils.h"
#include "utils/log_adapter.h"
#include "utils/shape_utils.h"

namespace mindspore {
namespace ops {
void CheckBatchDims(int64_t batch_dims, int64_t axis_val, const ShapeVector &params_shp, const ShapeVector &indices_shp,
                    const std::string &op_name) {
  int64_t params_rank = static_cast<int64_t>(params_shp.size());
  int64_t indices_rank = static_cast<int64_t>(indices_shp.size());
  if (batch_dims < -indices_rank || batch_dims > indices_rank) {
    MS_LOG(EXCEPTION) << "For '" << op_name << "', batch_dims must be in [" << -indices_rank << ", " << indices_rank
                      << "], but got batch_dims: " << batch_dims;
  }
  if (batch_dims < 0) {
    batch_dims += indices_rank;
  }
  if (batch_dims > params_rank) {
    MS_LOG(EXCEPTION) << "For '" << op_name
                      << "', batch_dims must be less than params's rank, but got batch_dims: " << batch_dims
                      << ", oarams's rank: " << params_rank;
  }
  if (axis_val < batch_dims) {
    MS_LOG(EXCEPTION) << "For '" << op_name
                      << "', batch_dims must be less than or equal to axis, but got batch_dims: " << batch_dims
                      << ", axis: " << axis_val;
  }
  for (size_t i = 0; i < LongToSize(batch_dims); i++) {
    if (params_shp[i] != indices_shp[i]) {
      MS_LOG(EXCEPTION) << "For '" << op_name << "', params.shape[" << i << "] should be equal to indices.shape[" << i
                        << "] but got param.shape: " << params_shp << ", indices.shape: " << indices_shp;
    }
  }
}

ShapeVector CalcuateGatherWithBatchDimsOutputShape(int64_t batch_dims, int64_t axis_val, const ShapeVector &ind_vec,
                                                   const ShapeVector &params_vec) {
  if (batch_dims < 0) {
    batch_dims += SizeToLong(ind_vec.size());
  }
  ShapeVector out_vec;
  for (size_t i = 0; i < LongToSize(axis_val); i++) {
    out_vec.push_back(params_vec[i]);
  }
  for (size_t i = LongToSize(batch_dims); i < ind_vec.size(); i++) {
    out_vec.push_back(ind_vec[i]);
  }
  for (size_t i = LongToSize(axis_val) + 1; i < params_vec.size(); i++) {
    out_vec.push_back(params_vec[i]);
  }
  return out_vec;
}

abstract::ShapePtr GatherInferShape(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  const int64_t input_num = 3;
  const std::string &op_name = primitive->name();
  CheckAndConvertUtils::CheckInputArgs(input_args, kEqual, input_num, op_name);
  auto params_shape_ptr = input_args[kInputIndex0]->GetShape();
  auto indices_shape_ptr = input_args[kInputIndex1]->GetShape();
  // Dynamic rank.
  if (params_shape_ptr->IsDimUnknown() || indices_shape_ptr->IsDimUnknown()) {
    return std::make_shared<abstract::Shape>(ShapeVector{abstract::Shape::kShapeRankAny});
  }
  auto indices = CheckAndConvertUtils::CheckArgsType(op_name, input_args, 1, kObjectTypeTensorType);
  auto params = CheckAndConvertUtils::CheckArgsType(op_name, input_args, 0, kObjectTypeTensorType);
  int64_t axis_val = 0;
  bool is_axis_dyn = false;
  // 3rd input is a Tensor when Gather is a dynamic shape operator
  if (CheckAndConvertUtils::IsTensor(input_args[kInputIndex2])) {
    auto axis_value_ptr = input_args[kInputIndex2]->GetValue();
    MS_EXCEPTION_IF_NULL(axis_value_ptr);
    auto axis_type_ptr = input_args[kInputIndex2]->GetType();
    if (axis_value_ptr->isa<tensor::Tensor>()) {
      auto axis_vec = CheckAndConvertUtils::CheckTensorIntValue("axis", axis_value_ptr, op_name, axis_type_ptr);
      if (axis_vec.size() != 1) {
        MS_EXCEPTION(ValueError) << " The input size of Gather axis must be 1, but got " << axis_vec.size();
      }
      axis_val = axis_vec[0];
    } else {
      is_axis_dyn = true;
    }
  } else if (CheckAndConvertUtils::IsScalar(input_args[kInputIndex2])) {
    auto axis_value = input_args[kInputIndex2]->GetValue();
    if (axis_value->ContainsValueAny()) {
      is_axis_dyn = true;
    } else {
      auto axis_val_opt = GetScalarValue<int64_t>(axis_value);
      axis_val = axis_val_opt.value();
    }
  } else {
    MS_LOG(EXCEPTION) << "For '" << primitive->name()
                      << "', the third input type should be tensor or scalar, but got invalid abstract type:"
                      << input_args[kInputIndex2]->type_name() << ".";
  }
  auto params_shp = params->GetShape()->GetShapeVector();
  auto indices_shp = indices->GetShape()->GetShapeVector();
  ShapeVector out_shape = {};
  constexpr int dynamic_rank_value = -2;
  if (IsDynamicRank(params_shp) || IsDynamicRank(indices_shp) || is_axis_dyn) {
    out_shape.push_back(dynamic_rank_value);
    return std::make_shared<abstract::Shape>(out_shape);
  }

  auto params_rank = static_cast<int64_t>(params_shp.size());
  CheckAndConvertUtils::CheckInRange<int64_t>("axis", axis_val, kIncludeLeft, {-params_rank, params_rank}, op_name);
  // check axis_val within interval: [0, params_rank)
  if (!(-params_rank <= axis_val) || !(axis_val < params_rank)) {
    MS_LOG(EXCEPTION) << "For 'Gather', axis value must be within range [" << -params_rank << ", " << params_rank
                      << "], but got: " << axis_val << ".";
  }
  if (axis_val < 0) {
    axis_val += params_rank;
  }
  if (op_name == kNameGather) {
    int64_t batch_dims = GetValue<int64_t>(primitive->GetAttr(kBatchDims));
    CheckBatchDims(batch_dims, axis_val, params_shp, indices_shp, op_name);
    out_shape = CalcuateGatherWithBatchDimsOutputShape(batch_dims, axis_val, indices_shp, params_shp);
    return std::make_shared<abstract::Shape>(out_shape);
  }

  auto calc_shape = [axis_val](const ShapeVector &ind_vec, const ShapeVector &params_vec) -> ShapeVector {
    ShapeVector out_vec;
    (void)std::copy(params_vec.begin(), params_vec.begin() + axis_val, std::back_inserter(out_vec));
    (void)copy(ind_vec.begin(), ind_vec.end(), std::back_inserter(out_vec));
    (void)copy(params_vec.begin() + axis_val + 1, params_vec.end(), std::back_inserter(out_vec));
    return out_vec;
  };
  out_shape = calc_shape(indices_shp, params_shp);
  return std::make_shared<abstract::Shape>(out_shape);
}

TypePtr GatherInferType(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  const std::string &op_name = primitive->name();
  constexpr int64_t input_num = 3;
  CheckAndConvertUtils::CheckInputArgs(input_args, kEqual, input_num, op_name);
  std::set<TypePtr> valid_params_types = {kTensorType};
  (void)CheckAndConvertUtils::CheckSubClass("params", input_args[kInputIndex0]->GetType(), valid_params_types, op_name);
  std::set<TypePtr> int_types = {kInt8, kInt16, kInt32, kInt64};
  (void)CheckAndConvertUtils::CheckTensorTypeValid("indices", input_args[kInputIndex1]->GetType(), int_types, op_name);
  (void)CheckAndConvertUtils::CheckTypeValid("axis", input_args[kInputIndex2]->GetType(), int_types, op_name);

  auto params = CheckAndConvertUtils::CheckArgsType(op_name, input_args, 0, kObjectTypeTensorType);
  return params->GetType();
}

AbstractBasePtr GatherInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                            const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  const int64_t kInputsNum = 3;
  CheckAndConvertUtils::CheckInputArgs(input_args, kEqual, kInputsNum, primitive->name());
  auto infer_type = GatherInferType(primitive, input_args);
  auto infer_shape = GatherInferShape(primitive, input_args);
  return abstract::MakeAbstract(infer_shape, infer_type);
}
}  // namespace ops
}  // namespace mindspore
