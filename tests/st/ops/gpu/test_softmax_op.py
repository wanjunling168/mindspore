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
# ============================================================================

import numpy as np
import pytest

import mindspore.context as context
import mindspore.nn as nn
from mindspore import Tensor
from mindspore.ops import composite as C
from mindspore.ops import operations as P


class NetSoftmax(nn.Cell):
    def __init__(self):
        super(NetSoftmax, self).__init__()
        axis = -2
        self.softmax1 = P.Softmax()
        self.softmax2 = P.Softmax(axis)

    def construct(self, x):
        return self.softmax1(x), self.softmax2(x)


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.env_onecard
def test_softmax():
    x = Tensor(np.array([[0.1, 0.3, 0.6, -0.3],
                         [0.2, -0.6, 0.8, 0.6],
                         [0.6, -1.2, 0.4, 0.6]]).astype(np.float32))
    expect1 = np.ones(3)
    expect2 = np.ones(4)
    error1 = expect1 * 1.0e-6
    error2 = expect2 * 1.0e-6

    context.set_context(mode=context.PYNATIVE_MODE, device_target="GPU")
    Softmax = NetSoftmax()
    output = Softmax(x)
    outputSum1 = output[0].asnumpy().sum(axis=1)
    outputSum2 = output[1].asnumpy().sum(axis=0)
    diff1 = np.abs(outputSum1 - expect1)
    diff2 = np.abs(outputSum2 - expect2)
    assert np.all(diff1 < error1)
    assert np.all(diff2 < error2)

    context.set_context(mode=context.GRAPH_MODE, device_target="GPU")
    Softmax = NetSoftmax()
    output = Softmax(x)
    outputSum1 = output[0].asnumpy().sum(axis=1)
    outputSum2 = output[1].asnumpy().sum(axis=0)
    diff1 = np.abs(outputSum1 - expect1)
    diff2 = np.abs(outputSum2 - expect2)
    assert np.all(diff1 < error1)
    assert np.all(diff2 < error2)


class Net(nn.Cell):
    def __init__(self):
        super(Net, self).__init__()
        self.softmax1 = P.Softmax()

    def construct(self, x):
        return self.softmax1(x)


