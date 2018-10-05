#include "os/OS.hh"

#include "mythos/init.hh"
#include "mythos/invocation.hh"
#include "runtime/mlog.hh"
#include "runtime/umem.hh"

namespace OS {
  // optimized away?
  void init_all() {
    init_heap();
    PORTAL();
    PORTAL_POOL();
    CAPABILITY_SPACE();
    ADDRESS_SPACE();
    KERNEL_MEM();
    CAP_ALLOC();
  }

  void init_heap() {
    MLOG_INFO(mlog::app, "checking heap status...");
    static bool has_heap = false;
    if (!has_heap) {
      MLOG_INFO(mlog::app, "initializing heap... (ONLY ONCE)");
      constexpr size_t HEAP_SIZE = 512 * 1024 * 1024;
      constexpr size_t HEAP_ALIGN = 2 * 1024 * 1024;
      constexpr uintptr_t HEAP_ADDR = 28 * 1024 * 1024;

      mythos::PortalLock lock(OS::PORTAL());
      mythos::Frame frame(OS::CAP_ALLOC()());

      frame.create(lock, OS::KERNEL_MEM(), HEAP_SIZE, HEAP_ALIGN).wait();
      OS::ADDRESS_SPACE().mmap(lock, frame, HEAP_ADDR, HEAP_SIZE, 0x1).wait();

      mythos::heap.addRange(HEAP_ADDR, HEAP_SIZE);

      has_heap = true;
    } else {
      MLOG_INFO(mlog::app, "heap is already initialized");
    }
  }

  mythos::Portal& PORTAL() {
    static mythos::Portal PORTAL(mythos::init::PORTAL, msg_ptr);

    return PORTAL;
  }

  prothos::PortalPool& PORTAL_POOL() {
    OS::init_heap();
    static prothos::PortalPool PORTAL_POOL;

    return PORTAL_POOL;
  }

  mythos::CapMap& CAPABILITY_SPACE() {
    static mythos::CapMap CAPABILITY_SPACE(mythos::init::CSPACE);

    return CAPABILITY_SPACE;
  }

  mythos::PageMap& ADDRESS_SPACE() {
    static mythos::PageMap ADDRESS_SPACE(mythos::init::PML4);

    return ADDRESS_SPACE;
  }

  mythos::KernelMemory& KERNEL_MEM() {
    static mythos::KernelMemory KERNEL_MEM(mythos::init::KM);

    return KERNEL_MEM;
  }

  mythos::SimpleCapAlloc& CAP_ALLOC() {
    static mythos::SimpleCapAllocDel CAP_ALLOC(
        OS::PORTAL(), OS::CAPABILITY_SPACE(), mythos::init::APP_CAP_START,
        mythos::init::SIZE - mythos::init::APP_CAP_START);

    return CAP_ALLOC;
  }
} // namespace OS