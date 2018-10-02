#pragma once

#include <functional>
#include <memory>

#include "thread/ThreadId.hh"
#include "thread/ThreadState.hh"

namespace prothos {
  extern thread_local ThreadState* local_state;
  ThreadState& current_thread();

  /** Handle for thread */
  class Thread {
  public:
    explicit Thread(std::function<void()>&&);
    ~Thread();
    ThreadState& native_handle();
    void detach();
    bool joinable();
    void join();

  private:
    std::shared_ptr<ThreadState> m_state;
    bool m_detached = false;
  };
} // namespace prothos