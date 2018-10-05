#pragma once

// Prothos runtime
#include "thread/PortalPool.hh"

#include "runtime/CapMap.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/PageMap.hh"
#include "runtime/Portal.hh"
#include "runtime/SimpleCapAlloc.hh"

extern mythos::InvocationBuf* msg_ptr;

namespace OS {
  void init_all();
  void init_heap();
  mythos::Portal& PORTAL();
  prothos::PortalPool& PORTAL_POOL();
  mythos::CapMap& CAPABILITY_SPACE();
  mythos::PageMap& ADDRESS_SPACE();
  mythos::KernelMemory& KERNEL_MEM();
  mythos::SimpleCapAlloc& CAP_ALLOC();
} // namespace OS