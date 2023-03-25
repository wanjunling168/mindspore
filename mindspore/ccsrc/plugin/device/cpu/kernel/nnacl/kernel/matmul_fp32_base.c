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

#include "nnacl/kernel/matmul_fp32_base.h"
#include "nnacl/fp32/pack_fp32.h"
#include "nnacl/fp32/matmul_fp32.h"
#include "nnacl/op_base.h"
#if defined(ENABLE_AVX512)
#include "nnacl/kernel/matmul_fp32_avx512.h"
#include "nnacl/intrinsics/ms_simd_cpu_info.h"
#endif
#if defined(ENABLE_AVX)
#include "nnacl/kernel/matmul_fp32_avx.h"
#endif
#if defined(ENABLE_SSE)
#include "nnacl/kernel/matmul_fp32_sse.h"
#endif
#if defined(ENABLE_ARM32)
#include "nnacl/kernel/matmul_fp32_arm32.h"
#endif
#if defined(ENABLE_ARM64)
#include "nnacl/kernel/matmul_fp32_arm64.h"
#endif

#define kNumDeepThreshold 512

int MatmulFp32Run(void *cdata, int task_id, float l, float r) {
  NNACL_CHECK_NULL_RETURN_ERR(cdata);
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)cdata;
  return matmul->parallel_run_(matmul, task_id);
}

int MatmulFp32Base_PackMatrixBParallelRunByBatch(MatmulFp32Struct *matmul, int task_id) {
  MatMulParameter *param = (MatMulParameter *)(matmul->base_.param_);
  int start = task_id * matmul->pack_b_stride_;
  if (param->b_transpose_) {
    int end = MSMIN(matmul->col_, start + matmul->pack_b_stride_);
    matmul->matrix_b_pack_fun_(matmul->pack_b_src_, matmul->pack_b_dst_, matmul->col_, matmul->deep_, start, end);
  } else {
    int end = MSMIN(matmul->deep_, start + matmul->pack_b_stride_);
    matmul->matrix_b_pack_fun_(matmul->pack_b_src_, matmul->pack_b_dst_, matmul->deep_, matmul->col_, start, end);
  }
  return NNACL_OK;
}

int MatmulFp32PackMatrixBRun(void *cdata, int task_id, float l, float r) {
  NNACL_CHECK_NULL_RETURN_ERR(cdata);
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)cdata;
  return MatmulFp32Base_PackMatrixBParallelRunByBatch(matmul, task_id);
}

bool MatmulFp32Base_CheckRow1OptimalConditions(MatmulFp32Struct *matmul) {
  return matmul->row_ == 1 &&
         !(matmul->support_mul_batch_cut_by_row_ && (matmul->a_batch_ > 1 && matmul->b_batch_ == 1));
}

