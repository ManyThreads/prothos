#include <thread/ThreadState.hh>

#include <utility>

#include "os/OS.hh"

namespace prothos {
  ThreadState::ThreadState(std::function<void(void)>&& func)
      : id(), stack(), func(std::move(func)), finished(false),
        ec(OS::CAP_ALLOC()()) {}
} // namespace prothos