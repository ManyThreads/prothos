#include "thread/Thread.hh"

#include <utility>

// OS related
#include "runtime/CapMap.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/PageMap.hh"
#include "runtime/Portal.hh"
#include "runtime/mlog.hh"
#include "runtime/tls.hh"
#include "util/assert.hh"

extern uintptr_t PORTAL_POOL_ADDR;

extern mythos::Portal PORTAL;
extern mythos::KernelMemory KERNEL_MEM;
extern mythos::PageMap ADDRESS_SPACE;
extern mythos::CapMap CAPABILITY_SPACE;

namespace prothos {
  thread_local ThreadState* local_state;

  ThreadState& current_thread() {
    return *local_state;
  }

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
      local_state = state.get();

      DtorGuard guard(state);
      state->func();

      return nullptr;
    }
  } // namespace

  Thread::Thread(std::function<void()>&& func)
      : m_state(std::make_shared<ThreadState>(std::move(func))) {
    mythos::PortalLock lock(PORTAL);

    auto tls = mythos::setupNewTLS();
    auto spawn_result =
        m_state->ec.create(KERNEL_MEM)
            .as(ADDRESS_SPACE)
            .cs(CAPABILITY_SPACE)
            .sched(mythos::init::SCHEDULERS_START + m_state->id)
            .prepareStack(static_cast<void*>(m_state->stack.top()))
            .startFun(thread_main, static_cast<void*>(&m_state))
            .suspended(false)
            .fs(tls)
            .invokeVia(lock)
            .wait();

    ASSERT(!spawn_result.isError());
    MLOG_INFO(mlog::app, "thread spawned...");

    // m_state.portal.setbuf(reinterpret_cast<mythos::InvocationBuf*>(
    //    PORTAL_POOL_ADDR + m_state.id * sizeof(mythos::InvocationBuf)));
    // m_state.portal.create(lock, KERNEL_MEM).wait();
    // m_state.portal.bind(lock, frame, );
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