int MatmulFp32Base_InitParameter(MatmulFp32Struct *matmul) {
  MatMulParameter *param = (MatMulParameter *)(matmul->base_.param_);

  matmul->init_global_varibale_(matmul);
  if (MatmulFp32Base_CheckRow1OptimalConditions(matmul)) {
    matmul->row_tile_ = 1;
    matmul->matrix_a_pack_fun_ = param->a_transpose_ ? RowMajor2ColMajorParallel : RowMajor2RowMajorParallel;
    matmul->matrix_a_.need_pack_ = false;
    matmul->pack_opt_ = false;
    if (!matmul->b_const_ && matmul->col_ <= C128NUM) {
      matmul->col_tile_ = 1;
      matmul->out_need_aligned_ = false;
      matmul->matrix_b_pack_fun_ = param->b_transpose_ ? RowMajor2ColMajorParallel : RowMajor2RowMajorParallel;
      matmul->matrix_b_.need_pack_ = param->b_transpose_;
    }
  }
  if (matmul->col_ == 1 && !matmul->a_const_) {
    matmul->out_need_aligned_ = false;
    matmul->row_tile_ = 1;
    matmul->col_tile_ = 1;
    matmul->matrix_a_pack_fun_ = param->a_transpose_ ? RowMajor2ColMajorParallel : RowMajor2RowMajorParallel;
    matmul->matrix_b_pack_fun_ = param->b_transpose_ ? RowMajor2ColMajorParallel : RowMajor2RowMajorParallel;
    matmul->matrix_a_.need_pack_ = param->a_transpose_ && matmul->row_ != 1;
    matmul->matrix_b_.need_pack_ = false;
    matmul->pack_opt_ = false;
  }
  matmul->row_align_ = UP_ROUND(matmul->row_, matmul->row_tile_);
  matmul->col_align_ = UP_ROUND(matmul->col_, matmul->col_tile_);
  MS_CHECK_INT_MUL_NOT_OVERFLOW(matmul->a_batch_, matmul->row_align_, NNACL_ERR);
  MS_CHECK_INT_MUL_NOT_OVERFLOW(matmul->a_batch_ * matmul->row_align_, matmul->deep_, NNACL_ERR);
  MS_CHECK_INT_MUL_NOT_OVERFLOW(matmul->a_batch_, matmul->col_align_, NNACL_ERR);
  MS_CHECK_INT_MUL_NOT_OVERFLOW(matmul->a_batch_ * matmul->col_align_, matmul->deep_, NNACL_ERR);
  size_t a_pack_size = matmul->a_batch_ * matmul->row_align_ * matmul->deep_;
  size_t b_pack_size = matmul->b_batch_ * matmul->col_align_ * matmul->deep_;
  if ((matmul->matrix_a_.has_packed_ && matmul->matrix_a_.pack_size_ != a_pack_size) ||
      (matmul->matrix_b_.has_packed_ && matmul->matrix_b_.pack_size_ != b_pack_size)) {
    return NNACL_ERR;
  }
  matmul->matrix_a_.pack_size_ = a_pack_size;
  matmul->matrix_b_.pack_size_ = b_pack_size;
  matmul->row_align_ = UP_ROUND(matmul->row_, matmul->row_tile_);
  matmul->out_need_aligned_ = (matmul->out_need_aligned_ && ((matmul->col_ % matmul->col_tile_) != 0));
  matmul->col_step_ = matmul->out_need_aligned_ ? matmul->col_align_ : matmul->col_;
  MS_CHECK_FALSE(INT_MUL_OVERFLOW(matmul->a_batch_, matmul->row_), NNACL_ERR);
  matmul->row_num_ = matmul->a_batch_ * matmul->row_;
  return NNACL_OK;
}

int MatmulFp32Base_PackMatrixAImplOpt(MatmulFp32Struct *matmul) { return NNACL_ERR; }

int MatmulFp32Base_PackMatrixAImpl(MatmulFp32Struct *matmul) {
  MatMulParameter *param = (MatMulParameter *)(matmul->base_.param_);
  float *src_ptr =
    (matmul->matrix_a_.has_origin_) ? (matmul->matrix_a_.origin_ptr_) : (float *)(matmul->base_.in_[FIRST_INPUT].data_);
  MS_CHECK_TRUE_RET(src_ptr != NULL, NNACL_ERR);
  MS_CHECK_TRUE_RET(matmul->matrix_a_.pack_ptr_ != NULL, NNACL_ERR);
  MS_CHECK_TRUE_RET(matmul->matrix_a_pack_fun_ != NULL, NNACL_ERR);
  for (int i = 0; i < matmul->a_batch_; i++) {
    const float *src = src_ptr + i * matmul->deep_ * matmul->row_;
    float *dst = matmul->matrix_a_.pack_ptr_ + i * matmul->deep_ * matmul->row_align_;
    if (param->a_transpose_) {
      matmul->matrix_a_pack_fun_(src, dst, matmul->deep_, matmul->row_, 0, matmul->deep_);
    } else {
      matmul->matrix_a_pack_fun_(src, dst, matmul->row_, matmul->deep_, 0, matmul->row_);
    }
  }
  return NNACL_OK;
}

int MatmulFp32Base_PackMatrixBImpl(MatmulFp32Struct *matmul) {
  MatMulParameter *param = (MatMulParameter *)(matmul->base_.param_);

  float *src_ptr = matmul->matrix_b_.has_origin_
                     ? matmul->matrix_b_.origin_ptr_
                     : (matmul->conv1x1_origin_weight_ != NULL ? matmul->conv1x1_origin_weight_
                                                               : (float *)(matmul->base_.in_[SECOND_INPUT].data_));
  MS_CHECK_TRUE_RET(src_ptr != NULL, NNACL_ERR);
  MS_CHECK_TRUE_RET(matmul->matrix_b_.pack_ptr_ != NULL, NNACL_ERR);
  MS_CHECK_TRUE_RET(matmul->matrix_b_pack_fun_ != NULL, NNACL_ERR);

  for (int i = 0; i < matmul->b_batch_; i++) {
    if (param->b_transpose_) {
      matmul->pack_b_stride_ = UP_DIV(matmul->col_, matmul->base_.thread_nr_);
    } else {
      matmul->pack_b_stride_ = UP_DIV(matmul->deep_, matmul->base_.thread_nr_);
    }
    matmul->pack_b_src_ = src_ptr + i * matmul->deep_ * matmul->col_;
    matmul->pack_b_dst_ = matmul->matrix_b_.pack_ptr_ + i * matmul->deep_ * matmul->col_align_;
    int ret = matmul->base_.env_->parallel_launch(matmul->base_.env_->thread_pool_, MatmulFp32PackMatrixBRun, matmul,
                                                  matmul->base_.thread_nr_);
    MS_CHECK_FALSE(ret != NNACL_OK, NNACL_ERR);
  }
  return NNACL_OK;
}

