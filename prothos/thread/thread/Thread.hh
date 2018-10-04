#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "thread/ThreadId.hh"
#include "thread/ThreadState.hh"

namespace prothos {
  namespace thread {
    extern thread_local ThreadState* local_state;

    /** Get a reference to the state the current thread */
    ThreadState& current_thread();

    /** Get the number of available hardware threads (scheduling contexts) */
    uintptr_t hardware_concurrency();
  } // namespace thread

  /** Join handle for a thread */
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