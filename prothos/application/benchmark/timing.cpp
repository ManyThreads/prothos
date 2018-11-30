#include "timing.h"
#include <chrono>

std::chrono::milliseconds get_ms_time() {
    return std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );
}

long benchmark::get_time() {
    return get_ms_time().count();
}

clock_t benchmark::get_clock() {
    return clock();
}
