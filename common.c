
#include <pci/pci.h>
#include <time.h>

#include "common.h"

void sleep_ns(u64 nanoseconds) {
    struct timespec tv, tvret;

    tv.tv_sec = 0;
    tv.tv_nsec = nanoseconds;
    nanosleep(&tv, &tvret);
}