int MatmulFp32Base_PackMatrixA(MatmulFp32Struct *matmul) {
  if (!matmul->a_const_) {
    if (!matmul->matrix_a_.need_pack_) {
      matmul->matrix_a_.pack_ptr_ = (float *)matmul->base_.in_[0].data_;
      return NNACL_OK;
    }
    if (matmul->base_.train_session_) {
      matmul->matrix_a_.pack_ptr_ = (float *)(matmul->base_.workspace_);
    } else {
      matmul->matrix_a_.pack_ptr_ = (float *)(matmul->base_.env_->alloc(matmul->base_.env_->allocator_,
                                                                        matmul->matrix_a_.pack_size_ * sizeof(float)));
    }
  } else {
    bool is_packed = false;
    void *data = NULL;
    size_t data_size = (size_t)(matmul->matrix_a_.pack_size_) * sizeof(float);
    if (matmul->is_sharing_pack_) {
      matmul->get_pack_data_by_sharing_weight_(matmul->base_.in_[FIRST_INPUT].data_, data_size, &is_packed);
    } else {
      data = malloc(data_size);
    }
    NNACL_CHECK_NULL_RETURN_ERR(data);
    matmul->matrix_a_.pack_ptr_ = (float *)data;
    if (is_packed) {
      return NNACL_OK;
    }
  }
  if (matmul->pack_opt_) {
    /* valid in arm64 */
    return matmul->pack_matrix_a_impl_opt_(matmul);
  }
  return matmul->pack_matrix_a_impl_(matmul);
}

int MatmulFp32Base_PackMatrixB(MatmulFp32Struct *matmul) {
  if (!matmul->b_const_) {
    if (!matmul->matrix_b_.need_pack_) {
      matmul->matrix_b_.pack_ptr_ = (float *)matmul->base_.in_[SECOND_INPUT].data_;
      return NNACL_OK;
    }
    if (matmul->base_.train_session_) {
      matmul->matrix_b_.pack_ptr_ = (float *)(matmul->base_.workspace_) + matmul->matrix_a_.pack_size_;
    } else {
      matmul->matrix_b_.pack_ptr_ = (float *)(matmul->base_.env_->alloc(matmul->base_.env_->allocator_,
                                                                        matmul->matrix_b_.pack_size_ * sizeof(float)));
    }
  } else {
    if (!matmul->matrix_b_.need_pack_ && matmul->weight_is_packed_) {
      matmul->matrix_b_.pack_ptr_ = (float *)matmul->base_.in_[SECOND_INPUT].data_;
      return NNACL_OK;
    }
    bool is_packed = false;
    void *data = NULL;
    size_t data_size = (size_t)(matmul->matrix_b_.pack_size_) * sizeof(float);
    if (matmul->is_sharing_pack_) {
      matmul->get_pack_data_by_sharing_weight_(matmul->base_.in_[SECOND_INPUT].data_, data_size, &is_packed);
    } else {
      data = malloc(data_size);
    }
    NNACL_CHECK_NULL_RETURN_ERR(data);
    matmul->matrix_b_.pack_ptr_ = (float *)data;
    if (is_packed) {
      return NNACL_OK;
    }
  }
  return matmul->pack_matrix_b_impl_(matmul);
}

int MatmulFp32Base_BackupConstMatrix(MatmulFp32Struct *matmul, MatrixInfo *matrix_info, int index) {
  MS_CHECK_TRUE_RET(index < matmul->base_.in_size_, NNACL_ERR);
  int backup_size = GetElementNum(&(matmul->base_.in_[0])) * sizeof(float);
  MS_CHECK_TRUE_RET(backup_size > 0, NNACL_ERR);
  matrix_info->origin_ptr_ = (float *)(matmul->base_.env_->alloc(matmul->base_.env_->allocator_, backup_size));
  NNACL_CHECK_NULL_RETURN_ERR(matrix_info->origin_ptr_);
  void *src_ptr = matmul->base_.in_[0].data_;
  NNACL_CHECK_NULL_RETURN_ERR(src_ptr);
  (void)memcpy(matrix_info->origin_ptr_, src_ptr, backup_size * sizeof(float));
  matrix_info->has_origin_ = true;
  return NNACL_OK;
}

