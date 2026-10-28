/*
 ============================================================================
 Name        : hev-sys.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2026 hev
 Description : System headers of the public API
 ============================================================================
 */

#ifndef __HEV_SYS_H__
#define __HEV_SYS_H__

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)
#include <hev-sys-windows.h>
#else
#include <hev-sys-unix.h>
#endif

#endif /* __HEV_SYS_H__ */
