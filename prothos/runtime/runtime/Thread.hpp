#pragma once

#include <functional>
#include <memory>
#include <thread>

// OS
#include "mythos/init.hh"
#include "mythos/invocation.hh"
#include "runtime/CapMap.hh"
#include "runtime/ExecutionContext.hh"
#include "runtime/KernelMemory.hh"
#include "runtime/mlog.hh"
#include "runtime/PageMap.hh"
#include "runtime/SimpleCapAlloc.hh"
#include "util/assert.hh"

extern mythos::Portal PORTAL;
extern mythos::KernelMemory KERNEL_MEM;
extern mythos::PageMap ADDRESS_SPACE;
extern mythos::CapMap CAPABILITY_SPACE;
extern mythos::SimpleCapAllocDel CAP_ALLOC;

namespace prothos {
namespace thread {
    class Thread;

    extern thread_local Thread local_thread;
}
}

namespace prothos {
namespace thread {
    namespace {
        using byte = uint8_t;

        class ThreadStack final {
        public:
            explicit ThreadStack(size_t size):
                m_size(size),
                m_stack(std::make_unique<byte[]>(0))
            {
                ASSERT(m_stack != nullptr);
            }

            byte* top() {
                return m_stack.get() + m_size;
            }

            byte* bottom() {
                return m_stack.get();
            }

        private:
            size_t m_size;
            std::unique_ptr<byte[]> m_stack;
        };
    }

    using thread_func_type = void* (*)(void*);

    constexpr size_t DEFAULT_STACK_SIZE = 1024 * 1024;

    class ThreadId {
    friend class Thread;
    public:
        ThreadId(const ThreadId&) = delete;
        ThreadId(ThreadId&&) = delete;

        operator uintptr_t&() {
            return m_id;
        }

        operator uintptr_t() const {
            return m_id;
        }

    private:
        ThreadId() {
            static uintptr_t COUNTER = 0;
            m_id = COUNTER++;
        }

        uintptr_t m_id;
    };

    class Thread {
    public:
        Thread() = default;

        Thread(Thread&) = delete;

        Thread(const Thread&) = delete;

        Thread(thread_func_type func):
            m_id(),
            m_stack(DEFAULT_STACK_SIZE),
            m_ec(CAP_ALLOC()),
            m_portal(CAP_ALLOC(), nullptr)
        {
            MLOG_INFO(mlog::app, "thread constructor");
            mythos::PortalLock lock(PORTAL);

            auto tls = mythos::setupNewTLS();
            auto spawn_result = m_ec
                .create(KERNEL_MEM)
                .as(ADDRESS_SPACE)
                .cs(CAPABILITY_SPACE)
                .sched(m_id)
                .prepareStack(static_cast<void*>(m_stack.top()))
                .startFun(func, nullptr)
                .suspended(false)
                .fs(tls)
                .invokeVia(lock)
                .wait();

            // TODO: create Portal
        }

        ~Thread() {
            MLOG_INFO(mlog::app, "thread:", DVAR(m_id), "joined");
            //if joinable terminate()
        }

    private:
        ThreadId m_id;
        ThreadStack m_stack;
        mythos::ExecutionContext m_ec;
        mythos::Portal m_portal;
    };
}
}