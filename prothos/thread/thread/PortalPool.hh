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
    void init();
    mythos::Portal& bind_portal(ThreadId& id, mythos::ExecutionContext& ec);

  private:
    static constexpr size_t POOL_SIZE = 2 * 1024 * 1024;
    static constexpr uintptr_t POOL_ADDR = 26 * 1024 * 1024;

    mythos::Frame m_frame;
    std::vector<std::unique_ptr<mythos::Portal>> m_portals;
  };
} // namespace prothos