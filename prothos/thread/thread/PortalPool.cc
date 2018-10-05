#include "thread/PortalPool.hh"

#include <utility>

// Prothos runtime
#include "os/OS.hh"
#include "thread/Thread.hh"

#include "mythos/caps.hh"
#include "mythos/invocation.hh"
#include "runtime/mlog.hh"
#include "util/assert.hh"

namespace prothos {
  PortalPool::PortalPool() : m_frame(OS::CAP_ALLOC()()) {
    init();
  }

  mythos::Portal& PortalPool::assign_portal(mythos::PortalLock& lock,
                                            ThreadId id,
                                            mythos::ExecutionContext& ec) {
    ASSERT(id != 0 && id < thread::hardware_concurrency());
    auto& portal = *(m_portals[id - 1].get());
    auto offset = id * sizeof(mythos::InvocationBuf);

    portal.bind(lock, m_frame, offset, ec.cap()).wait();
    MLOG_INFO(mlog::app, "assigned portal to execution context");

    return portal;
  }

  /** private */

  void PortalPool::init() {
    MLOG_INFO(mlog::app, "initializing PortalPool...");
    mythos::PortalLock lock(OS::PORTAL());

    // map 2mb frame for storing the invocation buffers
    m_frame.create(lock, OS::KERNEL_MEM(), POOL_SIZE, 4096).wait();
    OS::ADDRESS_SPACE().mmap(lock, m_frame, POOL_ADDR, POOL_SIZE, 0x1).wait();

    const auto portal_count = thread::hardware_concurrency() - 1;
    ASSERT(portal_count * sizeof(mythos::InvocationBuf) <= POOL_SIZE);

    m_portals.reserve(portal_count);

    auto* buffer_addr = reinterpret_cast<mythos::InvocationBuf*>(POOL_ADDR);
    for (uintptr_t i = 0; i < portal_count; ++i) {
      new (buffer_addr) mythos::InvocationBuf();
      m_portals.push_back(
          std::make_unique<mythos::Portal>(OS::CAP_ALLOC()(), buffer_addr));
      m_portals[i]->create(lock, OS::KERNEL_MEM()).wait();

      ++buffer_addr;
    }

    MLOG_INFO(mlog::app, "PortalPool initialized");
  }
} // namespace prothos