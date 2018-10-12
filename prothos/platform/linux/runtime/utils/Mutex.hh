#pragma once

#include <mutex>

typedef std::mutex Mutex;

typedef std::lock_guard<std::mutex> Lock;
