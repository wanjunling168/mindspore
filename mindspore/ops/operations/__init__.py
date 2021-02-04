# Copyright 2021 Huawei Technologies Co., Ltd
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

"""
Primitive operator classes.

A collection of operators to build neural networks or to compute functions.
"""

from .image_ops import (CropAndResize)
from .array_ops import (Argmax, Argmin, Cast, Concat, Pack, Stack, Unpack, Unstack,
                        Diag, DiagPart, DType, ExpandDims, Eye,
                        Fill, Ones, Zeros, GatherNd, GatherV2, Gather, SparseGatherV2, InvertPermutation,
                        IsInstance, IsSubClass, ArgMaxWithValue, OnesLike, ZerosLike,
                        Rank, Reshape, ResizeNearestNeighbor, ArgMinWithValue, Meshgrid,
                        SameTypeShape, ScatterAdd, ScatterSub, ScatterMul, ScatterDiv, ScatterMax, ScatterMin,
                        ScatterUpdate, ScalarToArray, ScalarToTensor, ScatterNd, ScatterNdUpdate, Select,
                        Shape, DynamicShape, Size, Slice, Split, TransShape, ParallelConcat, Padding, UniqueWithPad,
                        ScatterNdAdd, ScatterNdSub, ScatterNonAliasingAdd, ReverseV2, Rint,
                        Squeeze, StridedSlice, Tile, TensorScatterUpdate, EditDistance, Sort,
                        Transpose, TruncatedNormal, TupleToArray, UnsortedSegmentMin, UnsortedSegmentMax,
                        UnsortedSegmentProd, UnsortedSegmentSum, SpaceToDepth, DepthToSpace, SpaceToBatch, BatchToSpace,
                        SpaceToBatchND, BatchToSpaceND, BroadcastTo, InplaceUpdate, ReverseSequence, EmbeddingLookup,
                        Unique, GatherD, Identity, Range)
from .comm_ops import (AllGather, AllReduce, _AlltoAll, AllSwap, ReduceScatter, Broadcast,
                       _MirrorOperator, _MirrorMiniStepOperator, ReduceOp, _VirtualDataset,
                       _VirtualDiv, _GetTensorSlice,
                       _HostAllGather, _HostReduceScatter)
from .debug_ops import (ImageSummary, InsertGradientOf, HookBackward, ScalarSummary,
                        TensorSummary, HistogramSummary, Print, Assert)
from .control_ops import ControlDepend, GeSwitch, Merge
from .inner_ops import ScalarCast, Randperm, NoRepeatNGram, LambApplyOptimizerAssign, LambApplyWeightAssign, MakeRefKey

from .math_ops import (Abs, ACos, Asin, Asinh, AddN, AccumulateNV2, AssignAdd, AssignSub, Atan2, BatchMatMul, BitwiseAnd, BitwiseOr,
                       BitwiseXor, Inv, Invert, ApproximateEqual, InplaceAdd, InplaceSub,
                       ReduceMax, ReduceMin, ReduceMean, ReduceSum, ReduceAll, ReduceProd, CumProd, ReduceAny,
                       Cos, Div, DivNoNan, Equal, EqualCount, Exp, Expm1, Erf, Erfc, Floor, FloorDiv, FloorMod, Ceil,
                       Acosh, Greater, GreaterEqual, Less, LessEqual, Log, Log1p, LogicalAnd, Mod,
                       LogicalNot, LogicalOr, MatMul, Maximum, MulNoNan,
                       Minimum, Mul, Neg, NMSWithMask, NotEqual,
                       NPUAllocFloatStatus, NPUClearFloatStatus, LinSpace,
                       NPUGetFloatStatus, Pow, RealDiv, IsNan, IsInf, IsFinite, FloatStatus,
                       Reciprocal, CumSum, HistogramFixedWidth, SquaredDifference, Xdivy, Xlogy,
                       Sin, Sqrt, Rsqrt, BesselI0e, BesselI1e, TruncateDiv, TruncateMod,
                       Square, Sub, TensorAdd, Add, Sign, Round, SquareSumAll, Atan, Atanh, Cosh, Sinh, Eps, Tan,
                       MatrixInverse)

