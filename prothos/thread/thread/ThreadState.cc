#include <thread/ThreadState.hh>

#include <utility>

// OS related
#include "runtime/SimpleCapAlloc.hh"

extern mythos::SimpleCapAllocDel CAP_ALLOC;

namespace prothos {
  ThreadState::ThreadState(std::function<void(void)>&& func)
      : id(), stack(), func(std::move(func)), finished(false), ec(CAP_ALLOC()),
        portal(CAP_ALLOC(), nullptr) {}
} // namespace prothos