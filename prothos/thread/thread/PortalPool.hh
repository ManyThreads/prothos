#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "runtime/ExecutionContext.hh"
#include "runtime/Portal.hh"
#include "thread/ThreadId.hh"

namespace prothos {
  class PortalPool final {
  public:
    PortalPool();
    mythos::Portal& assign_portal(mythos::PortalLock&, ThreadId,
                                  mythos::ExecutionContext&);

  private:
    static constexpr size_t POOL_SIZE = 2 * 1024 * 1024;
    static constexpr uintptr_t POOL_ADDR = 26 * 1024 * 1024;

    void init();

    mythos::Frame m_frame;
    std::vector<std::unique_ptr<mythos::Portal>> m_portals;
  };
} // namespace prothos