class Grad(nn.Cell):
    def __init__(self, network):
        super(Grad, self).__init__()
        self.grad = C.GradOperation(get_all=True, sens_param=True)
        self.network = network

    def construct(self, input_data, sens):
        gout = self.grad(self.network)(input_data, sens)
        return gout


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.env_onecard
def test_softmax_4d():
    context.set_context(mode=context.GRAPH_MODE, device_target="GPU")
    x = np.array([[[[2.7866030e-01, 8.5578346e-01, -2.7546784e-01, -8.5833269e-01, 1.5753637e-01],
                    [-4.5145524e-01, 1.5590921e-01, -6.1947298e-01, -6.3499230e-01, -1.0625143e+00],
                    [-6.8716180e-01, -3.5565588e-01, 9.9680430e-01, -3.5519487e-01, 5.2122700e-01],
                    [-9.8125875e-01, 9.0505141e-01, 6.5961617e-01, 6.5950197e-01, 1.0319239e+00]],
                   [[-7.6588345e-01, -1.6929083e-01, 9.4459933e-01, -8.3931917e-01, 1.4916732e+00],
                    [8.1874236e-02, -1.9288104e-02, 7.3255712e-01, -1.4598954e-01, 1.1225560e+00],
                    [2.7356184e-01, 1.2557162e-01, 1.3796539e+00, 1.0073920e-01, 7.9203087e-01],
                    [-3.6947381e-01, 4.7919992e-01, 2.2421131e+00, -8.3911163e-01, 1.0814662e+00]],
                   [[-2.5838584e-01, 2.0765430e-01, -1.9366746e-01, 6.7511219e-01, -3.7492469e-01],
                    [4.4170797e-01, -9.9537361e-01, -3.5100895e-01, -7.8317386e-01, 1.1672008e-02],
                    [1.6037937e+00, -1.7059358e+00, -9.3724984e-01, -1.5016698e+00, -2.7605603e-02],
                    [1.6392696e-01, 1.0074581e+00, -2.7704465e+00, 8.1361882e-02, 7.9730105e-01]]],
                  [[[2.9516423e-01, 4.6354745e-02, 1.7318316e-01, 1.5894413e+00, -1.2769363e+00],
                    [2.8939021e-01, -3.8801813e-01, -1.3376296e+00, -4.9808905e-01, -3.2318991e-02],
                    [-1.1740140e+00, -1.1140432e+00, -1.4198960e-01, 5.8953021e-02, -3.6763316e-01],
                    [1.8660797e+00, -5.8705074e-01, 6.8757606e-01, -4.0573463e-01, -7.1130061e-01]],
                   [[2.6170531e-01, 5.4814044e-02, 1.3891056e-01, 3.4492522e-02, -1.0920379e-01],
                    [1.1420644e-01, 1.6939731e-01, -1.0413316e+00, -1.4040415e-01, -3.3280477e-01],
                    [-3.0776244e-01, 1.0526397e+00, 2.9497927e-01, 1.1266683e+00, 8.4419928e-02],
                    [-2.1593940e+00, -1.0187222e+00, 1.7475771e+00, -3.5802367e-01, -1.2900480e+00]],
                   [[3.2892069e-01, -1.6604670e+00, -5.7856506e-01, 5.8143520e-01, 5.9596705e-01],
                    [-1.5992336e-01, -5.9647644e-01, 1.2957820e+00, -1.0650631e-01, 7.0879894e-01],
                    [4.1372257e-01, 3.6408889e-01, -6.3091749e-01, 1.0573713e+00, 1.0981073e+00],
                    [-1.9162457e-01, 3.6392561e-05, -1.8338780e-01, 1.7549801e+00, -9.3534666e-01]]]]).astype(
                        np.float32)

    dy = np.array([[[[2.98213929e-01, 3.10518718e+00, -1.64306939e-01, -7.33681679e-01, 5.23136854e-02],
                     [-3.47142726e-01, -1.52662742e+00, 5.26977003e-01, 5.29672280e-02, -4.34386432e-01],
                     [1.34674394e+00, 1.69386661e+00, 3.17139983e-01, 5.77129781e-01, 1.25290680e+00],
                     [-1.71099675e+00, -1.62872851e+00, -7.89083183e-01, 8.64615321e-01, -1.74364686e+00]],
                    [[1.11915946e+00, -7.06878662e-01, -6.71557069e-01, -4.50884640e-01, 2.95763493e-01],
                     [-7.64747679e-01, 1.62951392e-03, -2.84069944e-02, 7.55402744e-01, -1.02387452e+00],
                     [-5.92088878e-01, 4.47980821e-01, 4.50127304e-01, -3.99038166e-01, -5.24561822e-01],
                     [1.92535609e-01, 2.44671494e-01, -8.70469391e-01, -8.30129832e-02, -4.04477213e-03]],
                    [[-1.94159836e-01, -8.50215256e-01, -1.01224804e+00, 2.64235616e-01, 5.34391068e-02],
                     [-6.71353936e-01, 3.73690695e-01, 4.48037744e-01, -2.84973383e-01, -2.80129910e+00],
                     [6.69475198e-01, 2.08404279e+00, 4.49459851e-01, 2.50908136e+00, 9.80683088e-01],
                     [1.18290365e+00, -1.28790128e+00, -1.70202863e+00, -1.37078688e-01, 9.53227460e-01]]],
                   [[[-6.44128084e-01, 1.37707603e+00, -8.60912442e-01, -3.83467346e-01, 6.68365955e-01],
                     [-3.32795471e-01, 3.05202007e-01, 2.20850635e+00, 6.93960607e-01, -1.94968760e-01],
                     [-3.35764170e-01, 1.10562348e+00, -1.13264215e+00, -1.08296621e+00, -6.53923571e-01],
                     [-4.64974046e-01, 8.83257568e-01, -1.70353889e+00, -4.48120385e-01, -1.76938546e+00]],
                    [[-3.80976290e-01, -1.49393475e+00, -8.51393223e-01, -1.49780405e+00, -1.24160886e-01],
                     [-7.18508661e-02, 2.44543999e-01, 3.29225749e-01, 7.09274471e-01, -9.26648498e-01],
                     [6.67312503e-01, -1.08737612e+00, -9.63039994e-01, -3.22715081e-02, -4.03802067e-01],
                     [-5.97982287e-01, -1.40739769e-01, 2.80631828e+00, 5.72278857e-01, 2.05998325e+00]],
                    [[3.46207246e-02, 7.34213948e-01, 1.45563519e+00, 1.02045703e+00, 1.40984225e+00],
                     [4.14457440e-01, -8.74118507e-01, -4.21902031e-01, 7.87168801e-01, -1.48280108e+00],
                     [1.42688036e+00, -2.02695489e+00, 9.26816165e-01, 9.37691629e-01, 7.85577714e-01],
                     [-6.59893751e-01, 1.14681525e-02, -5.79456389e-01, -1.65206456e+00, 4.37116653e-01]]]]).astype(
                         np.float32)

    expect_x = np.array([[[[0.21919312, 0.3903627, 0.12594244, 0.07031325, 0.19418849],
                           [0.19778392, 0.36304963, 0.16719443, 0.1646197, 0.10735231],
                           [0.07986113, 0.11125171, 0.43020225, 0.11130301, 0.26738194],
                           [0.03936873, 0.25963634, 0.20313013, 0.20310691, 0.29475793]],
                          [[0.05308856, 0.09640461, 0.29366633, 0.04932966, 0.50751084],
                           [0.13426398, 0.12134594, 0.2573638, 0.10690536, 0.38012096],
                           [0.13503104, 0.11645612, 0.40813455, 0.11359984, 0.22677852],
                           [0.04576753, 0.10693795, 0.6233836, 0.02861518, 0.19529575]],
                          [[0.14096586, 0.2246532, 0.15039064, 0.35853124, 0.12545899],
                           [0.37957698, 0.09019516, 0.17180163, 0.11151683, 0.2469094],
                           [0.7375885, 0.0269412, 0.05811028, 0.03304673, 0.14431332],
                           [0.16174863, 0.37599453, 0.00859921, 0.1489303, 0.3047274]]],
                         [[[0.15335402, 0.11957449, 0.13574363, 0.55949026, 0.03183762],
                           [0.34669915, 0.17609946, 0.06813136, 0.15774474, 0.2513253],
                           [0.09487908, 0.10074313, 0.26630113, 0.32556766, 0.21250896],
                           [0.6357843, 0.05469263, 0.19565557, 0.0655652, 0.0483023]],
                          [[0.23898226, 0.19431841, 0.21136671, 0.19040942, 0.16492325],
                           [0.2641041, 0.27909, 0.08316323, 0.20473833, 0.16890427],
                           [0.08062991, 0.3142761, 0.14732064, 0.33842432, 0.11934903],
                           [0.01604616, 0.05020634, 0.79826504, 0.09720672, 0.03827571]],
                          [[0.24191543, 0.03308899, 0.09762195, 0.31140763, 0.31596598],
                           [0.10669514, 0.06895282, 0.45745608, 0.11254943, 0.25434658],
                           [0.16156755, 0.15374413, 0.05684244, 0.3075298, 0.32031605],
                           [0.09346025, 0.11320464, 0.09423324, 0.65467626, 0.04442552]]]]).astype(np.float32)

    expect_dx = np.array([[[[-0.20103945, 0.737705, -0.17376284, -0.1370458, -0.22585672],
                            [0.04461281, -0.34632078, 0.18386088, 0.10299816, 0.01484894],
                            [0.04113413, 0.09592049, -0.22135337, -0.02833145, 0.11263024],
                            [-0.0284293, -0.1661311, 0.04058228, 0.37645525, -0.22247711]],
                           [[0.06355994, -0.06061868, -0.17428297, -0.01839012, 0.1897318],
                            [-0.04652473, 0.05094835, 0.10032654, 0.12546772, -0.23021786],
                            [-0.07882182, 0.05314343, 0.18712361, -0.04438123, -0.11706398],
                            [0.03219109, 0.08079126, -0.22419631, 0.01224192, 0.09897206]],
                           [[0.01057316, -0.1305348, -0.11175273, 0.19124077, 0.04047358],
                            [0.07448982, 0.11195826, 0.2260284, 0.06497248, -0.47744888],
                            [-0.09664576, 0.03458005, -0.02039931, 0.05646288, 0.02600216],
                            [0.1973966, -0.47014874, -0.01431374, -0.01483214, 0.30189803]]],
                          [[[-0.06132338, 0.19386888, -0.08370841, -0.07789247, 0.02905542],
                            [-0.16714299, 0.0274538, 0.14029635, 0.08591694, -0.08652411],
                            [0.03585254, 0.18327834, -0.11158065, -0.12024056, 0.01269035],
                            [0.14654502, 0.0863447, -0.19723451, 0.01621746, -0.05187264]],
                           [[0.11614501, -0.12182987, 0.00329342, -0.12011584, 0.12250728],
                            [-0.03623635, 0.05001016, 0.02194443, 0.13183522, -0.16755345],
                            [0.09322704, -0.18807998, -0.06984743, 0.15454148, 0.01015892],
                            [-0.04743218, -0.12545264, 0.35787603, -0.1735842, -0.01140684]],
                           [[-0.21854429, -0.00674347, 0.05053139, 0.02567403, 0.14908233],
                            [0.09731252, -0.02596174, 0.03463032, 0.14460044, -0.2505815],
                            [0.1478814, -0.3902862, 0.02360253, 0.13103928, 0.087763],
                            [0.04834083, 0.13455458, 0.05632052, -0.3109298, 0.07171366]]]]).astype(np.float32)
    y = Net()(Tensor(x))
    assert np.allclose(y.asnumpy(), expect_x)

    dx = Grad(Net())(Tensor(x), Tensor(dy))
    assert np.allclose(dx[0].asnumpy(), expect_dx)
