# Copyright 2023 Huawei Technologies Co., Ltd
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
import mindspore.nn as nn
import mindspore.ops as ops
from mindspore.rewrite import SymbolTree, NodeType, Node, ScopedValue


def external_func(x):
    x = ops.abs(x)
    return x


class SubSubNet(nn.Cell):
    def __init__(self):
        super().__init__()
        self.relu = nn.ReLU()
        self.abs = ops.Abs()

    def construct(self, x):
        x = self.relu(x)
        x = external_func(x)
        x = self.subsubnet_internal_func(x)
        return x

    def subsubnet_internal_func(self, x):
        x = self.abs(x)
        return x


class SubNet(nn.Cell):
    def __init__(self):
        super().__init__()
        self.subsubnet = SubSubNet()
        self.abs = ops.Abs()
        self.relu = nn.ReLU()

    def construct(self, x):
        x = self.relu(x)
        x = external_func(x)
        x = self.subnet_internal_func(x)
        return x

    def subnet_internal_func(self, x):
        x = self.abs(x)
        x = self.subsubnet(x)
        return x


class MyNet(nn.Cell):
    def __init__(self):
        super().__init__()
        self.relu = nn.ReLU()
        self.sub_net = SubNet()
        self.abs = ops.Abs()

    def construct(self, x):
        x = self.relu(x)
        x = external_func(x)
        x = self.internal_func(x)
        return x

    def internal_func(self, x):
        x = self.sub_net(self.abs(x))
        return x


def test_call_function():
    """
    Feature: Python api get_code of Node of Rewrite.
    Description: Test rewrite CallFunction nodes.
    Expectation: Success.
    """
    net = MyNet()
    stree = SymbolTree.create(net)

    # check CallFunction nodes
    internal_func_node = stree.get_node('internal_func')
    assert internal_func_node
    assert internal_func_node.get_node_type() == NodeType.CallFunction
    internal_func_subnodes = internal_func_node.get_handler().nodes()
    assert internal_func_subnodes
    assert len(internal_func_subnodes) == 4

    subtree = internal_func_node.get_handler().get_node("sub_net").symbol_tree
    subnet_internal_func_node = subtree.get_node('subnet_internal_func')
    assert subnet_internal_func_node
    assert subnet_internal_func_node.get_node_type() == NodeType.CallFunction
    subnet_internal_func_subnodes = subnet_internal_func_node.nodes()
    assert subnet_internal_func_subnodes
    assert len(subnet_internal_func_subnodes) == 4

    subsubtree = subnet_internal_func_node.get_node("subsubnet").symbol_tree
    subsubnet_internal_func_node = subsubtree.get_node('subsubnet_internal_func')
    assert subsubnet_internal_func_node
    assert subsubnet_internal_func_node.get_node_type() == NodeType.CallFunction
    subsubnet_internal_func_subnodes = subsubnet_internal_func_node.nodes()
    assert subsubnet_internal_func_subnodes
    assert len(subsubnet_internal_func_subnodes) == 3

    # insert node to CallFunction nodes
    new_node = Node.create_call_cell(ops.Abs(), targets=['x'],
                                     args=[ScopedValue.create_naming_value("x")],
                                     name="new_abs")
    internal_func_abs = internal_func_node.get_handler().get_node('abs')
    stree.insert(stree.after(Node(internal_func_abs)), new_node)

    internal_func_subnodes = internal_func_node.get_handler().nodes()
    assert len(internal_func_subnodes) == 5
    assert internal_func_node.get_handler().get_node('new_abs')

    codes = stree.get_code()
    assert codes.count("self.new_abs = obj.new_abs") == 1
    assert codes.count("x = self.new_abs(x)") == 1

    # erase node in CallFunction nodes
    assert codes.count("abs = self.abs(x)") == 1

    internal_func_node.get_handler().get_node("sub_net").set_arg('x', 0)
    stree.erase(Node(internal_func_abs))

    internal_func_subnodes = internal_func_node.get_handler().nodes()
    assert len(internal_func_subnodes) == 4
    assert not internal_func_node.get_handler().get_node('abs')

    codes = stree.get_code()
    assert codes.count("abs = self.abs(x)") == 0
