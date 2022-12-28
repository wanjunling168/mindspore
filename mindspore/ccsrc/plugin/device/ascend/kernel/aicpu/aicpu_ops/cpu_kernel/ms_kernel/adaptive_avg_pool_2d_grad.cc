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
#include "cpu_kernel/ms_kernel/adaptive_avg_pool_2d_grad.h"
#include <cmath>
#include "utils/eigen_tensor.h"
#include "utils/kernel_util.h"
#include "cpu_kernel/common/cpu_kernel_utils.h"

namespace {
const char *kAdaptiveAvgPool2dGrad = "AdaptiveAvgPool2dGrad";
template <typename SCALAR_T>
struct AdaptiveCalcArgs {
  SCALAR_T *input_data = nullptr;
  SCALAR_T *output_data = nullptr;

  int64_t in_size_b = 0;
  int64_t in_size_d = 0;
  int64_t in_size_h = 0;
  int64_t in_size_w = 0;
  int64_t out_size_h = 0;
  int64_t out_size_w = 0;

  int64_t out_stride_d = 0;
  int64_t in_stride_d = 0;
  int64_t out_stride_h = 0;
  int64_t in_stride_h = 0;
};

// out_size is not be zero
inline int StartIndex(int offset, int out_size, int in_size) {
  return (int)std::floor((float)(offset * in_size) / out_size);
}

// out_size is not be zero
inline int EndIndex(int offset, int out_size, int in_size) {
  return (int)std::ceil((float)((offset + 1) * in_size) / out_size);
}
}  // namespace

