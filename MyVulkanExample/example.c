#include <stdio.h>
#include "example.h"

#ifdef WIN32
#include <Windows.h>
#endif

static LARGE_INTEGER s_frequency;
static LARGE_INTEGER s_start_time;

void example_init(void) {
    QueryPerformanceFrequency(&s_frequency);
    QueryPerformanceCounter(&s_start_time);
}

// https://docs.microsoft.com/en-us/windows/desktop/sysinfo/acquiring-high-resolution-time-stamps
float high_resolution_clock_now(void) {
    LARGE_INTEGER end_time;
    LARGE_INTEGER elapsed_time;
    QueryPerformanceCounter(&end_time);
    elapsed_time.QuadPart = end_time.QuadPart - s_start_time.QuadPart;

    elapsed_time.QuadPart *= 1000000;
    elapsed_time.QuadPart /= s_frequency.QuadPart;

    return (float)((double)elapsed_time.QuadPart / 1000000.0);
}

#if defined(_DEBUG) || defined(DEBUG)
void runtime_log(const char *fmt, ...) {
    static char out[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out, 512, fmt, ap);
    va_end(ap);

#ifdef WIN32
    OutputDebugString(out);
#endif
    printf("%s", out);
}
#endif

