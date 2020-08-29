/**
 * Copyright 2019 Huawei Technologies Co., Ltd
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

#include <pybind11/operators.h>
#include "backend/kernel_compiler/oplib/oplib.h"
#include "backend/kernel_compiler/oplib/oploader.h"
#include "pipeline/jit/pipeline.h"
#include "frontend/operator/composite/composite.h"
#include "pipeline/pynative/pynative_execute.h"
#include "utils/symbolic.h"
#include "pybind_api/api_register.h"
#include "pipeline/jit/parse/python_adapter.h"
#include "utils/summary/event_writer.h"
#include "utils/config_manager.h"
#include "utils/mpi/mpi_config.h"
#include "frontend/parallel/context.h"
#include "frontend/parallel/costmodel_context.h"
#ifdef ENABLE_GPU_COLLECTIVE
#include "runtime/device/gpu/distribution/collective_init.h"
#else
#include "runtime/device/gpu/distribution/collective_fake_init.h"
#endif
#if (ENABLE_CPU && (ENABLE_D || ENABLE_GPU))
#include "frontend/parallel/ps/util.h"
#endif
namespace py = pybind11;

using EnvInstance = mindspore::EnvInstance;
using ExecutorPy = mindspore::pipeline::ExecutorPy;
using Pipeline = mindspore::pipeline::Pipeline;
using PrimitivePy = mindspore::PrimitivePy;
using MetaFuncGraph = mindspore::MetaFuncGraph;
using EventWriter = mindspore::summary::EventWriter;
using OpLib = mindspore::kernel::OpLib;
using OpInfoLoaderPy = mindspore::kernel::OpInfoLoaderPy;
using ParallelContext = mindspore::parallel::ParallelContext;
using CostModelContext = mindspore::parallel::CostModelContext;
using mindspore::MsCtxParam;

namespace mindspore {
void MsCtxSetParameter(std::shared_ptr<MsContext> ctx, MsCtxParam param, const py::object &value) {
  MS_LOG(DEBUG) << "set param(" << param << ") with value '" << py::str(value) << "' of type '"
                << py::str(value.get_type()) << "'.";
  if (param >= MS_CTX_TYPE_BOOL_BEGIN && param < MS_CTX_TYPE_BOOL_END && py::isinstance<py::bool_>(value)) {
    ctx->set_param<bool>(param, value.cast<bool>());
    return;
  }
  if (param >= MS_CTX_TYPE_INT_BEGIN && param < MS_CTX_TYPE_INT_END && py::isinstance<py::int_>(value)) {
    ctx->set_param<int>(param, value.cast<int>());
    return;
  }
  if (param >= MS_CTX_TYPE_UINT32_BEGIN && param < MS_CTX_TYPE_UINT32_END && py::isinstance<py::int_>(value)) {
    ctx->set_param<uint32_t>(param, value.cast<uint32_t>());
    return;
  }
  if (param >= MS_CTX_TYPE_FLOAT_BEGIN && param < MS_CTX_TYPE_FLOAT_END && py::isinstance<py::float_>(value)) {
    ctx->set_param<float>(param, value.cast<float>());
    return;
  }
  if (param >= MS_CTX_TYPE_STRING_BEGIN && param < MS_CTX_TYPE_STRING_END && py::isinstance<py::str>(value)) {
    ctx->set_param<std::string>(param, value.cast<std::string>());
    return;
  }

  MS_LOG(EXCEPTION) << "Got illegal param " << param << " and value with type " << py::str(value.get_type());
}

py::object MsCtxGetParameter(const std::shared_ptr<MsContext> &ctx, MsCtxParam param) {
  if (param >= MS_CTX_TYPE_BOOL_BEGIN && param < MS_CTX_TYPE_BOOL_END) {
    return py::bool_(ctx->get_param<bool>(param));
  }
  if (param >= MS_CTX_TYPE_INT_BEGIN && param < MS_CTX_TYPE_INT_END) {
    return py::int_(ctx->get_param<int>(param));
  }
  if (param >= MS_CTX_TYPE_UINT32_BEGIN && param < MS_CTX_TYPE_UINT32_END) {
    return py::int_(ctx->get_param<uint32_t>(param));
  }
  if (param >= MS_CTX_TYPE_FLOAT_BEGIN && param < MS_CTX_TYPE_FLOAT_END) {
    return py::float_(ctx->get_param<float>(param));
  }
  if (param >= MS_CTX_TYPE_STRING_BEGIN && param < MS_CTX_TYPE_STRING_END) {
    return py::str(ctx->get_param<std::string>(param));
  }

  MS_LOG(EXCEPTION) << "Got illegal param " << param << ".";
}
}  // namespace mindspore

// Interface with python
PYBIND11_MODULE(_c_expression, m) {
  m.doc() = "MindSpore c plugin";

  auto fns = mindspore::PybindDefineRegister::AllFuncs();
  for (auto &item : fns) {
    item.second(&m);
  }

  // Class Pipeline interface
  (void)py::class_<ExecutorPy, std::shared_ptr<ExecutorPy>>(m, "Executor_")
    .def_static("get_instance", &ExecutorPy::GetInstance, "Executor get_instance.")
    .def("__call__", &ExecutorPy::Run, py::arg("args"), py::arg("phase") = py::str(""), "Executor run function.")
    .def("del_net_res", &ExecutorPy::DelNetRes, py::arg("network_id") = py::str(""), "Delete network resource.")
    .def("get_func_graph", &ExecutorPy::GetFuncGraph, py::arg("phase") = py::str(""), "Get graph pointer.")
    .def("get_func_graph_proto", &ExecutorPy::GetFuncGraphProto, py::arg("phase") = py::str(""),
         py::arg("type") = py::str("onnx_ir"), "Get graph proto string by specifying ir type.")
    .def("compile", &ExecutorPy::Compile, py::arg("obj"), py::arg("args"), py::arg("phase") = py::str(""),
         py::arg("use_vm") = py::bool_(false), "Compile obj by executor.")
    .def("updata_param_node_default_input", &ExecutorPy::UpdataParamNodeDefaultInput, py::arg("phase"),
         py::arg("params"), "Fetch the inputs of Conv or Matmul for quant export.")
    .def("get_parameter_layout", &ExecutorPy::GetParameterLayout, py::arg("phase") = py::str("train"),
         "Get Parameter Tensor Layout Dictionary.")
    .def("get_strategy", &ExecutorPy::GetCNodeStrategy, py::arg("phase") = py::str("train"),
         "Get CNode Strategy Dictionary.")
    .def("get_allreduce_fusion", &ExecutorPy::GetAllreduceFusion, py::arg("phase") = py::str("train"),
         "Get Allreduce Fusion Dictionary.")
    .def("fetch_info_for_quant_export", &ExecutorPy::FetchInfoForQuantExport, py::arg("phase") = py::str("train"),
         "Fetch the inputs of Conv or Matmul for quant export.")
    .def("build_data_graph", &ExecutorPy::BuildGraph, py::arg("build_params"), py::arg("phase") = py::str("train"),
         py::arg("broadcast_params") = py::dict(), "Build data graph.")
    .def("has_compiled", &ExecutorPy::HasCompiled, py::arg("phase") = py::str(""), "get if cell compiled.")
    .def("run_init_graph", &ExecutorPy::RunInitGraph, "Run init Graph.");

  (void)py::class_<EnvInstance, std::shared_ptr<EnvInstance>>(m, "EnvInstance_").def(py::init());

  (void)m.def("generate_key", &mindspore::pipeline::GenerateKey, "Generate the function graph key.");
  (void)m.def("real_run_op", &mindspore::pynative::RunOp, "Run op pynatively.");
  (void)m.def("reset_op_id", &mindspore::pipeline::ResetOpId, "Reset Operator Id");
  (void)m.def("init_hccl", &mindspore::pipeline::InitHccl, "Init Hccl");
  (void)m.def("finalize_hccl", &mindspore::pipeline::FinalizeHccl, "Finalize Hccl");
  (void)m.def("verify_inputs_signature", &mindspore::pipeline::VerifyInputSignature, "Verify input signature.");
  (void)m.def("init_exec_dataset", &mindspore::pipeline::InitExecDataset, py::arg("queue_name"), py::arg("size"),
              py::arg("batch_size"), py::arg("types"), py::arg("shapes"), py::arg("input_indexs"),
              py::arg("phase") = py::str("dataset"), py::arg("need_run") = py::bool_(true), "Init and exec dataset.");
  (void)m.def("random_normal", &mindspore::pipeline::InitRandomNormal, py::arg("mean"), py::arg("stddev"),
              py::arg("outshape"), py::arg("seed"), py::arg("outputtensor"), "InitRandRandom");
  (void)m.def("_set_dataset_mode_config", &mindspore::ConfigManager::SetDatasetModeConfig, "API for set dataset mode.");
  (void)m.def("init_backend", &mindspore::pipeline::InitBackend, "Init Backend.");

  (void)m.def("export_graph", &mindspore::pipeline::ExportGraph, "Export Graph.");

  (void)m.def("ms_ctx_get_param", &mindspore::MsCtxGetParameter, "Get value of specified paramter.");
  (void)m.def("ms_ctx_set_param", &mindspore::MsCtxSetParameter, "Set value for specified paramter.");

  (void)py::enum_<MsCtxParam>(*m, "ms_ctx_param", py::arithmetic())
    .value("auto_mixed_precision_flag", MsCtxParam::MS_CTX_AUTO_MIXED_PRECISION_FLAG)
    .value("check_bprop_flag", MsCtxParam::MS_CTX_CHECK_BPROP_FLAG)
    .value("enable_dump", MsCtxParam::MS_CTX_ENABLE_DUMP)
    .value("enable_dynamic_mem_pool", MsCtxParam::MS_CTX_ENABLE_DYNAMIC_MEM_POOL)
    .value("enable_gpu_summary", MsCtxParam::MS_CTX_ENABLE_GPU_SUMMARY)
    .value("enable_graph_kernel", MsCtxParam::MS_CTX_ENABLE_GRAPH_KERNEL)
    .value("enable_hccl", MsCtxParam::MS_CTX_ENABLE_HCCL)
    .value("enable_loop_sink", MsCtxParam::MS_CTX_ENABLE_LOOP_SINK)
    .value("enable_mem_reuse", MsCtxParam::MS_CTX_ENABLE_MEM_REUSE)
    .value("enable_pynative_hook", MsCtxParam::MS_CTX_ENABLE_PYNATIVE_HOOK)
    .value("enable_pynative_infer", MsCtxParam::MS_CTX_ENABLE_PYNATIVE_INFER)
    .value("enable_reduce_precision", MsCtxParam::MS_CTX_ENABLE_REDUCE_PRECISION)
    .value("enable_sparse", MsCtxParam::MS_CTX_ENABLE_SPARSE)
    .value("enable_task_sink", MsCtxParam::MS_CTX_ENABLE_TASK_SINK)
    .value("ir_fusion_flag", MsCtxParam::MS_CTX_IR_FUSION_FLAG)
    .value("is_multi_graph_sink", MsCtxParam::MS_CTX_IS_MULTI_GRAPH_SINK)
    .value("is_pynative_ge_init", MsCtxParam::MS_CTX_IS_PYNATIVE_GE_INIT)
    .value("precompile_only", MsCtxParam::MS_CTX_PRECOMPILE_ONLY)
    .value("enable_profiling", MsCtxParam::MS_CTX_ENABLE_PROFILING)
    .value("save_graphs_flag", MsCtxParam::MS_CTX_SAVE_GRAPHS_FLAG)
    .value("max_device_memory", MsCtxParam::MS_CTX_MAX_DEVICE_MEMORY)
    .value("execution_mode", MsCtxParam::MS_CTX_EXECUTION_MODE)
    .value("device_target", MsCtxParam::MS_CTX_DEVICE_TARGET)
    .value("graph_memory_max_size", MsCtxParam::MS_CTX_GRAPH_MEMORY_MAX_SIZE)
    .value("print_file_path", MsCtxParam::MS_CTX_PRINT_FILE_PATH)
    .value("profiling_options", MsCtxParam::MS_CTX_PROFILING_OPTIONS)
    .value("save_dump_path", MsCtxParam::MS_CTX_SAVE_DUMP_PATH)
    .value("save_graphs_path", MsCtxParam::MS_CTX_SAVE_GRAPHS_PATH)
    .value("variable_memory_max_size", MsCtxParam::MS_CTX_VARIABLE_MEMORY_MAX_SIZE)
    .value("device_id", MsCtxParam::MS_CTX_DEVICE_ID)
    .value("ge_ref", MsCtxParam::MS_CTX_GE_REF)
    .value("max_call_depth", MsCtxParam::MS_CTX_MAX_CALL_DEPTH)
    .value("tsd_ref", MsCtxParam::MS_CTX_TSD_REF);

  (void)py::class_<mindspore::MsContext, std::shared_ptr<mindspore::MsContext>>(m, "MSContext")
    .def_static("get_instance", &mindspore::MsContext::GetInstance, "Get ms context instance.")
    .def("get_backend_policy", &mindspore::MsContext::backend_policy, "Get backend policy.")
    .def("set_backend_policy", &mindspore::MsContext::set_backend_policy, "Set backend policy.");

  (void)py::class_<mindspore::MpiConfig, std::shared_ptr<mindspore::MpiConfig>>(m, "MpiConfig")
    .def_static("get_instance", &mindspore::MpiConfig::GetInstance, "Get mpi config instance.")
    .def("get_enable_mpi", &mindspore::MpiConfig::enable_mpi, "Get whether enable mpi.")
    .def("set_enable_mpi", &mindspore::MpiConfig::set_enable_mpi, "Set whether to enable mpi.");

  (void)py::class_<ParallelContext, std::shared_ptr<ParallelContext>>(m, "AutoParallelContext")
    .def_static("get_instance", &ParallelContext::GetInstance, "Get auto parallel context instance.")
    .def("get_device_num", &ParallelContext::device_num, "Get device num.")
    .def("set_device_num", &ParallelContext::set_device_num, "Set device num.")
    .def("get_device_num_is_set", &ParallelContext::device_num_is_set, "Get device num is set.")
    .def("get_global_rank", &ParallelContext::global_rank, "Get global rank.")
    .def("set_global_rank", &ParallelContext::set_global_rank, "Set global rank.")
    .def("get_global_rank_is_set", &ParallelContext::global_rank_is_set, "Get global rank is set.")
    .def("get_mirror_mean", &ParallelContext::mirror_mean, "Get mirror mean.")
    .def("set_mirror_mean", &ParallelContext::set_mirror_mean, "Set mirror mean.")
    .def("get_gradient_fp32_sync", &ParallelContext::gradient_fp32_sync, "Get cast before mirror.")
    .def("set_gradient_fp32_sync", &ParallelContext::set_gradient_fp32_sync, "Set cast before mirror.")
    .def("get_loss_repeated_mean", &ParallelContext::loss_repeated_mean, "Get loss repeated mean.")
    .def("set_loss_repeated_mean", &ParallelContext::set_loss_repeated_mean, "Set loss repeated mean.")
    .def("get_parallel_mode", &ParallelContext::parallel_mode, "Get parallel mode.")
    .def("set_parallel_mode", &ParallelContext::set_parallel_mode, "Set parallel mode.")
    .def("get_strategy_search_mode", &ParallelContext::strategy_search_mode, "Get strategy search mode.")
    .def("set_strategy_search_mode", &ParallelContext::set_strategy_search_mode, "Set strategy search mode.")
    .def("set_all_reduce_fusion_split_indices", &ParallelContext::SetAllReduceFusionSplitIndices,
         "Set all reduce fusion split indices.")
    .def("get_all_reduce_fusion_split_indices", &ParallelContext::GetAllReduceFusionSplitIndices,
         "Get all reduce fusion split indices.")
    .def("set_all_reduce_fusion_split_sizes", &ParallelContext::SetAllReduceFusionSplitSizes,
         "Set all reduce fusion split sizes.")
    .def("get_all_reduce_fusion_split_sizes", &ParallelContext::GetAllReduceFusionSplitSizes,
         "Get all reduce fusion split sizes.")
    .def("set_enable_all_reduce_fusion", &ParallelContext::set_enable_all_reduce_fusion,
         "Set enable/disable all reduce fusion.")
    .def("get_enable_all_reduce_fusion", &ParallelContext::enable_all_reduce_fusion,
         "Get enable/disable all reduce fusion.")
    .def("get_parameter_broadcast", &ParallelContext::parameter_broadcast, "Get parameter broadcast.")
    .def("get_parameter_broadcast_is_set", &ParallelContext::parameter_broadcast_is_set,
         "Get parameter broadcast is set.")
    .def("set_parameter_broadcast", &ParallelContext::set_parameter_broadcast, "Set parameter broadcast.")
    .def("set_strategy_ckpt_load_file", &ParallelContext::set_strategy_ckpt_load_file,
         "Set strategy checkpoint load file.")
    .def("set_strategy_ckpt_save_file", &ParallelContext::set_strategy_ckpt_save_file,
         "Set strategy checkpoint save file.")
    .def("get_strategy_ckpt_load_file", &ParallelContext::strategy_ckpt_load_file, "Get strategy checkpoint load file.")
    .def("get_strategy_ckpt_save_file", &ParallelContext::strategy_ckpt_save_file, "Get strategy checkpoint save file.")
    .def("set_full_batch", &ParallelContext::set_full_batch, "Set whether load full batch on each device.")
    .def("get_full_batch", &ParallelContext::full_batch, "Get whether load full batch on each device.")
    .def("set_enable_parallel_optimizer", &ParallelContext::set_enable_parallel_optimizer,
         "Set enable/disable parallel optimizer.")
    .def("get_enable_parallel_optimizer", &ParallelContext::enable_parallel_optimizer,
         "Get enable/disable parallel optimizer.")
    .def("reset", &ParallelContext::Reset, "Reset auto parallel context.");

  (void)py::class_<CostModelContext, std::shared_ptr<CostModelContext>>(m, "CostModelContext")
    .def_static("get_instance", &CostModelContext::GetInstance, "Get cost_model context instance.")
    .def("set_device_memory_capacity", &CostModelContext::set_device_memory_capacity,
         "Set the capacity of device memory.")
    .def("get_device_memory_capacity", &CostModelContext::device_memory_capacity, "Get the capacity of device memory.")
    .def("set_costmodel_alpha", &CostModelContext::set_costmodel_alpha,
         "Set the parameter cost_model_alpha of the DP algorithm.")
    .def("get_costmodel_alpha", &CostModelContext::costmodel_alpha,
         "Get the parameter cost_model_alpha of the DP algorithm.")
    .def("set_costmodel_beta", &CostModelContext::set_costmodel_beta,
         "Set the parameter cost_model_beta of the DP algorithm.")
    .def("get_costmodel_beta", &CostModelContext::costmodel_beta,
         "Get the parameter cost_model_beta of the DP algorithm.")
    .def("set_costmodel_gamma", &CostModelContext::set_costmodel_gamma,
         "Set the parameter cost_model_gamma of the DP algorithm")
    .def("get_costmodel_gamma", &CostModelContext::costmodel_gamma,
         "Get the parameter cost_model_gamma of the DP algorithm.")
    .def("set_costmodel_communi_threshold", &CostModelContext::set_costmodel_communi_threshold,
         "Set the parameter cost_model_communi_threshold of the DP algorithm.")
    .def("get_costmodel_communi_threshold", &CostModelContext::costmodel_communi_threshold,
         "Get the parameter cost_model_communi_threshold of the DP algorithm.")
    .def("set_costmodel_communi_const", &CostModelContext::set_costmodel_communi_const,
         "Set the parameter cost_model_communi_const of the DP algorithm.")
    .def("get_costmodel_communi_const", &CostModelContext::costmodel_communi_const,
         "Get the parameter cost_model_communi_const of the DP algorithm.")
    .def("set_costmodel_communi_bias", &CostModelContext::set_costmodel_communi_bias,
         "Set the parameter cost_model_communi_bias of the DP algorithm.")
    .def("get_costmodel_communi_bias", &CostModelContext::costmodel_communi_bias,
         "Get the parameter cost_model_communi_bias of the DP algorithm.")
    .def("set_multi_subgraphs", &CostModelContext::set_multi_subgraphs, "Set the parameter is_multi_subgraphs.")
    .def("get_multi_subgraphs", &CostModelContext::is_multi_subgraphs, "Get the parameter is_multi_subgraphs.")
    .def("set_run_phase", &CostModelContext::set_run_phase, "Set the flag run_phase.")
    .def("get_run_phase", &CostModelContext::run_phase, "Get the flag run_phase.")
    .def("set_costmodel_allreduce_fusion_algorithm", &CostModelContext::set_costmodel_allreduce_fusion_algorithm,
         "Set the parameter gradient AllReduce fusion algorithm.")
    .def("get_costmodel_allreduce_fusion_algorithm", &CostModelContext::costmodel_allreduce_fusion_algorithm,
         "Get the parameter gradient AllReduce fusion algorithm.")
    .def("set_costmodel_allreduce_fusion_times", &CostModelContext::set_costmodel_allreduce_fusion_times,
         "Set the parameter gradient AllReduce times.")
    .def("get_costmodel_allreduce_fusion_times", &CostModelContext::costmodel_allreduce_fusion_times,
         "Get the parameter gradient AllReduce times.")
    .def("set_costmodel_allreduce_fusion_tail_percent", &CostModelContext::set_costmodel_allreduce_fusion_tail_percent,
         "Set the parameter gradient AllReduce fusion tail percent.")
    .def("get_costmodel_allreduce_fusion_tail_percent", &CostModelContext::costmodel_allreduce_fusion_tail_percent,
         "Get the parameter gradient AllReduce fusion tail percent.")
    .def("set_costmodel_allreduce_fusion_tail_time", &CostModelContext::set_costmodel_allreduce_fusion_tail_time,
         "Set the parameter gradient AllReduce fusion tail time.")
    .def("get_costmodel_allreduce_fusion_tail_time", &CostModelContext::costmodel_allreduce_fusion_tail_time,
         "Get the parameter gradient AllReduce fusion tail time.")
    .def("set_costmodel_allreduce_fusion_allreduce_inherent_time",
         &CostModelContext::set_costmodel_allreduce_fusion_allreduce_inherent_time,
         "Set the parameter gradient AllReduce fusion allreduce inherent time.")
    .def("get_costmodel_allreduce_fusion_allreduce_inherent_time",
         &CostModelContext::costmodel_allreduce_fusion_allreduce_inherent_time,
         "Get the parameter gradient AllReduce fusion allreduce inherent time.")
    .def("set_costmodel_allreduce_fusion_allreduce_bandwidth",
         &CostModelContext::set_costmodel_allreduce_fusion_allreduce_bandwidth,
         "Set the parameter gradient AllReduce fusion allreduce bandwidth.")
    .def("get_costmodel_allreduce_fusion_allreduce_bandwidth",
         &CostModelContext::costmodel_allreduce_fusion_allreduce_bandwidth,
         "Get the parameter gradient AllReduce fusion allreduce bandwidth.")
    .def("set_costmodel_allreduce_fusion_computation_time_parameter",
         &CostModelContext::set_costmodel_allreduce_fusion_computation_time_parameter,
         "Set the parameter gradient AllReduce fusion computation time parameter.")
    .def("get_costmodel_allreduce_fusion_computation_time_parameter",
         &CostModelContext::costmodel_allreduce_fusion_computation_time_parameter,
         "Get the parameter gradient AllReduce fusion computation time parameter.")
    .def("set_tensor_slice_align_enable", &CostModelContext::set_tensor_slice_alignment_enable,
         "Set the parameter tensor_slice_align_enable in strategy generation.")
    .def("get_tensor_slice_align_enable", &CostModelContext::tensor_slice_alignment_enable,
         "Get the parameter tensor_slice_align_enable in strategy generation.")
    .def("set_tensor_slice_align_size", &CostModelContext::set_tensor_slice_alignment_size,
         "Set the parameter tensor_slice_size in strategy generation.")
    .def("get_tensor_slice_align_size", &CostModelContext::tensor_slice_alignment_size,
         "Get the parameter tensor_slice_size in strategy generation.")
    .def("set_fully_use_devices", &CostModelContext::set_fully_use_device,
         "Set the parameter fully_use_devices in the DP algorithm.")
    .def("get_fully_use_devices", &CostModelContext::fully_use_device,
         "Get the parameter fully_use_devices in the DP algorithm.")
    .def("set_elementwise_op_strategy_follow", &CostModelContext::set_elementwise_stra_follow,
         "Set the parameter elementwise_op_strategy_follow in the DP algorithm.")
    .def("get_elementwise_op_strategy_follow", &CostModelContext::elementwise_stra_follow,
         "Get the parameter elementwise_op_strategy_follow in the DP algorithm.")
    .def("reset_cost_model", &CostModelContext::ResetCostModel, "Reset the CostModelContext.")
    .def("reset_algo_parameters", &CostModelContext::ResetAlgoParameters, "Reset the AlgoParameters.");

  (void)py::module::import("atexit").attr("register")(py::cpp_function{[&]() -> void {
    // only in case that c++ calling python interface, ClearResAtexit should be called.
    if (mindspore::parse::python_adapter::IsPythonEnv()) {
      mindspore::pipeline::ClearResAtexit();

#ifdef ENABLE_MINDDATA
      py::module iterators = py::module::import("mindspore.dataset.engine.iterators");
      (void)iterators.attr("_cleanup")();
#endif
    }
  }});

  (void)py::class_<EventWriter, std::shared_ptr<EventWriter>>(m, "EventWriter_")
    .def(py::init<const std::string &>())
    .def("GetFileName", &EventWriter::GetFileName, "Get the file name.")
    .def("Open", &EventWriter::Open, "Open the write file.")
    .def("Write", &EventWriter::Write, "Write the serialize event.")
    .def("EventCount", &EventWriter::GetWriteEventCount, "Write event count.")
    .def("Flush", &EventWriter::Flush, "Flush the event.")
    .def("Close", &EventWriter::Close, "Close the write.")
    .def("Shut", &EventWriter::Shut, "Final close the write.");

  (void)py::class_<OpLib, std::shared_ptr<OpLib>>(m, "Oplib")
    .def(py::init())
    .def_static("reg_op", &OpLib::RegOp, "Register op info.");
#ifdef ENABLE_GPU_COLLECTIVE
  (void)m.def("init_gpu_collective", &mindspore::device::gpu::CollectiveInitializer::InitCollective,
              "Init gpu collective communication mode.");
  (void)m.def("finalize_gpu_collective", &mindspore::device::gpu::CollectiveInitializer::FinalizeCollective,
              "Finalize gpu collective communication mode.");
#else
  (void)m.def("init_gpu_collective", &mindspore::device::gpu::CollectiveFakeInitializer::InitCollective,
              "Init gpu collective communication mode.");
  (void)m.def("finalize_gpu_collective", &mindspore::device::gpu::CollectiveFakeInitializer::FinalizeCollective,
              "Finalize gpu collective communication mode.");
#endif

#if (ENABLE_CPU && (ENABLE_D || ENABLE_GPU))
  (void)m.def("get_ps_mode_rank", &mindspore::parallel::ps::Util::GetRankId, "Get Worker and PServer rank id.");
#endif

  (void)py::class_<OpInfoLoaderPy, std::shared_ptr<OpInfoLoaderPy>>(m, "OpInfoLoaderPy")
    .def(py::init())
    .def("get_all_ops_info", &OpInfoLoaderPy::GetAllOpsInfo, "get all ops info.");
}
