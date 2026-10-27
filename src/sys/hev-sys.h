/* hev-sys.h - System headers of the public API */

#ifndef __HEV_SYS_H__
#define __HEV_SYS_H__

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__)
#include "windows/hev-sys.h"
#else
#include "unix/hev-sys.h"
#endif

#endif /* __HEV_SYS_H__ */
