/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#include "runtime/graph_scheduler/actor/memory_manager_actor.h"
#include "runtime/graph_scheduler/actor/data_source_actor.h"
#include "runtime/graph_scheduler/actor/kernel_actor.h"
#include "mindrt/include/async/async.h"
#include "utils/log_adapter.h"

namespace mindspore {
namespace runtime {
namespace {
void OnMemoryAllocFinish(const AID &from_aid, OpContext<DeviceTensor> *const op_context) {
  if (!ActorDispatcher::get_is_memory_allocation_sync()) {
    ActorDispatcher::Send(from_aid, &MemoryAwareActor::OnMemoryAllocFinish, op_context);
  }
}
}  // namespace

void MemoryManagerActor::AllocateMemory(const std::vector<DeviceTensor *> *alloc_list,
                                        const DeviceContext *device_context, OpContext<DeviceTensor> *const op_context,
                                        const AID &from_aid) {
  MS_EXCEPTION_IF_NULL(alloc_list);
  MS_EXCEPTION_IF_NULL(device_context);
  MS_EXCEPTION_IF_NULL(op_context);

  for (auto &device_tensor : *alloc_list) {
    MS_EXCEPTION_IF_NULL(device_tensor);
    if (device_tensor->GetPtr() != nullptr) {
      continue;
    }
    try {
      // Allocate memory through the device context.
      device::DynamicMemAllocatorDebugInfo::SetDebugInfo(from_aid.Name(), device::AllocatorType::kKernelOutput);
      if (!device_context->device_res_manager_->AllocateMemory(device_tensor, device_tensor->GetSize())) {
        SetOpContextMemoryAllocFail(from_aid.Name(), device_context, device_tensor->GetSize(), op_context);
        return;
      }
    } catch (const std::exception &e) {
      SetOpContextMemoryAllocFail(from_aid.Name(), device_context, device_tensor->GetSize(), op_context);
      return;
    }
  }

  // Call back to the from actor to process after memory allocation finished.
  OnMemoryAllocFinish(from_aid, op_context);
}

void MemoryManagerActor::AllocateContinuousMemory(const std::vector<std::vector<DeviceTensorPtr>> *alloc_list_list,
                                                  const std::vector<std::vector<size_t>> *size_list_list,
                                                  const std::vector<size_t> *total_size_list,
                                                  const std::vector<const DeviceContext *> *device_contexts,
                                                  OpContext<DeviceTensor> *const op_context, const AID &from_aid) {
  MS_EXCEPTION_IF_NULL(alloc_list_list);
  MS_EXCEPTION_IF_NULL(size_list_list);
  MS_EXCEPTION_IF_NULL(total_size_list);
  MS_EXCEPTION_IF_NULL(device_contexts);
  MS_EXCEPTION_IF_NULL(op_context);
  if (((*alloc_list_list).size() != (*size_list_list).size()) ||
      ((*size_list_list).size() != (*total_size_list).size()) ||
      ((*total_size_list).size() != (*device_contexts).size())) {
    SET_OPCONTEXT_FAIL_RET_WITH_ERROR(
      (*op_context), "The size of alloc_list_list, size_list_list, total_size_list and device_contexts are not equal.");
  }

  for (size_t i = 0; i < (*alloc_list_list).size(); ++i) {
    auto &alloc_list = (*alloc_list_list)[i];
    auto &size_list = (*size_list_list)[i];
    auto &device_context = (*device_contexts)[i];
    MS_EXCEPTION_IF_NULL(device_context);
    // Allocate memory through the device context.
    device::DynamicMemAllocatorDebugInfo::SetDebugInfo(from_aid.Name(), device::AllocatorType::kKernelOutput);
    auto dev_ptr_list = device_context->device_res_manager_->AllocateContinuousMemory(size_list);
    if (dev_ptr_list.empty() || dev_ptr_list.size() != alloc_list.size()) {
      MS_LOG(ERROR) << "Allocate continuous memory failed, device ptr list size: " << dev_ptr_list.size()
                    << ", address list size:" << alloc_list.size();
      auto &total_size = (*total_size_list)[i];
      SetOpContextMemoryAllocFail(from_aid.Name(), device_context, total_size, op_context);
      return;
    }

    for (size_t index = 0; index < alloc_list.size(); index++) {
      if (alloc_list[index]->GetPtr() != nullptr) {
        auto old_dev_addr = alloc_list[index];
        auto old_size = old_dev_addr->GetSize();
        if (old_size > size_list[index]) {
          MS_LOG(EXCEPTION) << "Device size of old device address is larger than new device address, " << old_size
                            << " vs " << size_list[index];
        }
        auto new_dev_addr = device_context->device_res_manager_->CreateDeviceAddress(
          dev_ptr_list[index], old_size, old_dev_addr->format(), old_dev_addr->type_id(), old_dev_addr->host_shape());
        new_dev_addr->SyncDeviceToDevice(old_dev_addr.get());
        device_context->device_res_manager_->FreeMemory(old_dev_addr.get());
      }
      alloc_list[index]->set_ptr(dev_ptr_list[index]);
      alloc_list[index]->SetSize(size_list[index]);
      alloc_list[index]->set_from_mem_pool(true);
    }
  }

  // Call back to the from actor to process after memory allocation finished.
  OnMemoryAllocFinish(from_aid, op_context);
}

void MemoryManagerActor::AllocateBatchMemory(const std::vector<DeviceTensor *> *alloc_list,
                                             const std::vector<const DeviceContext *> *device_contexts,
                                             OpContext<DeviceTensor> *const op_context, const AID &from_aid) {
  MS_EXCEPTION_IF_NULL(alloc_list);
  MS_EXCEPTION_IF_NULL(device_contexts);
  MS_EXCEPTION_IF_NULL(op_context);
  if ((*alloc_list).size() != (*device_contexts).size()) {
    SET_OPCONTEXT_FAIL_RET_WITH_ERROR((*op_context),
                                      "The size of alloc list is not equal to the size of device contexts.");
  }

  for (size_t i = 0; i < (*alloc_list).size(); ++i) {
    auto &device_tensor = (*alloc_list)[i];
    auto &device_context = (*device_contexts)[i];
    MS_EXCEPTION_IF_NULL(device_tensor);
    MS_EXCEPTION_IF_NULL(device_context);
    if (device_tensor->GetPtr() != nullptr) {
      continue;
    }

    try {
      // Allocate memory through the device context.
      device::DynamicMemAllocatorDebugInfo::SetDebugInfo(from_aid.Name(), device::AllocatorType::kKernelOutput);
      if (!device_context->device_res_manager_->AllocateMemory(device_tensor, device_tensor->GetSize())) {
        SetOpContextMemoryAllocFail(from_aid.Name(), device_context, device_tensor->GetSize(), op_context);
        return;
      }
    } catch (const std::exception &e) {
      SetOpContextMemoryAllocFail(from_aid.Name(), device_context, device_tensor->GetSize(), op_context);
      return;
    }
  }

  // Call back to the from actor to process after memory allocation finished.
  OnMemoryAllocFinish(from_aid, op_context);
}

void MemoryManagerActor::FreeMemory(const std::vector<DeviceTensor *> *free_list, const DeviceContext *device_context,
                                    OpContext<DeviceTensor> *, const AID &from_aid) {
  MS_EXCEPTION_IF_NULL(free_list);
  for (auto &device_tensor : *free_list) {
    FreeMemoryByRefCount(device_tensor, device_context, from_aid.Name());
  }
}

void MemoryManagerActor::FreeBatchMemory(const std::vector<DeviceTensor *> *free_list,
                                         const std::vector<const DeviceContext *> *device_contexts,
                                         OpContext<DeviceTensor> *const op_context, const AID &from_aid) {
  MS_EXCEPTION_IF_NULL(free_list);
  MS_EXCEPTION_IF_NULL(device_contexts);
  MS_EXCEPTION_IF_NULL(op_context);
  if ((*free_list).size() != (*device_contexts).size()) {
    SET_OPCONTEXT_FAIL_RET_WITH_ERROR((*op_context),
                                      "The size of free list is not equal to the size of device contexts.");
  }

  for (size_t i = 0; i < (*free_list).size(); ++i) {
    auto &device_tensor = (*free_list)[i];
    auto &device_context = (*device_contexts)[i];
    FreeMemoryByRefCount(device_tensor, device_context, from_aid.Name());
  }
}

void MemoryManagerActor::Wait(OpContext<DeviceTensor> *const op_context, const AID &from_aid) {
  // Call back to the from actor to process.
  ActorDispatcher::Send(from_aid, &MemoryAwareActor::OnMemoryAllocFinish, op_context);
}

void MemoryManagerActor::SetOpContextMemoryAllocFail(const std::string &kernel_name,
                                                     const DeviceContext *device_context, size_t alloc_size,
                                                     OpContext<DeviceTensor> *const op_context) {
  MS_EXCEPTION_IF_NULL(device_context);
  MS_EXCEPTION_IF_NULL(op_context);

  std::lock_guard<std::mutex> locker(mem_alloc_failed_mutex_);
  int step_id = op_context->sequential_num_;
  // First occur allocating memory failed.
  if (mem_alloc_failed_step_ids_.find(step_id) == mem_alloc_failed_step_ids_.end()) {
    mem_alloc_failed_step_ids_.clear();
    (void)mem_alloc_failed_step_ids_.insert(step_id);
    SET_OPCONTEXT_MEMORY_ALLOC_FAIL_BY_STRATEGY(GraphExecutionStrategy::kPipeline, *op_context, *device_context,
                                                kernel_name, alloc_size);
  }
}
}  // namespace runtime
}  // namespace mindspore
