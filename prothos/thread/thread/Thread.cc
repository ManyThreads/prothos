#include "thread/Thread.hh"

#include <utility>

// OS related
#include "runtime/mlog.hh"
#include "runtime/tls.hh"
#include "util/assert.hh"

// Prothos runtime
#include "os/OS.hh"

namespace prothos {
  namespace thread {
    thread_local ThreadState* local_state;

    ThreadState& current_thread() {
      return *local_state;
    }

    mythos::Portal& local_portal() {
      *local_state->portal;
    }

    /** Mythos currently does not provide a way to determine the number of
     * scheduling contexts */
    uintptr_t hardware_concurrency() {
      return 4;
    }
  } // namespace thread

  namespace {
    struct DtorGuard {
      explicit DtorGuard(std::shared_ptr<ThreadState>& state) : state(state) {}

      // ensures `finished` is set even in case of exceptions
      ~DtorGuard() {
        MLOG_INFO(mlog::app, "thread finished", DVAR(state->id));
        state->finished.store(true); // Release?
      }

      std::shared_ptr<ThreadState>& state;
    };

    /** thread main function */
    void* thread_main(void* context) {
      MLOG_INFO(mlog::app, "thread main...");

      // take (shared) ownership of thread state
      auto state_ptr = reinterpret_cast<std::shared_ptr<ThreadState>*>(context);
      auto state = *state_ptr;
      ASSERT(state.use_count() == 2);

      // initialize TLS
      thread::local_state = state.get();

      DtorGuard guard(state);
      state->func();

      return nullptr;
    }
  } // namespace

  Thread::Thread(std::function<void()>&& func)
      : m_state(std::make_shared<ThreadState>(std::move(func))) {
    mythos::PortalLock lock(OS::PORTAL());
    auto tls = mythos::setupNewTLS();
    auto spawn_result =
        m_state->ec.create(OS::KERNEL_MEM())
            .as(OS::ADDRESS_SPACE())
            .cs(OS::CAPABILITY_SPACE())
            .sched(mythos::init::SCHEDULERS_START + m_state->id)
            .prepareStack(static_cast<void*>(m_state->stack.top()))
            .startFun(thread_main, static_cast<void*>(&m_state))
            .suspended(false)
            .fs(tls)
            .invokeVia(lock)
            .wait();
    ASSERT(!spawn_result.isError());

    m_state->portal =
        &OS::PORTAL_POOL().assign_portal(lock, m_state->id, m_state->ec);

    // TODO: requires fix in mythos
    auto resume_result = m_state->ec.resume(lock).wait();
    ASSERT(!resume_result.isError());

    MLOG_INFO(mlog::app, "thread spawned...");
  }

  Thread::~Thread() {
    if (!m_detached) {
      join();
    }
  }

  ThreadState& Thread::native_handle() {
    return *m_state.get();
  }

  void Thread::detach() {
    m_detached = true;
  }

  bool Thread::joinable() {
    return !m_detached;
  }

  void Thread::join() {
    if (!m_detached) {
      while (m_state->finished.load() == false) {} // Acquire?
    }
  }
} // namespace prothos