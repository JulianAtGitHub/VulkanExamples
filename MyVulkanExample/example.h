#ifndef VK_EXAMPLE_EXAMPLE_H
#define VK_EXAMPLE_EXAMPLE_H

#if defined (_DEBUG) || defined(DEBUG)

#define ENABLE_VALIDATION_LAYERS

#endif

#include <stdio.h>

#define MY_VK_ALLOCATOR NULL

extern void example_init(void);

// start 0.0 from application launch
extern float high_resolution_clock_now(void);

#if defined(_DEBUG) || defined(DEBUG)
extern void runtime_log(const char *format, ...);
#define LOG runtime_log
#else
#define LOG
#endif

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#include "cglm_ext.h"

#endif //VK_EXAMPLE_EXAMPLE_H