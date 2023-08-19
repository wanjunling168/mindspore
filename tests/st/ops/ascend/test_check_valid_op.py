# Copyright 2022 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================
import pytest
import numpy as np
from mindspore import Tensor
import mindspore.context as context
from mindspore.ops import functional as F
from mindspore.common import dtype as mstype


def test_check_valid_functional():
    """
    Feature: test check_valid functional API.
    Description: test case for check_valid functional API.
    Expectation: the result match with expected result.
    """
    bboxes = Tensor(np.linspace(0, 6, 12).reshape(3, 4), mstype.float32)
    img_metas = Tensor(np.array([2, 1, 3]), mstype.float32)
    output = F.check_valid(bboxes, img_metas)
    expected = np.array([True, False, False])
    np.testing.assert_array_almost_equal(output.asnumpy(), expected)


@pytest.mark.level1
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_check_valid_functional_modes():
    """
    Feature: test check_valid functional API in PyNative and Graph modes.
    Description: test case for check_valid functional API.
    Expectation: the result match with expected result.
    """
    context.set_context(mode=context.GRAPH_MODE, device_target="Ascend")
    test_check_valid_functional()
    context.set_context(mode=context.PYNATIVE_MODE, device_target="Ascend")
    test_check_valid_functional()
