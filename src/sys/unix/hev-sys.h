/* hev-sys.h - POSIX system layer */

#ifndef __HEV_SYS_H__
#define __HEV_SYS_H__

#include <poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <alloca.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#if defined(__MSYS__)
#include <io.h>
#include "lib/misc/hev-windows-api.h"
#endif

#define HEV_LONGJMP(env, val) _longjmp ((env), (val))

#define HEV_STACK_RANGE_ENTER(task) ((void)0)
#define HEV_STACK_RANGE_LEAVE() ((void)0)

#define HEV_TASK_CALL_EXECUTE(call, entry) hev_task_execute ((call), (entry))

#define HEV_AIDE_FINI() ((void)0)

#define HEV_MUTEX_T pthread_mutex_t
#define HEV_MUTEX_INIT(m) pthread_mutex_init ((m), NULL)
#define HEV_MUTEX_FINI(m) pthread_mutex_destroy (m)
#define HEV_MUTEX_LOCK(m) pthread_mutex_lock (m)
#define HEV_MUTEX_UNLOCK(m) pthread_mutex_unlock (m)

#define HEV_IOCP_HANDLE_T long

#define HEV_FD_TO_HANDLE(fd) ((uintptr_t)get_osfhandle (fd))

#endif /* __HEV_SYS_H__ */
