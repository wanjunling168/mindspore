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
import numpy as np
import pytest
import mindspore as ms
import mindspore.nn as nn
from mindspore import Tensor
from mindspore import ops
from mindspore.common import dtype as mstype


class Net(nn.Cell):
    def construct(self, x, weights=None, minlength=0):
        return ops.bincount(x, weights, minlength)


@pytest.mark.level1
@pytest.mark.platform_x86_cpu
@pytest.mark.platform_arm_cpu
@pytest.mark.platform_x86_gpu_training
@pytest.mark.env_onecard
@pytest.mark.parametrize('mode', [ms.GRAPH_MODE, ms.PYNATIVE_MODE])
def test_bincount(mode):
    """
    Feature: bincount
    Description: Verify the result of bincount
    Expectation: success
    """
    ms.set_context(mode=mode)
    x = Tensor([2, 4, 1, 0, 0], dtype=mstype.int64)
    weights = Tensor([0., 0.25, 0.5, 0.75, 1.], dtype=mstype.float32)
    minlength = 7
    net = Net()
    output = net(x, weights, minlength)
    expect_output = np.array([1.75, 0.50, 0., 0., 0.25, 0., 0.], dtype=np.float32)
    assert np.allclose(output.asnumpy(), expect_output)