namespace aicpu {
template <typename SCALAR_T>
uint32_t AdaptiveAvgPool2dGradOutFrame(CpuKernelContext &ctx, AdaptiveCalcArgs<SCALAR_T> args) {
  uint32_t min_core_num = 1;
  int64_t max_core_num = std::max(min_core_num, aicpu::CpuKernelUtils::GetCPUNum(ctx) - 2);

  int64_t total_size = args.in_size_d * args.in_size_b * args.out_size_h * args.out_size_w;
  int64_t max_core_num_total = max_core_num;
  if (max_core_num_total > total_size) {
    max_core_num_total = total_size;
  }
  auto shard_init = [&](int64_t start, int64_t end) {
    for (auto c = start; c < end; c++) {
      args.output_data[c] = (SCALAR_T)0;
    }
  };
  KERNEL_HANDLE_ERROR(CpuKernelUtils::ParallelFor(ctx, total_size, total_size / max_core_num_total, shard_init),
                      "AdaptiveAvgPool2dGrad Compute failed.");

  int64_t in_size_db = args.in_size_d * args.in_size_b;
  if (max_core_num > in_size_db) {
    max_core_num = in_size_db;
  }
  // treat batch size and channels as one dimension
  auto shard_work = [&](int64_t start, int64_t end) {
    for (auto c = start; c < end; c++) {
      SCALAR_T *output_offset_ptr = args.output_data + c * args.out_stride_d;
      SCALAR_T *input_offset_ptr = args.input_data + c * args.in_stride_d;

      for (int64_t ih = 0; ih < args.in_size_h; ih++) {
        int64_t out_start_h = StartIndex(ih, args.in_size_h, args.out_size_h);
        int64_t out_end_h = EndIndex(ih, args.in_size_h, args.out_size_h);
        int64_t step_h = out_end_h - out_start_h;
        for (int64_t iw = 0; iw < args.in_size_w; iw++) {
          int64_t out_start_w = StartIndex(iw, args.in_size_w, args.out_size_w);
          int64_t out_end_w = EndIndex(iw, args.in_size_w, args.out_size_w);
          int64_t step_w = out_end_w - out_start_w;
          if (step_w == 0 || step_h == 0) {
            continue;
          }
          SCALAR_T grad_delta = input_offset_ptr[ih * args.in_stride_h + iw] / step_h / step_w;
          int64_t oh = 0, ow = 0, output_size = args.out_stride_d;
          for (oh = out_start_h; oh < out_end_h; oh++) {
            for (ow = out_start_w; ow < out_end_w; ow++) {
              int64_t output_idx = oh * args.out_stride_h + ow;
              KERNEL_CHECK_FALSE_VOID((output_idx < output_size),
                                      "Feature map output_idx [%lld] overflow output_size [%lld].", output_idx,
                                      output_size);
              output_offset_ptr[output_idx] += grad_delta;
            }
          }
        }
      }
    }
  };
  KERNEL_HANDLE_ERROR(CpuKernelUtils::ParallelFor(ctx, in_size_db, in_size_db / max_core_num, shard_work),
                      "AdaptiveAvgPool2dGrad Compute failed.");
  return KERNEL_STATUS_OK;
}

template <typename SCALAR_T>
uint32_t AdaptiveAvgPool2dGradOutCpuTemplate(CpuKernelContext &ctx) {
  Tensor &input = *(ctx.Input(kFirstInputIndex));

  auto input_shape_ptr = input.GetTensorShape();
  KERNEL_CHECK_NULLPTR(input_shape_ptr, KERNEL_STATUS_PARAM_INVALID, "Get input x shape failed.");
  int32_t input_dims = input_shape_ptr->GetDims();

  for (int32_t i = 0; i < input_dims; i++) {
    KERNEL_CHECK_FALSE((input_shape_ptr->GetDimSize(i) > 0), KERNEL_STATUS_PARAM_INVALID,
                       "Adaptive_avg_pool2d_grad: expected input to have non-empty spatial dimensions, "
                       "but input has sizes [%d] with dimension [%d] being empty.",
                       input_dims, i);
  }

  KERNEL_CHECK_FALSE(input_dims == 4, KERNEL_STATUS_PARAM_INVALID, "Non-empty [4D] tensor expected for input.");

  AdaptiveCalcArgs<SCALAR_T> args;
  args.in_size_b = 1;
  args.in_size_d = 0;
  args.in_size_h = 0;
  args.in_size_w = 0;
  args.out_size_h = 0;
  args.out_size_w = 0;
  args.out_stride_d = 1;
  args.in_stride_d = 1;
  args.out_stride_h = 1;
  args.in_stride_h = 1;

  std::vector<int64_t> orig_input_size = ctx.GetAttr("orig_input_shape")->GetListInt();
  KERNEL_CHECK_FALSE((orig_input_size.size() == 4), KERNEL_STATUS_PARAM_INVALID,
                     "Adaptive_avg_pool2d_grad: internal error, orig_input_size.size() must be [4]");
  KERNEL_CHECK_FALSE((input_shape_ptr->GetDimSize(0) == orig_input_size[0]), KERNEL_STATUS_PARAM_INVALID,
                     "Adaptive_avg_pool2d_grad: internal error, orig_input_size Batch must equal "
                     "input_size Batch, now orig_input_size Batch is [%lld], input_size Batch is [%lld].",
                     input_shape_ptr->GetDimSize(0), orig_input_size[0]);
  KERNEL_CHECK_FALSE((input_shape_ptr->GetDimSize(1) == orig_input_size[1]), KERNEL_STATUS_PARAM_INVALID,
                     "Adaptive_avg_pool2d_grad: internal error, orig_input_size Channel must equal "
                     "input_size channel, now orig_input_size Channel is [%lld], input_size Channel is [%lld].",
                     input_shape_ptr->GetDimSize(1), orig_input_size[1]);

  int dim_w = 3;
  int dim_h = 2;
  // sizes
  args.in_size_d = input_shape_ptr->GetDimSize(dim_h - 1);
  args.in_size_h = input_shape_ptr->GetDimSize(dim_h);
  args.in_size_w = input_shape_ptr->GetDimSize(dim_w);

  args.out_size_h = orig_input_size[dim_h];
  args.out_size_w = orig_input_size[dim_w];
  KERNEL_CHECK_FALSE((args.out_size_h != 0 && args.out_size_w != 0), KERNEL_STATUS_PARAM_INVALID,
                     "Adaptive_avg_pool2d_grad: internal error, output_size H or W can not be zero, "
                     "now H is [%lld], W is [%lld].",
                     args.out_size_h, args.out_size_w);
  // strides
  // The calculation does not overflow because max value is number of user input data,
  // which less then int64_t range.
  args.out_stride_d = args.out_size_h * args.out_size_w;
  args.out_stride_h = args.out_size_w;
  args.in_stride_d = args.in_size_h * args.in_size_w;
  args.in_stride_h = args.in_size_w;

  args.input_data = static_cast<SCALAR_T *>(input.GetData());
  args.output_data = static_cast<SCALAR_T *>(ctx.Output(kFirstOutputIndex)->GetData());

  return AdaptiveAvgPool2dGradOutFrame<SCALAR_T>(ctx, args);
}

uint32_t AdaptiveAvgPool2dGrad::Compute(CpuKernelContext &ctx) {
  Tensor *input_0 = ctx.Input(kFirstInputIndex);
  KERNEL_CHECK_NULLPTR(input_0, KERNEL_STATUS_PARAM_INVALID, "Get input tensor failed.");
  KERNEL_CHECK_NULLPTR(input_0->GetData(), KERNEL_STATUS_PARAM_INVALID, "Get input data failed.");
  Tensor *output_0 = ctx.Output(kFirstOutputIndex);
  KERNEL_CHECK_NULLPTR(output_0, KERNEL_STATUS_PARAM_INVALID, "Get output tensor failed.");
  KERNEL_CHECK_NULLPTR(output_0->GetData(), KERNEL_STATUS_PARAM_INVALID, "Get output data failed.");

  AttrValue *attr_orig_input_shape = ctx.GetAttr("orig_input_shape");
  KERNEL_CHECK_NULLPTR(attr_orig_input_shape, KERNEL_STATUS_PARAM_INVALID, "[%s] get attr:orig_input_shape failed.",
                       kAdaptiveAvgPool2dGrad);
  std::vector<int64_t> v_orig_input_shape = attr_orig_input_shape->GetListInt();

  KERNEL_LOG_INFO("AdaptiveAvgPool2dGrad kernel, input[0]: size is [%llu]; output_0: size is [%llu].",
                  input_0->GetDataSize(), output_0->GetDataSize());
  KERNEL_LOG_INFO("[%s] get attr:orig_input_shape [%s].", kAdaptiveAvgPool2dGrad,
                  VectorToString(v_orig_input_shape).c_str());

  auto data_type = static_cast<DataType>(input_0->GetDataType());
  // Compute by data_type
  switch (data_type) {
    case DT_FLOAT:
      return AdaptiveAvgPool2dGradOutCpuTemplate<float>(ctx);
    case DT_FLOAT16:
      return AdaptiveAvgPool2dGradOutCpuTemplate<Eigen::half>(ctx);
    default:
      KERNEL_LOG_ERROR("AdaptiveAvgPool2dGrad kernel data type [%s] not support.", DTypeStr(data_type).c_str());
      return KERNEL_STATUS_PARAM_INVALID;
  }
}

REGISTER_CPU_KERNEL(kAdaptiveAvgPool2dGrad, AdaptiveAvgPool2dGrad);
}  // namespace aicpu
