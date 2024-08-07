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
import json
import pytest

import mindspore as ms
import mindspore.nn as nn
from mindspore import Tensor, Layout
from mindspore import context
from mindspore.common.api import _cell_graph_executor
from mindspore.context import set_auto_parallel_context
from mindspore.ops import composite as C
from mindspore.ops.operations.nn_ops import FlashAttentionScore
from mindspore.ops import operations as P
from tests.ut.python.ops.test_math_ops import VirtualLoss


def setup_function():
    context.set_auto_parallel_context(dataset_strategy="full_batch")


grad_all = C.GradOperation(get_all=True)


def generate_inputs(B, N, S, D, input_layout, use_mqa=False, with_real_shift=True, sparse_mode=0):
    N_Q = N
    N_KV = 1 if use_mqa else N
    compressed_mask_mode = [2, 3, 4, 5, 6, 7, 8]
    if input_layout == "BSH":
        H_Q = N_Q * D
        H_KV = N_KV * D
        query = Tensor(np.ones((B, S, H_Q), dtype=np.float16))
        key = Tensor(np.ones((B, S, H_KV), dtype=np.float16))
        value = Tensor(np.ones((B, S, H_KV), dtype=np.float16))
    elif input_layout == "SBH":
        H_Q = N_Q * D
        H_KV = N_KV * D
        query = Tensor(np.ones((S, B, H_Q), dtype=np.float16))
        key = Tensor(np.ones((S, B, H_KV), dtype=np.float16))
        value = Tensor(np.ones((S, B, H_KV), dtype=np.float16))
    elif input_layout == "BNSD":
        query = Tensor(np.ones((B, N_Q, S, D), dtype=np.float16))
        key = Tensor(np.ones((B, N_KV, S, D), dtype=np.float16))
        value = Tensor(np.ones((B, N_KV, S, D), dtype=np.float16))
    elif input_layout == "BSND":
        query = Tensor(np.ones((B, S, N_Q, D), dtype=np.float16))
        key = Tensor(np.ones((B, S, N_KV, D), dtype=np.float16))
        value = Tensor(np.ones((B, S, N_KV, D), dtype=np.float16))
    elif input_layout == "TND":
        query = Tensor(np.ones((B * S, N_Q, D), dtype=np.float16))
        key = Tensor(np.ones((B * S, N_KV, D), dtype=np.float16))
        value = Tensor(np.ones((B * S, N_KV, D), dtype=np.float16))
    else:
        raise ValueError(f"input_layout is invalid.")
    real_shift = Tensor(np.ones((B, N, S, S), dtype=np.float16)) if with_real_shift else None
    if sparse_mode not in compressed_mask_mode:
        attn_mask = Tensor(np.ones((B, 1, S, S), dtype=np.uint8))
    else:
        attn_mask = Tensor(np.ones((2048, 2048), dtype=np.uint8))
    return query, key, value, real_shift, attn_mask


class NetWithLoss(nn.Cell):
    def __init__(self, network):
        super(NetWithLoss, self).__init__()
        self.loss = VirtualLoss()
        self.network = network

    def construct(self, x):
        predict = self.network(x)
        return self.loss(predict)


class GradWrap(nn.Cell):
    def __init__(self, network):
        super(GradWrap, self).__init__()
        self.network = network

    def construct(self, *inputs):
        return grad_all(self.network)(*inputs)


def compile_net(net, *inputs):
    net.set_train()
    _cell_graph_executor.compile(net, *inputs)


