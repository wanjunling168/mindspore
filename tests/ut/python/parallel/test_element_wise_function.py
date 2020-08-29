# Copyright 2019 Huawei Technologies Co., Ltd
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

import numpy as np

import mindspore as ms
import mindspore.nn as nn
from mindspore import Tensor
from mindspore import context
from mindspore.common.api import _executor
from mindspore.ops import composite as C
from mindspore.ops import operations as P
from tests.ut.python.ops.test_math_ops import VirtualLoss


grad_all = C.GradOperation(get_all=True)


class NetWithLoss(nn.Cell):
    def __init__(self, network):
        super(NetWithLoss, self).__init__()
        self.loss = VirtualLoss()
        self.network = network

    def construct(self, x, y, b):
        predict = self.network(x, y, b)
        return self.loss(predict)


class GradWrap(nn.Cell):
    def __init__(self, network):
        super(GradWrap, self).__init__()
        self.network = network

    def construct(self, x, y, b):
        return grad_all(self.network)(x, y, b)


def compile_net(net, x, y, b):
    net.set_auto_parallel()
    _executor.compile(net, x, y, b)


def test_matmul_pow():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.pow = P.Pow().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.pow(out, 2.0)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2), ())
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float32)
    y = Tensor(np.ones([32, 64]), dtype=ms.float32)
    b = Tensor(np.ones([64, 64]), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_exp():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.exp = P.Exp().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.exp(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float32)
    y = Tensor(np.ones([32, 64]), dtype=ms.float32)
    b = Tensor(np.ones([64, 64]), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_log():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.log = P.Log().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.log(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float32)
    y = Tensor(np.ones([32, 64]), dtype=ms.float32)
    b = Tensor(np.ones([64, 64]), dtype=ms.float32)
    compile_net(net, x, y, b)

def test_matmul_abs():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.abs = P.Abs().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.abs(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)

def test_matmul_sign():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.sign = P.Sign().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.sign(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_floor():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.floor = P.Floor().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.floor(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)

def test_matmul_round():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.round = P.Round().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.round(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_reciprocal():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.reciprocal = P.Reciprocal().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.reciprocal(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_inv():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.inv = P.Inv().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.inv(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_rsqrt():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.rsqrt = P.Rsqrt().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.rsqrt(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_tan():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.tan = P.Tan().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.tan(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_sin():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.sin = P.Sin().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.sin(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_sinh():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.sinh = P.Sinh().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.sinh(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_log1p():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.log1p = P.Log1p().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.log1p(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_expm1():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.expm1 = P.Expm1().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.expm1(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_cosh():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.cosh = P.Cosh().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.cosh(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)

def test_matmul_erf():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.erf = P.Erf().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.erf(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(1, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(1, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(1, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_erfc():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.erfc = P.Erfc().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.erfc(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(1, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(1, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(1, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_zeroslike():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.zeroslike = P.ZerosLike().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.zeroslike(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(1, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(1, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(1, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_oneslike():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.oneslike = P.OnesLike().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.oneslike(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(1, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(1, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(1, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_BesselI0e():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.BesselI0e = P.BesselI0e().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.BesselI0e(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(1, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(1, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(1, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_BesselI1e():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.BesselI1e = P.BesselI1e().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.BesselI1e(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(1, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(1, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(1, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_ceil():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.Ceil = P.Ceil().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.Ceil(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_atan():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.atan = P.Atan().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.atan(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_Atanh():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.atanh = P.Atanh().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.atanh(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_asin():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.asin = P.Asin().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.asin(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_asinh():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.asinh = P.Asinh().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.asinh(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_acosh():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.acosh = P.Acosh().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy1)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.acosh(out)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.random.uniform(-5, 5, size=(128, 32)), dtype=ms.float32)
    y = Tensor(np.random.uniform(-5, 5, size=(32, 64)), dtype=ms.float32)
    b = Tensor(np.random.uniform(-5, 5, size=(64, 64)), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_logical_not():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2, strategy3):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.logicalnot = P.LogicalNot().set_strategy(strategy2)
            self.equal = P.Equal().set_strategy(strategy3)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            out = self.equal(out, b)
            out = self.logicalnot(out)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    strategy3 = ((4, 2), (4, 2))
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2, strategy3)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float32)
    y = Tensor(np.ones([32, 64]), dtype=ms.float32)
    b = Tensor(np.ones([128, 64]), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_matmul_cast():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2, strategy3):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.cast = P.Cast().set_strategy(strategy2)
            self.matmul2 = P.MatMul().set_strategy(strategy3)

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            b = self.cast(b, ms.float32)
            out = self.matmul2(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((4, 2),)
    strategy3 = ((1, 4), (4, 2))
    net = GradWrap(NetWithLoss(Net(strategy1, strategy2, strategy3)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float32)
    y = Tensor(np.ones([32, 64]), dtype=ms.float32)
    b = Tensor(np.ones([64, 64]), dtype=ms.int32)
    compile_net(net, x, y, b)


def test_gradient_fp32_sync():
    class Net(nn.Cell):
        def __init__(self, strategy1):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.cast = P.Cast()

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            b = self.cast(b, ms.float32)
            out = self.matmul(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0, gradient_fp32_sync=True)
    strategy1 = ((2, 2), (2, 2))
    net = GradWrap(NetWithLoss(Net(strategy1)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float32)
    y = Tensor(np.ones([32, 64]), dtype=ms.float32)
    b = Tensor(np.ones([64, 64]), dtype=ms.float16)
    compile_net(net, x, y, b)


def test_gradient_fp32_sync1():
    class Net(nn.Cell):
        def __init__(self, strategy1):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.cast = P.Cast()

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            b = self.cast(b, ms.float16)
            out = self.matmul(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0, gradient_fp32_sync=True)
    strategy1 = ((2, 2), (2, 2))
    net = GradWrap(NetWithLoss(Net(strategy1)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float16)
    y = Tensor(np.ones([32, 64]), dtype=ms.float16)
    b = Tensor(np.ones([64, 64]), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_gradient_fp32_sync2():
    class Net(nn.Cell):
        def __init__(self, strategy1):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.cast = P.Cast()

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            b = self.cast(b, ms.float16)
            out = self.matmul(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0, gradient_fp32_sync=False)
    strategy1 = ((2, 2), (2, 2))
    net = GradWrap(NetWithLoss(Net(strategy1)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float16)
    y = Tensor(np.ones([32, 64]), dtype=ms.float16)
    b = Tensor(np.ones([64, 64]), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_gradient_fp32_sync3():
    class Net(nn.Cell):
        def __init__(self, strategy1):
            super().__init__()
            self.matmul = P.MatMul().set_strategy(strategy1)
            self.cast = P.Cast()

        def construct(self, x, y, b):
            out = self.matmul(x, y)
            b = self.cast(b, ms.float16)
            out = self.matmul(out, b)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    net = GradWrap(NetWithLoss(Net(strategy1)))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float16)
    y = Tensor(np.ones([32, 64]), dtype=ms.float16)
    b = Tensor(np.ones([64, 64]), dtype=ms.float32)
    compile_net(net, x, y, b)


def test_mul_two_cast():
    class Net(nn.Cell):
        def __init__(self, strategy1, strategy2, strategy3):
            super().__init__()
            self.mul = P.Mul().set_strategy(strategy1)
            self.mul2 = P.Mul().set_strategy(strategy2)
            self.cast = P.Cast().set_strategy(strategy3)
            self.cast2 = P.Cast().set_strategy(strategy3)

        def construct(self, x, y, b):
            out = self.mul(x, y)
            out = self.mul2(out, b)
            out = self.cast(out, ms.int32)
            out = self.cast2(out, ms.bool_)
            return out

    context.set_auto_parallel_context(device_num=8, global_rank=0)
    strategy1 = ((2, 2), (2, 2))
    strategy2 = ((8, 1), (8, 1))
    strategy3 = ((8, 1),)
    net = GradWrap(Net(strategy1, strategy2, strategy3))
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")

    x = Tensor(np.ones([128, 32]), dtype=ms.float32)
    y = Tensor(np.ones([128, 32]), dtype=ms.float32)
    b = Tensor(np.ones([128, 32]), dtype=ms.float32)
    compile_net(net, x, y, b)
