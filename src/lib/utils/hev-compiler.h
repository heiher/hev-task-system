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

#ifdef __cplusplus
extern "C" {
#endif

#define barrier() __asm__ __volatile__("" : : : "memory")

static inline void
__copy_once_size (void *dst, const volatile void *src, int size)
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
    case sizeof (long long):
        *(long long *)dst = *(volatile long long *)src;
        break;
    default:
        barrier ();
        __builtin_memcpy ((void *)dst, (const void *)src, size);
        barrier ();
    }
}

#define READ_ONCE(x)                                  \
    ({                                                \
        union                                         \
        {                                             \
            typeof(x) __val;                          \
            char __c[1];                              \
        } __u;                                        \
        __copy_once_size (__u.__c, &(x), sizeof (x)); \
        __u.__val;                                    \
    })

#define WRITE_ONCE(x, val)                            \
    ({                                                \
        union                                         \
        {                                             \
            typeof(x) __val;                          \
            char __c[1];                              \
        } __u = { .__val = (typeof(x)) (val) };       \
        __copy_once_size (&(x), __u.__c, sizeof (x)); \
        __u.__val;                                    \
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