from .random_ops import (RandomChoiceWithMask, StandardNormal, Gamma, Poisson, UniformInt, UniformReal,
                         RandomCategorical, StandardLaplace, Multinomial, UniformCandidateSampler,
                         LogUniformCandidateSampler)
from .nn_ops import (LSTM, SGD, Adam, FusedSparseAdam, FusedSparseLazyAdam, AdamNoUpdateParam, ApplyMomentum, BatchNorm,
                     BiasAdd, Conv2D,
                     DepthwiseConv2dNative,
                     DropoutDoMask, Dropout, Dropout3d, DropoutGenMask, Flatten,
                     FusedBatchNorm, FusedBatchNormEx, InstanceNorm, BNTrainingReduce, BNTrainingUpdate,
                     Gelu, FastGelu, Elu,
                     GetNext, L2Normalize, LayerNorm, L2Loss, CTCLoss, CTCGreedyDecoder,
                     LogSoftmax,
                     MaxPool, DataFormatDimMap,
                     AvgPool, Conv2DBackpropInput, ComputeAccidentalHits,
                     MaxPoolWithArgmax, OneHot, Pad, MirrorPad, Mish, PReLU, ReLU, ReLU6, ReLUV2, HSwish, HSigmoid,
                     ResizeBilinear, Sigmoid, SeLU,
                     SigmoidCrossEntropyWithLogits, NLLLoss,
                     SmoothL1Loss, Softmax, Softsign, Softplus, LRN, RNNTLoss, DynamicRNN, DynamicGRUV2,
                     SoftmaxCrossEntropyWithLogits, ROIAlign,
                     SparseSoftmaxCrossEntropyWithLogits, Tanh,
                     TopK, BinaryCrossEntropy, KLDivLoss, SparseApplyAdagrad, LARSUpdate, ApplyFtrl, SparseApplyFtrl,
                     ApplyProximalAdagrad, SparseApplyProximalAdagrad, SparseApplyAdagradV2, SparseApplyFtrlV2,
                     FusedSparseFtrl, FusedSparseProximalAdagrad,
                     ApplyAdaMax, ApplyAdadelta, ApplyAdagrad, ApplyAdagradV2,
                     ApplyAddSign, ApplyPowerSign, ApplyGradientDescent, ApplyProximalGradientDescent,
                     ApplyRMSProp, ApplyCenteredRMSProp, BasicLSTMCell, InTopK)
from . import _quant_ops
from ._quant_ops import *
from .other_ops import (Assign, InplaceAssign, IOU, BoundingBoxDecode, BoundingBoxEncode,
                        ConfusionMatrix, PopulationCount,
                        CheckValid, Partial, Depend, identity, CheckBprop, Push, Pull)
from ._thor_ops import (CusBatchMatMul, CusCholeskyTrsm, CusFusedAbsMax1, CusImg2Col, CusMatMulCubeDenseLeft,
                        CusMatMulCubeFraczRightMul, CusMatMulCube, CusMatrixCombine, CusTranspose02314,
                        CusMatMulCubeDenseRight,
                        CusMatMulCubeFraczLeftCast, Im2Col, UpdateThorGradient, Cholesky, CholeskyTrsm, DetTriangle,
                        ProdForceSeA)
from .sparse_ops import SparseToDense
from ._embedding_cache_ops import (CacheSwapHashmap, SearchCacheIdx, CacheSwapTable, UpdateCache, MapCacheIdx, SubAndFilter,
                                   MapUniform, DynamicAssign, PadAndShift)

