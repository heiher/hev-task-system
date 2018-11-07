/*
 ============================================================================
 Name        : hev-task-execute.S
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2017 everyone.
 Description :
 ============================================================================
 */

#if defined(__i386__)

# include "arch/x86/hev-task-execute-x86.s"

#elif defined(__x86_64__)

# include "arch/x86/hev-task-execute-x86_64.s"

#elif defined(__mips__)

# if (_MIPS_SIM == _ABI64)
#  include "arch/mips/hev-task-execute-mips64.s"
# else
#  include "arch/mips/hev-task-execute-mips32.s"
# endif

#elif defined(__arm__)

# include "arch/arm/hev-task-execute-arm.s"

#elif defined(__aarch64__)

# include "arch/arm/hev-task-execute-aarch64.s"

#elif defined(__sw_64__)

# include "arch/sw64/hev-task-execute-sw64.s"

#elif defined(__riscv) && defined(_LP64)

# include "arch/riscv/hev-task-execute-riscv64.s"

#else

# error "Unsupported platform!"

#endif

