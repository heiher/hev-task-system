/* hev-win-time.c - clock_gettime() on top of QueryPerformanceCounter */

#define HEV_WIN_RAW
#include "hev-sys-private.h"

int
hev_win_clock_gettime (int clock_id, struct timespec *ts)
{
    LARGE_INTEGER freq, counter;

    (void)clock_id;

    if (!ts)
        return -1;

    if (!QueryPerformanceFrequency (&freq) ||
        !QueryPerformanceCounter (&counter))
        return -1;

    ts->tv_sec = (time_t)(counter.QuadPart / freq.QuadPart);
    ts->tv_nsec = (long)((counter.QuadPart % freq.QuadPart) * 1000000000LL /
                         freq.QuadPart);

    return 0;
}