class Net(nn.Cell):
    def __init__(self, head_num, keep_prob=0.9, input_layout="BSH", sparse_mode=0, use_mqa=False,
                 with_real_shift=True, dp=None, mp=None, sp=1, use_layout=False):
        super(Net, self).__init__()
        self.reshape = P.Reshape()
        self.drop_gen_mask = P.DropoutGenMask()
        self.keep_prob = Tensor(keep_prob, ms.float16)
        compressed_mask_mode = [2, 3, 4, 5, 6, 7, 8]
        self.head_num = head_num
        self.input_layout = input_layout
        pre_tokens = 2147483647 if sparse_mode not in compressed_mask_mode else 512
        next_tokens = 2147483647 if sparse_mode not in compressed_mask_mode else 0
        self.fa_op = FlashAttentionScore(head_num=head_num,
                                         keep_prob=keep_prob,
                                         pre_tokens=pre_tokens,
                                         next_tokens=next_tokens,
                                         input_layout=input_layout,
                                         sparse_mode=sparse_mode)
        if dp is not None and mp is not None:
            if use_layout:
                if input_layout == "TND":
                    layout = Layout(device_matrix=(dp, sp, mp), alias_name=("dp", "sp", "mp"))
                    kv_head_map_name = "None" if use_mqa else "sp"
                    self.fa_op.shard(in_strategy=(layout(("dp", "sp"), "mp", "None"),
                                                  layout("dp", kv_head_map_name, "None"),
                                                  layout("dp", kv_head_map_name, "None"),
                                                  layout("None", "None"),
                                                  layout("dp"),
                                                  layout("dp")))
                else:
                    raise ValueError("Only TND can be config by layout.")
            else:
                kv_head_stra = 1 if use_mqa else mp
                if input_layout == "BSH":
                    stra = ((dp, sp, mp), (dp, 1, kv_head_stra), (dp, 1, kv_head_stra))
                elif input_layout == "SBH":
                    stra = ((sp, dp, mp), (1, dp, kv_head_stra), (1, dp, kv_head_stra))
                elif input_layout == "BNSD":
                    stra = ((dp, mp, sp, 1), (dp, kv_head_stra, 1, 1), (dp, kv_head_stra, 1, 1))
                elif input_layout == "BSND":
                    stra = ((dp, sp, mp, 1), (dp, 1, kv_head_stra, 1), (dp, 1, kv_head_stra, 1))
                elif input_layout == "TND":
                    stra = ((dp * sp, mp, 1), (dp, kv_head_stra, 1), (dp, kv_head_stra, 1))
                else:
                    raise ValueError(f"input_layout is invalid.")
                if with_real_shift:
                    stra += ((dp, mp, sp, 1),)
                if keep_prob < 1.0:
                    stra += ((dp, mp, sp, 1),)
                if sparse_mode not in compressed_mask_mode:
                    stra += ((dp, 1, sp, 1),)
                else:
                    stra += ((1, 1),)
                if input_layout == "TND":
                    stra += ((dp,),)
                    stra += ((dp,),)
                self.fa_op.shard(stra)

    def construct(self, query, key, value, real_shift, attn_mask, actual_seq_qlen=None, actual_seq_kvlen=None):
        drop_mask_bits = None
        if self.input_layout != "TND":
            if self.input_layout == "BSH":
                bsz, seq_len, _ = query.shape
            elif self.input_layout == "SBH":
                seq_len, bsz, _ = query.shape
            elif self.input_layout == "BNSD":
                bsz, _, seq_len, _ = query.shape
            elif self.input_layout == "BSND":
                bsz, seq_len, _, _ = query.shape
            else:
                raise ValueError(f"input_layout is invalid.")
            if self.keep_prob < 1.0:
                drop_mask_bits = self.reshape(self.drop_gen_mask((bsz, self.head_num, seq_len, seq_len),
                                                                 self.keep_prob),
                                              (bsz, self.head_num, seq_len, 128))
        return self.fa_op(query, key, value, real_shift, drop_mask_bits, None, attn_mask, None, actual_seq_qlen,
                          actual_seq_kvlen)


@pytest.mark.parametrize('keep_prob', [0.9, 1.0])
@pytest.mark.parametrize('input_layout', ["BSH", "SBH", "BNSD", "BSND"])
@pytest.mark.parametrize('with_real_shift', [True, False])
def test_self_attention_standalone(keep_prob, input_layout, with_real_shift):
    """
    Features: test FlashAttentionScoreInfo
    Description: StandAlone
    Expectation: compile success
    """
    context.reset_auto_parallel_context()
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode="stand_alone")
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D, input_layout,
                                                               with_real_shift=with_real_shift)
    net = Net(N, keep_prob, input_layout, with_real_shift=with_real_shift)
    compile_net(net, query, key, value, real_shift, attn_mask)


