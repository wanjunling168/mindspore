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
""" test list index operation"""

import pytest
from mindspore import Tensor, jit, context


context.set_context(mode=context.GRAPH_MODE)


def test_list_index():
    """
    Feature: list index.
    Description: support list index operation.
    Expectation: No exception.
    """
    @jit
    def foo():
        x = [1, 2, 3, 4]
        return x.index(1)
    assert foo() == 0


def test_list_index_2():
    """
    Feature: list index.
    Description: support list index operation.
    Expectation: No exception.
    """
    @jit
    def foo():
        x = ['1', '2', 3, 4]
        return x.index('2')
    assert foo() == 1


def test_list_index_3():
    """
    Feature: list index.
    Description: support list index operation.
    Expectation: No exception.
    """
    @jit
    def foo():
        x = [Tensor([1]), Tensor([2]), Tensor([3]), Tensor([4])]
        return x.index(Tensor([3]))
    assert foo() == 2


def test_list_index_not_found():
    """
    Feature: list index.
    Description: support list index operation.
    Expectation: Raise ValueError.
    """
    @jit
    def foo():
        x = [1, 2, 3, 4]
        return x.index(5)

    with pytest.raises(ValueError) as info:
        foo()
    assert "is not in" in str(info.value)


def test_list_index_not_found_2():
    """
    Feature: list index.
    Description: support list index operation.
    Expectation: Raise ValueError.
    """
    @jit
    def foo():
        x = [1, 2, 3, 4]
        return x.index(Tensor(1))

    with pytest.raises(ValueError) as info:
        foo()
    assert "is not in" in str(info.value)
