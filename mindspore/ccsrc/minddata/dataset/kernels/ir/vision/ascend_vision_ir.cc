/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include <algorithm>

#include "minddata/dataset/kernels/ir/vision/ascend_vision_ir.h"
#ifndef ENABLE_ANDROID
#include "minddata/dataset/kernels/image/image_utils.h"
#endif

#include "minddata/dataset/kernels/image/dvpp/dvpp_crop_jpeg_op.h"
#include "minddata/dataset/kernels/image/dvpp/dvpp_decode_resize_jpeg_op.h"
#include "minddata/dataset/kernels/image/dvpp/dvpp_decode_resize_crop_jpeg_op.h"
#include "minddata/dataset/kernels/image/dvpp/dvpp_decode_jpeg_op.h"
#include "minddata/dataset/kernels/image/dvpp/dvpp_decode_png_op.h"
#include "minddata/dataset/kernels/image/dvpp/dvpp_resize_jpeg_op.h"

namespace mindspore {
namespace dataset {

// Transform operations for computer vision
namespace vision {
/* ####################################### Derived TensorOperation classes ################################# */

// DvppCropOperation
DvppCropJpegOperation::DvppCropJpegOperation(const std::vector<uint32_t> &crop) : crop_(crop) {}

Status DvppCropJpegOperation::ValidateParams() {
  // size
  if (crop_.empty() || crop_.size() > 2) {
    std::string err_msg =
      "DvppCropJpeg: Crop resolution must be a vector of one or two elements, got: " + std::to_string(crop_.size());
    MS_LOG(ERROR) << err_msg;
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  if (*min_element(crop_.begin(), crop_.end()) < 32 || *max_element(crop_.begin(), crop_.end()) > 2048) {
    std::string err_msg = "Dvpp module supports crop image with resolution in range [32, 2048], got crop Parameters: ";
    if (crop_.size() == 2) {
      MS_LOG(ERROR) << err_msg << "[" << crop_[0] << ", " << crop_[1] << "]";
    } else {
      MS_LOG(ERROR) << err_msg << "[" << crop_[0] << ", " << crop_[0] << "]";
    }
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  return Status::OK();
}

std::shared_ptr<TensorOp> DvppCropJpegOperation::Build() {
  // If size is a single value, the smaller edge of the image will be
  // resized to this value with the same image aspect ratio.
  uint32_t cropHeight, cropWidth;
  // User specified the width value.
  if (crop_.size() == 1) {
    cropHeight = crop_[0];
    cropWidth = crop_[0];
  } else {
    cropHeight = crop_[0];
    cropWidth = crop_[1];
  }
  std::shared_ptr<DvppCropJpegOp> tensor_op = std::make_shared<DvppCropJpegOp>(cropHeight, cropWidth);
  return tensor_op;
}

Status DvppCropJpegOperation::to_json(nlohmann::json *out_json) {
  nlohmann::json args;
  args["size"] = crop_;
  *out_json = args;
  return Status::OK();
}

// DvppDecodeResizeOperation
DvppDecodeResizeOperation::DvppDecodeResizeOperation(const std::vector<uint32_t> &resize) : resize_(resize) {}

Status DvppDecodeResizeOperation::ValidateParams() {
  // size
  if (resize_.empty() || resize_.size() > 2) {
    std::string err_msg = "DvppDecodeResizeJpeg: resize resolution must be a vector of one or two elements, got: " +
                          std::to_string(resize_.size());
    MS_LOG(ERROR) << err_msg;
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  if (*min_element(resize_.begin(), resize_.end()) < 32 || *max_element(resize_.begin(), resize_.end()) > 2048) {
    std::string err_msg =
      "Dvpp module supports resize image with resolution in range [32, 2048], got resize Parameters: ";
    if (resize_.size() == 2) {
      MS_LOG(ERROR) << err_msg << "[" << resize_[0] << ", " << resize_[1] << "]";
    } else {
      MS_LOG(ERROR) << err_msg << "[" << resize_[0] << ", " << resize_[0] << "]";
    }
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  return Status::OK();
}

std::shared_ptr<TensorOp> DvppDecodeResizeOperation::Build() {
  // If size is a single value, the smaller edge of the image will be
  // resized to this value with the same image aspect ratio.
  uint32_t resizeHeight, resizeWidth;
  // User specified the width value.
  if (resize_.size() == 1) {
    resizeHeight = resize_[0];
    resizeWidth = 0;
  } else {
    resizeHeight = resize_[0];
    resizeWidth = resize_[1];
  }
  std::shared_ptr<DvppDecodeResizeJpegOp> tensor_op =
    std::make_shared<DvppDecodeResizeJpegOp>(resizeHeight, resizeWidth);
  return tensor_op;
}

Status DvppDecodeResizeOperation::to_json(nlohmann::json *out_json) {
  nlohmann::json args;
  args["size"] = resize_;
  *out_json = args;
  return Status::OK();
}

// DvppDecodeResizeCropOperation
DvppDecodeResizeCropOperation::DvppDecodeResizeCropOperation(const std::vector<uint32_t> &crop,
                                                             const std::vector<uint32_t> &resize)
    : crop_(crop), resize_(resize) {}

Status DvppDecodeResizeCropOperation::ValidateParams() {
  // size
  if (crop_.empty() || crop_.size() > 2) {
    std::string err_msg = "DvppDecodeResizeCropJpeg: crop resolution must be a vector of one or two elements, got: " +
                          std::to_string(crop_.size());
    MS_LOG(ERROR) << err_msg;
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  if (resize_.empty() || resize_.size() > 2) {
    std::string err_msg = "DvppDecodeResizeCropJpeg: resize resolution must be a vector of one or two elements, got: " +
                          std::to_string(resize_.size());
    MS_LOG(ERROR) << err_msg;
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  if (*min_element(crop_.begin(), crop_.end()) < 32 || *max_element(crop_.begin(), crop_.end()) > 2048) {
    std::string err_msg = "Dvpp module supports crop image with resolution in range [32, 2048], got Crop Parameters: ";
    if (crop_.size() == 2) {
      MS_LOG(ERROR) << err_msg << "[" << crop_[0] << ", " << crop_[1] << "]";
    } else {
      MS_LOG(ERROR) << err_msg << "[" << crop_[0] << ", " << crop_[0] << "]";
    }
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  if (*min_element(resize_.begin(), resize_.end()) < 32 || *max_element(resize_.begin(), resize_.end()) > 2048) {
    std::string err_msg =
      "Dvpp module supports resize image with resolution in range [32, 2048], got Crop Parameters: ";
    if (resize_.size() == 2) {
      MS_LOG(ERROR) << err_msg << "[" << resize_[0] << ", " << resize_[1] << "]";
    } else {
      MS_LOG(ERROR) << err_msg << "[" << resize_[0] << "]";
    }
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  if (crop_.size() < resize_.size()) {
    if (crop_[0] > MIN(resize_[0], resize_[1])) {
      std::string err_msg =
        "Each value of crop parameter must be smaller than corresponding resize parameter, for example: x[0] <= "
        "y[0],  and x[1] <= y[1], please verify your input parameters.";
      MS_LOG(ERROR) << err_msg;
      RETURN_STATUS_SYNTAX_ERROR(err_msg);
    }
  }
  if (crop_.size() > resize_.size()) {
    if (MAX(crop_[0], crop_[1]) > resize_[0]) {
      std::string err_msg =
        "Each value of crop parameter must be smaller than corresponding resize parameter, for example: x[0] <= "
        "y[0],  and x[1] <= y[1], please verify your input parameters.";
      MS_LOG(ERROR) << err_msg;
      RETURN_STATUS_SYNTAX_ERROR(err_msg);
    }
  }
  if (crop_.size() == resize_.size()) {
    for (int32_t i = 0; i < crop_.size(); ++i) {
      if (crop_[i] > resize_[i]) {
        std::string err_msg =
          "Each value of crop parameter must be smaller than corresponding resize parameter, for example: x[0] <= "
          "y[0],  and x[1] <= y[1], please verify your input parameters.";
        MS_LOG(ERROR) << err_msg;
        RETURN_STATUS_SYNTAX_ERROR(err_msg);
      }
    }
  }
  return Status::OK();
}

std::shared_ptr<TensorOp> DvppDecodeResizeCropOperation::Build() {
  // If size is a single value, the smaller edge of the image will be
  // resized to this value with the same image aspect ratio.
  uint32_t cropHeight, cropWidth, resizeHeight, resizeWidth;
  if (crop_.size() == 1) {
    cropHeight = crop_[0];
    cropWidth = crop_[0];
  } else {
    cropHeight = crop_[0];
    cropWidth = crop_[1];
  }
  // User specified the width value.
  if (resize_.size() == 1) {
    resizeHeight = resize_[0];
    resizeWidth = 0;
  } else {
    resizeHeight = resize_[0];
    resizeWidth = resize_[1];
  }
  std::shared_ptr<DvppDecodeResizeCropJpegOp> tensor_op =
    std::make_shared<DvppDecodeResizeCropJpegOp>(cropHeight, cropWidth, resizeHeight, resizeWidth);
  return tensor_op;
}

Status DvppDecodeResizeCropOperation::to_json(nlohmann::json *out_json) {
  nlohmann::json args;
  args["crop_size"] = crop_;
  args["resize_size"] = resize_;
  *out_json = args;
  return Status::OK();
}

// DvppDecodeJPEG
Status DvppDecodeJpegOperation::ValidateParams() { return Status::OK(); }

std::shared_ptr<TensorOp> DvppDecodeJpegOperation::Build() { return std::make_shared<DvppDecodeJpegOp>(); }

// DvppDecodePNG
Status DvppDecodePngOperation::ValidateParams() { return Status::OK(); }

std::shared_ptr<TensorOp> DvppDecodePngOperation::Build() { return std::make_shared<DvppDecodePngOp>(); }

// DvppResizeOperation
DvppResizeJpegOperation::DvppResizeJpegOperation(const std::vector<uint32_t> &resize) : resize_(resize) {}

Status DvppResizeJpegOperation::ValidateParams() {
  // size
  if (resize_.empty() || resize_.size() > 2) {
    std::string err_msg = "DvppResizeJpeg: resize resolution must be a vector of one or two elements, got: " +
                          std::to_string(resize_.size());
    MS_LOG(ERROR) << err_msg;
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  if (*min_element(resize_.begin(), resize_.end()) < 32 || *max_element(resize_.begin(), resize_.end()) > 2048) {
    std::string err_msg =
      "Dvpp module supports resize image with resolution in range [32, 2048], got resize Parameters: ";
    if (resize_.size() == 2) {
      MS_LOG(ERROR) << err_msg << "[" << resize_[0] << ", " << resize_[1] << "]";
    } else {
      MS_LOG(ERROR) << err_msg << "[" << resize_[0] << ", " << resize_[0] << "]";
    }
    RETURN_STATUS_SYNTAX_ERROR(err_msg);
  }
  return Status::OK();
}

std::shared_ptr<TensorOp> DvppResizeJpegOperation::Build() {
  // If size is a single value, the smaller edge of the image will be
  // resized to this value with the same image aspect ratio.
  uint32_t resizeHeight, resizeWidth;
  // User specified the width value.
  if (resize_.size() == 1) {
    resizeHeight = resize_[0];
    resizeWidth = 0;
  } else {
    resizeHeight = resize_[0];
    resizeWidth = resize_[1];
  }
  std::shared_ptr<DvppResizeJpegOp> tensor_op = std::make_shared<DvppResizeJpegOp>(resizeHeight, resizeWidth);
  return tensor_op;
}

Status DvppResizeJpegOperation::to_json(nlohmann::json *out_json) {
  nlohmann::json args;
  args["size"] = resize_;
  *out_json = args;
  return Status::OK();
}

}  // namespace vision
}  // namespace dataset
}  // namespace mindspore