int MatmulFp32Base_ParallelRunByRow(MatmulFp32Struct *matmul, int task_id) { return NNACL_OK; }

int MatmulFp32Base_PackBiasMatrix(MatmulFp32Struct *matmul) {
  if (matmul->base_.in_size_ != FOURTH_INPUT) {
    return NNACL_OK;
  }
  if (matmul->matrix_c_.has_packed_) {
    MS_CHECK_FALSE(matmul->matrix_c_.pack_size_ < matmul->col_align_, NNACL_ERR);
    return NNACL_OK;
  }
  TensorC *bias_tensor = &(matmul->base_.in_[THIRD_INPUT]);
  float *bias_src =
    matmul->matrix_c_.has_origin_
      ? matmul->matrix_c_.origin_ptr_
      : (matmul->conv1x1_origin_bias_ != NULL ? matmul->conv1x1_origin_bias_ : (float *)(bias_tensor->data_));
  NNACL_CHECK_NULL_RETURN_ERR(bias_src);

  int bias_num = GetElementNum(bias_tensor);
  MS_CHECK_TRUE_RET(bias_num > 0 && matmul->col_align_ >= bias_num, NNACL_ERR);

  matmul->matrix_c_.pack_size_ = matmul->col_align_;
  matmul->matrix_c_.pack_ptr_ = (float *)(malloc(matmul->matrix_c_.pack_size_ * sizeof(float)));
  NNACL_CHECK_NULL_RETURN_ERR(matmul->matrix_c_.pack_ptr_);

  if (bias_num == 1) {
    for (int i = 0; i < matmul->matrix_c_.pack_size_; ++i) {
      matmul->matrix_c_.pack_ptr_[i] = bias_src[0];
    }
  } else {
    (void)memcpy(matmul->matrix_c_.pack_ptr_, bias_src, bias_num * sizeof(float));
    (void)memset(matmul->matrix_c_.pack_ptr_ + bias_num, 0, (matmul->matrix_c_.pack_size_ - bias_num) * sizeof(float));
  }
  if (matmul->matrix_c_.has_origin_) {
    matmul->base_.env_->free(matmul->base_.env_->allocator_, matmul->matrix_c_.origin_ptr_);
    matmul->matrix_c_.origin_ptr_ = NULL;
    matmul->matrix_c_.has_origin_ = false;
  }
  return NNACL_OK;
}

int MatmulFp32Base_InitTmpOutBuffer(MatmulFp32Struct *matmul) {
  if (matmul->out_need_aligned_) {
    if (matmul->output_data_ != NULL) {
      free(matmul->output_data_);
    }
    // avx need to malloc dst aligned to C8NUM
    // avx512 need to malloc dst aligned to C16NUM
    int out_channel = matmul->col_;
    int oc_block_num = UP_DIV(out_channel, matmul->col_tile_);
    int data_size = matmul->batch_ * matmul->row_ * oc_block_num * matmul->col_tile_ * sizeof(float);
    matmul->output_data_ = (float *)(malloc(data_size));
    if (matmul->output_data_ == NULL) {
      return NNACL_ERR;
    }
  }
  return NNACL_OK;
}

bool MatmulFp32Base_CheckThreadCuttingByRow() { return false; }

void MatmulFp32Base_FreePackedMatrixA(KernelBase *self) {
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)self;
  if (matmul->matrix_a_.need_pack_ && !matmul->base_.train_session_ && matmul->matrix_a_.pack_ptr_ != NULL) {
    self->env_->free(self->env_->allocator_, matmul->matrix_a_.pack_ptr_);
  }
  matmul->matrix_a_.pack_ptr_ = NULL;
}

void MatmulFp32Base_FreePackedMatrixB(KernelBase *self) {
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)self;
  if (matmul->matrix_b_.need_pack_ && !matmul->base_.train_session_ && matmul->matrix_b_.pack_ptr_ != NULL) {
    matmul->base_.env_->free(matmul->base_.env_->allocator_, matmul->matrix_b_.pack_ptr_);
  }
  matmul->matrix_b_.pack_ptr_ = NULL;
}