__all__ = [
    'Unique',
    'ReverseSequence',
    'Sort',
    'EditDistance',
    'CropAndResize',
    'Add',
    'TensorAdd',
    'Argmax',
    'Argmin',
    'ArgMaxWithValue',
    'ArgMinWithValue',
    'AddN',
    'AccumulateNV2',
    'Sub',
    'CumSum',
    'MatMul',
    'BatchMatMul',
    'Mul',
    'Meshgrid',
    'Pow',
    'Exp',
    'Expm1',
    'Rsqrt',
    'Sqrt',
    'Square',
    'DynamicGRUV2',
    'SquaredDifference',
    'Xdivy',
    'Xlogy',
    'Conv2D',
    'Flatten',
    'MaxPoolWithArgmax',
    'FusedBatchNorm',
    'FusedBatchNormEx',
    'BNTrainingReduce',
    'BNTrainingUpdate',
    'BatchNorm',
    'MaxPool',
    'TopK',
    'LinSpace',
    'Adam',
    'FusedSparseAdam',
    'FusedSparseLazyAdam',
    'AdamNoUpdateParam',
    'Softplus',
    'Softmax',
    'Softsign',
    'LogSoftmax',
    'SoftmaxCrossEntropyWithLogits',
    'ROIAlign',
    'SparseSoftmaxCrossEntropyWithLogits',
    'NLLLoss',
    'SGD',
    'ApplyMomentum',
    'ExpandDims',
    'Cast',
    'IsSubClass',
    'IsInstance',
    'Reshape',
    'Squeeze',
    'Transpose',
    'OneHot',
    'GatherV2',
    'Gather',
    'SparseGatherV2',
    'EmbeddingLookup',
    'Padding',
    'GatherD',
    'Identity',
    'UniqueWithPad',
    'Concat',
    'Pack',
    'Stack',
    'Unpack',
    'Unstack',
    'Tile',
    'BiasAdd',
    'Gelu',
    'FastGelu',
    'Minimum',
    'Maximum',
    'StridedSlice',
    'ReduceSum',
    'ReduceMean',
    'LayerNorm',
    'Rank',
    'Less',
    'LessEqual',
    'RealDiv',
    'Div',
    'DivNoNan',
    'Inv',
    'Invert',
    'TruncatedNormal',
    'Fill',
    'Ones',
    'Zeros',
    'OnesLike',
    'ZerosLike',
    'Select',
    'Split',
    'Mish',
    'SeLU',
    'MulNoNan',
    'ReLU',
    'ReLU6',
    'Elu',
    'Erf',
    'Erfc',
    'Sigmoid',
    'HSwish',
    'HSigmoid',
    'Tanh',
    'NoRepeatNGram',
    'Randperm',
    'RandomChoiceWithMask',
    'StandardNormal',
    'Multinomial',
    'Gamma',
    'Poisson',
    'UniformInt',
    'UniformReal',
    'StandardLaplace',
    'RandomCategorical',
    'ResizeBilinear',
    'ScalarSummary',
    'ImageSummary',
    'TensorSummary',
    'HistogramSummary',
    "Print",
    "Assert",
    'InsertGradientOf',
    'HookBackward',
    'InvertPermutation',
    'Shape',
    'DynamicShape',
    'DropoutDoMask',
    'DropoutGenMask',
    'Dropout',
    'Neg',
    'InplaceAdd',
    'InplaceSub',
    'Slice',
    'DType',
    'NPUAllocFloatStatus',
    'NPUGetFloatStatus',
    'NPUClearFloatStatus',
    'IsNan',
    'IsFinite',
    'IsInf',
    'FloatStatus',
    'Reciprocal',
    'SmoothL1Loss',
    'L2Loss',
    'CTCLoss',
    'CTCGreedyDecoder',
    'RNNTLoss',
    'DynamicRNN',
    'ReduceAll',
    'ReduceAny',
    'ScalarToArray',
    'ScalarToTensor',
    'TupleToArray',
    'ControlDepend',
    'GeSwitch',
    'Merge',
    'SameTypeShape',
    'CheckBprop',
    'CheckValid',
    'BoundingBoxEncode',
    'BoundingBoxDecode',
    'L2Normalize',
    'ScatterAdd',
    'ScatterSub',
    'ScatterMul',
    'ScatterDiv',
    'ScatterNd',
    'ScatterMax',
    'ScatterMin',
    'ScatterNdAdd',
    'ScatterNdSub',
    'ScatterNonAliasingAdd',
    'ReverseV2',
    'Rint',
    'ResizeNearestNeighbor',
    'HistogramFixedWidth',
    'Pad',
    'MirrorPad',
    'GatherNd',
    'TensorScatterUpdate',
    'ScatterUpdate',
    'ScatterNdUpdate',
    'Floor',
    'NMSWithMask',
    'IOU',
    'Partial',
    'MakeRefKey',
    'Depend',
    'identity',
    'AvgPool',
    # Back Primitive
    'Equal',
    'EqualCount',
    'NotEqual',
    'Greater',
    'GreaterEqual',
    'LogicalNot',
    'LogicalAnd',
    'LogicalOr',
    'Size',
    'DepthwiseConv2dNative',
    'UnsortedSegmentSum',
    'UnsortedSegmentMin',
    'UnsortedSegmentMax',
    'UnsortedSegmentProd',
    "AllGather",
    "AllReduce",
    "AllSwap",
    "ReduceScatter",
    "Broadcast",
    "ReduceOp",
    'ScalarCast',
    'GetNext',
    'ReduceMax',
    'ReduceMin',
    'ReduceProd',
    'CumProd',
    'Log',
    'Log1p',
    'SigmoidCrossEntropyWithLogits',
    'FloorDiv',
    'FloorMod',
    'TruncateDiv',
    'TruncateMod',
    'Ceil',
    'Acosh',
    'Asinh',
    "PReLU",
    "Cos",
    "Cosh",
    "ACos",
    "Diag",
    "DiagPart",
    'Eye',
    'Assign',
    'AssignAdd',
    'AssignSub',
    "Sin",
    "Sinh",
    "Asin",
    "LSTM",
    "Abs",
    "BinaryCrossEntropy",
    "KLDivLoss",
    "SparseApplyAdagrad",
    "SparseApplyAdagradV2",
    "SpaceToDepth",
    "DepthToSpace",
    "Conv2DBackpropInput",
    "ComputeAccidentalHits",
    "Sign",
    "LARSUpdate",
    "Round",
    "Eps",
    "ApplyFtrl",
    "SpaceToBatch",
    "SparseApplyFtrl",
    "SparseApplyFtrlV2",
    "FusedSparseFtrl",
    "ApplyProximalAdagrad",
    "SparseApplyProximalAdagrad",
    "FusedSparseProximalAdagrad",
    "ApplyAdaMax",
    "ApplyAdadelta",
    "ApplyAdagrad",
    "ApplyAdagradV2",
    "ApplyAddSign",
    "ApplyPowerSign",
    "ApplyGradientDescent",
    "ApplyProximalGradientDescent",
    "BatchToSpace",
    "Atan2",
    "ApplyRMSProp",
    "ApplyCenteredRMSProp",
    "SpaceToBatchND",
    "BatchToSpaceND",
    "SquareSumAll",
    "BitwiseAnd",
    "BitwiseOr",
    "BitwiseXor",
    "BesselI0e",
    "BesselI1e",
    "Atan",
    "Atanh",
    "Tan",
    "BasicLSTMCell",
    "BroadcastTo",
    "DataFormatDimMap",
    "ApproximateEqual",
    "InplaceUpdate",
    "InTopK",
    "UniformCandidateSampler",
    "LogUniformCandidateSampler",
    "LRN",
    "Mod",
    "ConfusionMatrix",
    "PopulationCount",
    "ParallelConcat",
    "Push",
    "Pull",
    "ReLUV2",
    "SparseToDense",
    "MatrixInverse",
    "Range",
]

__all__.sort()
