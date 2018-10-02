#pragma once

#include <cstdint>

namespace prothos {
class ThreadId final {
  friend struct ThreadState;

public:
  operator uintptr_t &();
  operator uintptr_t() const;

private:
  // Constructor should be called from friend class only
  ThreadId();

  uintptr_t m_id;
};
}