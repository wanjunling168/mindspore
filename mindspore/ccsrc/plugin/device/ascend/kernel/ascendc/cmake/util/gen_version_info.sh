#!/bin/bash
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
# Description: Generate npu_supported_ops.json
# ==============================================================================

ascend_install_dir=$1
gen_file_dir=$2

# create version.info
compiler_version=$(grep "Version" -w ${ascend_install_dir}/compiler/version.info | awk -F = '{print $2}')
echo "custom_opp_compiler_version=${compiler_version}" > ${gen_file_dir}/version.info