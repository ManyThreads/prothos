#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace prothos {
  using byte = uint8_t;
  constexpr size_t DEFAULT_STACK_SIZE = 1024 * 1024;

  class ThreadStack final {
  public:
    explicit ThreadStack(size_t size = DEFAULT_STACK_SIZE);

    byte* top();
    byte* bottom();

  private:
    size_t m_size;
    std::unique_ptr<byte[]> m_stack;
  };
} // namespace prothos