@pytest.mark.parametrize('input_layout', ["BSH", "SBH", "BNSD", "BSND"])
@pytest.mark.parametrize('sparse_mode', [2, 3, 4])
def test_self_attention_standalone_with_compressed_mask(input_layout, sparse_mode):
    """
    Features: test FlashAttentionScoreInfo with compressed mask
    Description: StandAlone
    Expectation: compile success
    """
    context.reset_auto_parallel_context()
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode="stand_alone")
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D, input_layout,
                                                               sparse_mode=sparse_mode)
    net = Net(N, input_layout=input_layout, sparse_mode=sparse_mode)
    compile_net(net, query, key, value, real_shift, attn_mask)


@pytest.mark.parametrize('input_layout', ["BSH", "SBH", "BNSD", "BSND"])
@pytest.mark.parametrize('use_mqa', [True, False])
@pytest.mark.parametrize('with_real_shift', [True, False])
def test_flash_attention_semi_auto_parallel(input_layout, use_mqa, with_real_shift):
    """
    Features: test FlashAttentionScoreInfo
    Description: semi_auto_parallel with strategy
    Expectation: compile success
    """
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")
    dp = 2
    mp = 4
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D,
                                                               input_layout,
                                                               use_mqa,
                                                               with_real_shift)
    net = Net(N, input_layout=input_layout, use_mqa=use_mqa,
              with_real_shift=with_real_shift, dp=dp, mp=mp)
    compile_net(net, query, key, value, real_shift, attn_mask)


@pytest.mark.parametrize('input_layout', ["BSH", "SBH", "BNSD", "BSND"])
@pytest.mark.parametrize('sparse_mode', [2, 3, 4])
def test_flash_attention_semi_auto_parallel_with_compressed_mask(input_layout, sparse_mode):
    """
    Features: test FlashAttentionScoreInfo with compressed mask
    Description: semi_auto_parallel with strategy
    Expectation: compile success
    """
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")
    dp = 2
    mp = 4
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D,
                                                               input_layout,
                                                               sparse_mode=sparse_mode)
    net = Net(N, input_layout=input_layout, sparse_mode=sparse_mode, dp=dp, mp=mp)
    compile_net(net, query, key, value, real_shift, attn_mask)


@pytest.mark.parametrize('keep_prob', [0.9, 1.0])
@pytest.mark.parametrize('input_layout', ["BSH", "BNSD"])
@pytest.mark.parametrize('with_real_shift', [True, False])
def test_flash_attention_dp(keep_prob, input_layout, with_real_shift):
    """
    Features: test FlashAttentionScore under semi_auto_parallel
    Description: semi_auto_parallel without strategy
    Expectation: compile success
    """
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel")
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D, input_layout,
                                                               with_real_shift=with_real_shift)
    net = Net(N, keep_prob, input_layout, with_real_shift=with_real_shift)
    compile_net(net, query, key, value, real_shift, attn_mask)


@pytest.mark.parametrize('keep_prob', [0.9, 1.0])
@pytest.mark.parametrize('input_layout', ["BSH", "BNSD"])
@pytest.mark.parametrize('use_mqa', [True, False])
@pytest.mark.parametrize('with_real_shift', [True, False])
def test_flash_attention_auto_parallel(keep_prob, input_layout, use_mqa, with_real_shift):
    """
    Features: test FlashAttentionScoreInfo
    Description: auto_parallel
    Expectation: compile success
    """
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode="auto_parallel")
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D, input_layout, use_mqa, with_real_shift)
    net = Net(N, keep_prob, input_layout, use_mqa=use_mqa, with_real_shift=with_real_shift)
    compile_net(net, query, key, value, real_shift, attn_mask)


