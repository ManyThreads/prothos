#include "thread/ThreadStack.hh"

#include "runtime/mlog.hh"
#include "util/assert.hh"

namespace prothos {
  ThreadStack::ThreadStack(size_t size)
      : m_size(size), m_stack(std::make_unique<byte[]>(size)) {
    ASSERT(m_stack != nullptr);
    MLOG_INFO(mlog::app, "thread stack allocated at:", DVARhex(&m_stack));
  }

  byte* ThreadStack::top() { return m_stack.get() + m_size; }
  byte* ThreadStack::bottom() { return m_stack.get(); }
} // namespace prothos