int matmul_fp32_resize(KernelBase *self) {
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)self;

  int ret = MatmulFp32Base_InitParameter(matmul);
  MS_CHECK_FALSE(ret != NNACL_OK, ret);
  if (self->train_session_) {
    self->work_size_ = (matmul->matrix_a_.pack_size_ + matmul->matrix_b_.pack_size_) * sizeof(float);
  }

  ret = matmul->get_thread_cutting_policy_(matmul);
  MS_CHECK_FALSE(ret != NNACL_OK, ret);

  if (!matmul->matrix_c_.has_packed_) {
    ret = MatmulFp32Base_PackBiasMatrix(matmul);
    MS_CHECK_FALSE(ret != NNACL_OK, ret);
    matmul->matrix_c_.has_packed_ = true;
  }
  ret = MatmulFp32Base_InitTmpOutBuffer(matmul);
  MS_CHECK_FALSE(ret != NNACL_OK, ret);

  return NNACL_OK;
}

int matmul_fp32_release(struct KernelBase *self) {
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)self;

  // packed const-matrix will be delete by framework.
  if (matmul->out_need_aligned_ && matmul->output_data_ != NULL) {
    free(matmul->output_data_);
    matmul->output_data_ = NULL;
  }
  if (matmul->matrix_c_.pack_ptr_ != NULL) {
    free(matmul->matrix_c_.pack_ptr_);
    matmul->matrix_c_.pack_ptr_ = NULL;
  }
  if (matmul->a_const_) {
    if (matmul->is_sharing_pack_) {
      matmul->free_by_sharing_weight_(matmul->matrix_a_.pack_ptr_);
    } else {
      free(matmul->matrix_a_.pack_ptr_);
    }
  }
  if (matmul->b_const_) {
    if (!matmul->matrix_b_.need_pack_ && matmul->weight_is_packed_) {
      return NNACL_OK;
    }
    if (matmul->is_sharing_pack_) {
      matmul->free_by_sharing_weight_(matmul->matrix_b_.pack_ptr_);
    } else {
      free(matmul->matrix_b_.pack_ptr_);
    }
  }
  return NNACL_OK;
}

int matmul_fp32_prepare(struct KernelBase *self) {
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)self;

  MS_CHECK_FALSE(matmul->base_.in_size_ < C2NUM, NNACL_ERR);
  MS_CHECK_FALSE(matmul->base_.out_size_ < 1, NNACL_ERR);
  MS_CHECK_FALSE(matmul->base_.in_[FIRST_INPUT].data_type_ != kNumberTypeFloat32, NNACL_ERR);
  MS_CHECK_FALSE(matmul->base_.in_[SECOND_INPUT].data_type_ != kNumberTypeFloat32, NNACL_ERR);

  if (matmul->base_.in_size_ == FOURTH_INPUT) {
    MS_CHECK_FALSE(matmul->base_.in_[THIRD_INPUT].data_type_ != kNumberTypeFloat32, NNACL_ERR);
    if (self->in_[THIRD_INPUT].data_ == NULL) {
      return NNACL_ERR;
    }
  }

  MatMulParameter *param = (MatMulParameter *)(matmul->base_.param_);
  if (param->act_type_ != ActType_No && param->act_type_ != ActType_Relu && param->act_type_ != ActType_Relu6) {
    return NNACL_ERR;
  }

  int ret = MatmulFp32Base_InitParameter(matmul);
  MS_CHECK_FALSE(ret != NNACL_OK, ret);

  if (matmul->a_const_) {
    ret = MatmulFp32Base_PackMatrixA(matmul);
    MS_CHECK_FALSE(ret != NNACL_OK, ret);
    matmul->matrix_a_.has_packed_ = true;
  }
  if (matmul->b_const_) {
    ret = MatmulFp32Base_PackMatrixB(matmul);
    MS_CHECK_FALSE(ret != NNACL_OK, ret);
    matmul->matrix_b_.has_packed_ = true;
  }
  if (!matmul->infer_shape_) {
    if (matmul->base_.in_size_ == FOURTH_INPUT && !matmul->base_.train_session_) {
      ret = MatmulFp32Base_BackupConstMatrix(matmul, &matmul->matrix_c_, THIRD_INPUT);
      MS_CHECK_FALSE(ret != NNACL_OK, ret);
    }
  }
  return NNACL_OK;
}