@pytest.mark.parametrize('input_layout', ["BSH", "SBH", "BNSD", "BSND"])
@pytest.mark.parametrize('use_mqa', [True, False])
@pytest.mark.parametrize('with_real_shift', [True, False])
def test_flash_attention_with_seq_parallel(input_layout, use_mqa, with_real_shift):
    """
    Features: test FlashAttentionScoreInfo with sequence parallel, sparse_mode=0
    Description: semi_auto_parallel with strategy, seq_parallel
    Expectation: compile success
    """
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode='semi_auto_parallel')
    dp = 2
    mp = 2
    sp = 2
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D,
                                                               input_layout,
                                                               use_mqa,
                                                               with_real_shift)
    net = Net(N, input_layout=input_layout, use_mqa=use_mqa,
              with_real_shift=with_real_shift, dp=dp, mp=mp, sp=sp)
    compile_net(net, query, key, value, real_shift, attn_mask)


@pytest.mark.parametrize('input_layout', ["BSH", "SBH", "BNSD", "BSND"])
@pytest.mark.parametrize('sparse_mode', [2, 3, 4])
def test_flash_attention_compressed_mask_with_seq_parallel(input_layout, sparse_mode):
    """
    Features: test FlashAttentionScoreInfo with sequence parallel, sparse_mode=[2, 3, 4]
    Description: semi_auto_parallel with strategy, seq_parallel
    Expectation: compile success
    """
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode='semi_auto_parallel')
    dp = 2
    mp = 2
    sp = 2
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D,
                                                               input_layout,
                                                               sparse_mode=sparse_mode)
    net = Net(N, input_layout=input_layout, sparse_mode=sparse_mode,
              dp=dp, mp=mp, sp=sp)
    compile_net(net, query, key, value, real_shift, attn_mask)


@pytest.mark.parametrize('input_layout', ["BSH", "SBH", "BNSD", "BSND"])
@pytest.mark.parametrize('use_mqa', [True, False])
@pytest.mark.parametrize('with_real_shift', [True, False])
def test_flash_attention_with_load_balance(input_layout, use_mqa, with_real_shift):
    """
    Features: test FlashAttentionScoreInfo with sequence parallel load balance, sparse_mode=0
    Description: semi_auto_parallel with strategy, seq_parallel and load_balance
    Expectation: compile success
    """
    config = {"enable_flash_attention_load_balance": True,}
    with open("./parallel_speed_up_for_fa.json", "w") as file:
        json.dump(config, file, indent=4, separators=(',', ': '))
    context.set_context(
        ascend_config={"parallel_speed_up_json_path": "./parallel_speed_up_for_fa.json"})
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode='semi_auto_parallel')
    dp = 2
    mp = 2
    sp = 2
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D,
                                                               input_layout,
                                                               use_mqa=use_mqa,
                                                               with_real_shift=with_real_shift)
    net = Net(N, input_layout=input_layout, use_mqa=use_mqa, with_real_shift=with_real_shift,
              dp=dp, mp=mp, sp=sp)
    compile_net(net, query, key, value, real_shift, attn_mask)
    config = {"enable_flash_attention_load_balance": False,}
    with open("./parallel_speed_up_for_fa.json", "w") as file:
        json.dump(config, file, indent=4, separators=(',', ': '))
    context.set_context(
        ascend_config={"parallel_speed_up_json_path": "./parallel_speed_up_for_fa.json"})


