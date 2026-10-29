/*
 ============================================================================
 Name        : hev-sys-private.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2026 hev
 Description : System layer of the library
 ============================================================================
 */

#ifndef __HEV_SYS_PRIVATE_H__
#define __HEV_SYS_PRIVATE_H__

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)
#include "sys/windows/hev-sys-private.h"
#else
#include "sys/unix/hev-sys-private.h"
#endif

#endif /* __HEV_SYS_PRIVATE_H__ */