int matmul_fp32_compute(struct KernelBase *self) {
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)self;

  float *out_data = (float *)(matmul->base_.out_[FIRST_INPUT].data_);
  MS_CHECK_FALSE(out_data == NULL, NNACL_ERR);
  if (!matmul->out_need_aligned_) {
    matmul->output_data_ = out_data;
  }

  if (!matmul->a_const_) {
    int ret = MatmulFp32Base_PackMatrixA(matmul);
    MS_CHECK_FALSE(ret != NNACL_OK, ret);
  }
  if (!matmul->b_const_) {
    int ret = MatmulFp32Base_PackMatrixB(matmul);
    MS_CHECK_FALSE(ret != NNACL_OK, ret);
  }

  NNACL_CHECK_NULL_RETURN_ERR(matmul->matrix_a_.pack_ptr_);
  NNACL_CHECK_NULL_RETURN_ERR(matmul->matrix_b_.pack_ptr_);

  int ret = self->env_->parallel_launch(self->env_->thread_pool_, MatmulFp32Run, self, self->thread_nr_);
  MS_CHECK_FALSE(ret != NNACL_OK, ret);

  if (matmul->out_need_aligned_) {
    PackNHWCXToNHWCFp32(matmul->output_data_, out_data, matmul->batch_, matmul->row_, matmul->col_, matmul->col_tile_);
  } else {
    matmul->output_data_ = NULL;
  }
  if (!matmul->a_const_) {
    MatmulFp32Base_FreePackedMatrixA(self);
  }

  if (!matmul->b_const_) {
    MatmulFp32Base_FreePackedMatrixB(self);
  }
  return NNACL_OK;
}

void InitMatrixInfo(MatrixInfo *info) {
  info->need_pack_ = false;
  info->has_packed_ = false;
  info->has_origin_ = false;
  info->pack_size_ = -1;
  info->origin_ptr_ = NULL;
  info->pack_ptr_ = NULL;
}

KernelBase *CreateMatmulFp32Base() {
  MatmulFp32Struct *matmul = (MatmulFp32Struct *)malloc(sizeof(MatmulFp32Struct));
  NNACL_CHECK_NULL_RETURN_NULL(matmul);
  matmul->base_.prepare = matmul_fp32_prepare;
  matmul->base_.resize = matmul_fp32_resize;
  matmul->base_.release = matmul_fp32_release;
  matmul->base_.compute = matmul_fp32_compute;
  InitMatrixInfo(&(matmul->matrix_a_));
  InitMatrixInfo(&(matmul->matrix_b_));
  InitMatrixInfo(&(matmul->matrix_c_));
  matmul->pack_opt_ = false;
  matmul->out_need_aligned_ = false;
  matmul->conv1x1_origin_bias_ = NULL;
  matmul->model_thread_nr_ = -1;
  matmul->support_mul_batch_cut_by_row_ = false;
  matmul->check_thread_cutting_by_row_ = MatmulFp32Base_CheckThreadCuttingByRow;
  matmul->pack_matrix_a_impl_opt_ = MatmulFp32Base_PackMatrixAImplOpt;
  matmul->pack_matrix_a_impl_ = MatmulFp32Base_PackMatrixAImpl;
  matmul->pack_matrix_b_impl_ = MatmulFp32Base_PackMatrixBImpl;
  matmul->parallel_run_by_row_ = MatmulFp32Base_ParallelRunByRow;
  return (KernelBase *)matmul;
}

KernelBase *CreateMatmulFp32() {
  KernelBase *matmul = NULL;

#if defined(ENABLE_AVX512)
  AVX512_HARDWARE_SELF_AWARENESS_BEGIN
  matmul = CreateMatmulFp32Avx512();
  if (matmul != NULL) {
    return matmul;
  }
  AVX512_HARDWARE_SELF_AWARENESS_END
#endif

#if defined(ENABLE_AVX)
  matmul = CreateMatmulFp32Avx();
  if (matmul != NULL) {
    return matmul;
  }
#endif

#if defined(ENABLE_SSE)
  matmul = CreateMatmulFp32Sse();
  if (matmul != NULL) {
    return matmul;
  }
#endif

#if defined(ENABLE_ARM64)
  matmul = CreateMatmulFp32Arm64();
  if (matmul != NULL) {
    return matmul;
  }
#endif

#if defined(ENABLE_ARM32)
  matmul = CreateMatmulFp32Arm32();
  if (matmul != NULL) {
    return matmul;
  }
#endif

  matmul = CreateMatmulFp32Base();
  return matmul;
}