#ifndef TIMING_H
#define TIMING_H

#include <time.h>
#include <chrono>

/**
 * placeholder for platform dependent implementation
 */

namespace benchmark {

std::chrono::milliseconds get_ms_time() {
    return std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );
}

long get_time() {
    return get_ms_time().count();
}

clock_t get_clock() {
    return clock();
}


}

#endif // TIMING_H
