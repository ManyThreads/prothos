#include "thread/ThreadId.hh"

#include <atomic>
#include <cstdint>

// OS related
#include "runtime/mlog.hh"

namespace prothos {
  ThreadId::operator uintptr_t&() {
    return m_id;
  }

  ThreadId::operator uintptr_t() const {
    return m_id;
  }

  /** private */

  /** Thread IDs start at 1 */
  ThreadId::ThreadId() {
    static std::atomic<uintptr_t> COUNTER(1);
    m_id = COUNTER.fetch_add(1);
  }
} // namespace prothos