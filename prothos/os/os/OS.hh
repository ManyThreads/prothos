#pragma once

// Prothos runtime
#include "thread/PortalPool.hh"

#include "mythos/PciMsgQueueMPSC.hh"
#include "mythos/init.hh"
#include "mythos/invocation.hh"
#include "mythos/protocol/CpuDriverKNC.hh"
#include "runtime/CapMap.hh"
#include "runtime/Example.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/PageMap.hh"
#include "runtime/Portal.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "runtime/mlog.hh"
#include "runtime/tls.hh"
#include "runtime/umem.hh"
#include "util/optional.hh"

extern mythos::InvocationBuf* msg_ptr;

namespace OS {
  // using prothos::PortalPool;

  inline mythos::Portal& PORTAL() {
    /*static*/ mythos::Portal PORTAL(mythos::init::PORTAL, msg_ptr);

    return PORTAL;
  }

  // requires heap init!!
  inline prothos::PortalPool& PORTAL_POOL();

  inline mythos::CapMap& CAPABILITY_SPACE() {
    /*static*/ mythos::CapMap CAPABILITY_SPACE(mythos::init::CSPACE);

    return CAPABILITY_SPACE;
  }

  inline mythos::PageMap& ADDRESS_SPACE() {
    /*static*/ mythos::PageMap ADDRESS_SPACE(mythos::init::PML4);

    return ADDRESS_SPACE;
  }

  inline mythos::KernelMemory& KERNEL_MEM() {
    /*static*/ mythos::KernelMemory KERNEL_MEM(mythos::init::KM);

    return KERNEL_MEM;
  }

  inline mythos::SimpleCapAlloc& CAP_ALLOC() {
    /*static*/ mythos::SimpleCapAllocDel CAP_ALLOC(
        OS::PORTAL(), OS::CAPABILITY_SPACE(), mythos::init::APP_CAP_START,
        mythos::init::SIZE - mythos::init::APP_CAP_START);

    return CAP_ALLOC;
  }
} // namespace OS