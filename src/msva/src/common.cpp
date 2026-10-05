#include <cstdint>
#include <time.h>

namespace msva {

// defined in both headers
int64_t nsTime() {
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return (int64_t) tm.tv_sec * 1000000000LL + tm.tv_nsec;
}

};
