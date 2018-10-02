#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "thread/ThreadId.hh"
#include "thread/ThreadStack.hh"

// OS related
#include "runtime/ExecutionContext.hh"
#include "runtime/Portal.hh"

namespace prothos {
  class ThreadState final {
  public:
    explicit ThreadState(std::function<void(void)>&&);

    ThreadId id;
    ThreadStack stack;
    std::function<void(void)> func;
    std::atomic<bool> finished;
    mythos::ExecutionContext ec;
    mythos::Portal portal;
  };
} // namespace prothos