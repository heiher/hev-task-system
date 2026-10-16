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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#if !defined(_MSC_VER)
#include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#define ALIGN_UP(addr, align) \
    (((uintptr_t)(addr) + (uintptr_t)(align) - 1) & ~((uintptr_t)(align) - 1))
#define ALIGN_DOWN(addr, align) \
    ((uintptr_t)(addr) & ~((uintptr_t)(align) - 1))
#else
#define ALIGN_UP(addr, align) \
    ((addr + (typeof (addr))align - 1) & ~((typeof (addr))align - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((typeof (addr))align - 1))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))
#endif

#ifndef EXPORT_SYMBOL
#if defined(_MSC_VER) && (defined(HEV_TASK_SYSTEM_BUILD_SHARED) || defined(HEV_SOCKS5_TUNNEL_BUILD))
#define EXPORT_SYMBOL __declspec (dllexport)
#elif defined(_MSC_VER)
#define EXPORT_SYMBOL
#elif defined(__GNUC__)
#define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
#else
#define EXPORT_SYMBOL
#endif
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#define barrier() _ReadWriteBarrier ()
#else
#define barrier() __asm__ __volatile__ ("" : : : "memory")
#endif

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

/* MSVC has no GNU typeof/statement expressions in its traditional C mode.
 * All current WRITE_ONCE sites store pointers, while READ_ONCE sites use
 * pointer or int-sized counter values.  Keep native accessors typed and
 * reserve GNU's type-preserving implementation for GNU targets. */
#if defined(_MSC_VER)
static inline int hev_read_once_int (const volatile void *p)
{
    return *(const volatile int *)p;
}
static inline unsigned int hev_read_once_uint (const volatile void *p)
{
    return *(const volatile unsigned int *)p;
}
static inline void *hev_read_once_ptr (const volatile void *p)
{
    return *(void *const volatile *)p;
}
static inline void *hev_write_once_ptr (volatile void *p, void *val)
{
    *(void *volatile *)p = val;
    return val;
}
#define READ_ONCE(x) _Generic ((x), \
    int: hev_read_once_int, \
    unsigned int: hev_read_once_uint, \
    default: hev_read_once_ptr) (&(x))
#define WRITE_ONCE(x, val) hev_write_once_ptr (&(x), (void *)(val))

#else /* GNU compatible compilers */

#define READ_ONCE(x)                                  \
    ({                                                \
        union                                         \
        {                                             \
            typeof (x) __val;                         \
            char __c[1];                              \
        } __u;                                        \
        __read_once_size (__u.__c, &(x), sizeof (x)); \
        __u.__val;                                    \
    })

#define WRITE_ONCE(x, val)                             \
    ({                                                 \
        union                                          \
        {                                              \
            typeof (x) __val;                          \
            char __c[1];                               \
        } __u = { .__val = (typeof (x))(val) };        \
        __write_once_size (&(x), __u.__c, sizeof (x)); \
        __u.__val;                                     \
    })

#endif /* _MSC_VER */

#ifndef container_of
#if defined(_MSC_VER)
#define container_of(ptr, type, member)                     \
    ((type *)(void *)((char *)(ptr) - offsetof (type, member)))
#else
#define container_of(ptr, type, member)               \
    ({                                                \
        void *__mptr = (void *)(ptr);                 \
        ((type *)(__mptr - offsetof (type, member))); \
    })
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* __HEV_COMPILER_H__ */
