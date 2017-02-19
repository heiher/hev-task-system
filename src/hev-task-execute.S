/*
 ============================================================================
 Name        : hev-task-execute.S
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#if defined(__i386__)

# include "hev-task-execute-x86.s"

#elif defined(__x86_64__)

# include "hev-task-execute-x86_64.s"

#elif defined(__mips__)

# if (_MIPS_SIM == _ABI64)
#  include "hev-task-execute-mips64.s"
# else
#  include "hev-task-execute-mips32.s"
# endif

#elif defined(__arm__)

#include "hev-task-execute-arm.s"

#elif defined(__aarch64__)

#include "hev-task-execute-aarch64.s"

#else

# error "Unsupported platform!"

#endif