@pytest.mark.parametrize('input_layout', ["BSH", "SBH", "BNSD", "BSND"])
@pytest.mark.parametrize('sparse_mode', [2, 3, 4])
def test_flash_attention_compressed_mask_with_load_balance(input_layout, sparse_mode):
    """
    Features: test FlashAttentionScoreInfo with sequence parallel load balance, sparse_mode=[2, 3, 4]
    Description: semi_auto_parallel with strategy, seq_parallel and load_balance
    Expectation: compile success
    """
    config = {"enable_flash_attention_load_balance": True,}
    with open("./parallel_speed_up_for_fa.json", "w") as file:
        json.dump(config, file, indent=4, separators=(',', ': '))
    context.set_context(
        ascend_config={"parallel_speed_up_json_path": "./parallel_speed_up_for_fa.json"})
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode='semi_auto_parallel')
    dp = 2
    mp = 2
    sp = 2
    B, N, S, D = 8, 16, 1024, 128
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D,
                                                               input_layout,
                                                               sparse_mode=sparse_mode)
    net = Net(N, input_layout=input_layout, sparse_mode=sparse_mode,
              dp=dp, mp=mp, sp=sp)
    compile_net(net, query, key, value, real_shift, attn_mask)
    config = {"enable_flash_attention_load_balance": False,}
    with open("./parallel_speed_up_for_fa.json", "w") as file:
        json.dump(config, file, indent=4, separators=(',', ': '))
    context.set_context(
        ascend_config={"parallel_speed_up_json_path": "./parallel_speed_up_for_fa.json"})


def generate_dynamic_inputs(B, N, S, D):
    H = N * D
    query = Tensor(shape=[B, S, H], dtype=ms.float16)
    key = Tensor(shape=[B, S, H], dtype=ms.float16)
    value = Tensor(shape=[B, S, H], dtype=ms.float16)
    attn_mask = Tensor(shape=[B, 1, S, S], dtype=ms.uint8)
    return query, key, value, None, attn_mask


@pytest.mark.parametrize('keep_prob', [0.9])
def test_flash_attention_dynamic_shape_constraint(keep_prob):
    """
    Features: test FlashAttentionScoreInfo dynamic shape
    Description: semi_auto_parallel with strategy
    Expectation: compile failed
    """
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode="semi_auto_parallel", full_batch=False)
    dp = 2
    mp = 4
    B, N, S, D = None, 16, 1024, 128
    inputs = generate_dynamic_inputs(B, N, S, D)
    net = Net(N, keep_prob, dp=dp, mp=mp)
    with pytest.raises(RuntimeError):
        compile_net(net, *inputs)


@pytest.mark.parametrize('is_actual_tuple', [True, False])
@pytest.mark.parametrize('dp_sp_mp', [(2, 2, 2), (4, 1, 2), (1, 4, 2)])
@pytest.mark.parametrize('use_layout', [True, False])
def test_flash_attention_tnd(is_actual_tuple, dp_sp_mp, use_layout):
    """
    Features: test FlashAttentionScoreInfo FlashAttentionScore
    Description: Test for TND layout
    Expectation: compile success if use_layout else raise RuntimeError
    """
    set_auto_parallel_context(device_num=8, global_rank=0)
    context.set_auto_parallel_context(parallel_mode='semi_auto_parallel')
    dp, sp, mp = dp_sp_mp
    B, N, S, D = 8, 16, 1024, 128
    input_layout = "TND"
    sparse_mode = 3
    query, key, value, real_shift, attn_mask = generate_inputs(B, N, S, D,
                                                               input_layout,
                                                               use_mqa=False,
                                                               sparse_mode=sparse_mode,
                                                               with_real_shift=False
                                                               )
    inter = 512
    if is_actual_tuple:
        actual_seq_qlen = tuple(range(inter, B * S + 1, inter))
        actual_seq_kvlen = tuple(range(inter, B * S + 1, inter))
    else:
        actual_seq_qlen = Tensor(np.array(range(inter, B * S + 1, inter), np.int64))
        actual_seq_kvlen = Tensor(np.array(range(inter, B * S + 1, inter), np.int64))
    net = Net(N, input_layout=input_layout, use_mqa=False, keep_prob=1.0, sparse_mode=sparse_mode,
              with_real_shift=False, dp=dp, mp=mp, sp=sp, use_layout=use_layout)

    if sp > 1 and not use_layout:
        # Cannot slice seq-dim if config by strategy
        with pytest.raises(RuntimeError):
            compile_net(net, query, key, value, real_shift, attn_mask, actual_seq_qlen, actual_seq_kvlen)
    else:
        compile_net(net, query, key, value, real_shift, attn_mask, actual_seq_qlen, actual_seq_kvlen)
