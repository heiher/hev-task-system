/*
 ============================================================================
 Name        : hev-compiler.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2019 everyone.
 Description : Compiler
 ============================================================================
 */

#ifndef __HEV_COMPILER_H__
#define __HEV_COMPILER_H__

/* MSVC-ABI Windows targets: cl.exe and clang-cl (which defines both _WIN32
 * and _MSC_VER).  mingw-w64 defines _WIN32 but is not a supported target
 * and must not enter the MSVC-ABI code paths. */
#if defined(_WIN32) && defined(_MSC_VER)
#define HEV_MSVC_ABI 1
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALIGN_UP(addr, align) \
    ((addr + (__typeof__ (addr))align - 1) & ~((__typeof__ (addr))align - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((__typeof__ (addr))align - 1))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))
#endif

#ifndef EXPORT_SYMBOL
#if defined(HEV_MSVC_ABI) && defined(HEV_TASK_SYSTEM_BUILD_SHARED)
#define EXPORT_SYMBOL __declspec (dllexport)
#elif defined(HEV_MSVC_ABI)
/* PE does not export via ELF visibility; symbols stay unexported by default. */
#define EXPORT_SYMBOL
#elif defined(__GNUC__)
#define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
#else
#define EXPORT_SYMBOL
#endif
#endif

#define barrier() __asm__ __volatile__ ("" : : : "memory")

static inline void
__read_once_size (void *dst, const volatile void *src, int size)
{
    switch (size) {
    case sizeof (char):
        *(char *)dst = *(volatile char *)src;
        break;
    case sizeof (short):
        *(short *)dst = *(volatile short *)src;
        break;
    case sizeof (int):
        *(int *)dst = *(volatile int *)src;
        break;
    default:
        barrier ();
        memcpy ((void *)dst, (const void *)src, size);
        barrier ();
    }
}

static inline void
__write_once_size (volatile void *dst, const void *src, int size)
{
    switch (size) {
    case sizeof (char):
        *(volatile char *)dst = *(char *)src;
        break;
    case sizeof (short):
        *(volatile short *)dst = *(short *)src;
        break;
    case sizeof (int):
        *(volatile int *)dst = *(int *)src;
        break;
    default:
        barrier ();
        memcpy ((void *)dst, (const void *)src, size);
        barrier ();
    }
}

/* GNU type-preserving READ_ONCE/WRITE_ONCE implementations; clang-cl and
 * all GNU-compatible compilers support statement expressions and
 * __typeof__ natively. */

#define READ_ONCE(x)                                  \
    ({                                                \
        union                                         \
        {                                             \
            __typeof__ (x) __val;                     \
            char __c[1];                              \
        } __u;                                        \
        __read_once_size (__u.__c, &(x), sizeof (x)); \
        __u.__val;                                    \
    })

#define WRITE_ONCE(x, val)                             \
    ({                                                 \
        union                                          \
        {                                              \
            __typeof__ (x) __val;                      \
            char __c[1];                               \
        } __u = { .__val = (__typeof__ (x))(val) };    \
        __write_once_size (&(x), __u.__c, sizeof (x)); \
        __u.__val;                                     \
    })

#ifndef container_of
#define container_of(ptr, type, member)               \
    ({                                                \
        void *__mptr = (void *)(ptr);                 \
        ((type *)(__mptr - offsetof (type, member))); \
    })
#endif

#ifdef __cplusplus
}
#endif

#endif /* __HEV_COMPILER_H__ */
