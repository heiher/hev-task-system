# Configs

ENABLE_DEBUG := 0
ENABLE_STACK_OVERFLOW_DETECTION := 1
ENABLE_MEMALLOC_SLICE := 1
ENABLE_IO_SPLICE_SYSCALL := 1

# Stack backend
# {
#   STACK_HEAP,
#   STACK_MMAP
# }
CONFIG_STACK_BACKEND := STACK_MMAP

# Stack overflow detection
# {
#   1, 1 tag
#   2, 1024 tags
#   3, 2048 tags
# }
CONFIG_STACK_OVERFLOW_DETECTION := 1

CONFIG_MEMALLOC_SLICE_ALIGN := 64
CONFIG_MEMALLOC_SLICE_MAX_SIZE := 4096
CONFIG_MEMALLOC_SLICE_MAX_COUNT := 1000

# Schedule clock
# {
#   CLOCK_NONE,
#   CLOCK_MONOTONIC,
#   CLOCK_MONOTONIC_COARSE
# }
CONFIG_SCHED_CLOCK := CLOCK_NONE

CONFIG_CFLAGS :=

ifeq ($(ENABLE_DEBUG),1)
	CONFIG_CFLAGS+=-g -O0 -DENABLE_DEBUG
endif

ifeq ($(ENABLE_STACK_OVERFLOW_DETECTION),1)
	CONFIG_CFLAGS+=-DENABLE_STACK_OVERFLOW_DETECTION
endif

ifeq ($(ENABLE_MEMALLOC_SLICE),1)
	CONFIG_CFLAGS+=-DENABLE_MEMALLOC_SLICE
endif

ifeq ($(ENABLE_IO_SPLICE_SYSCALL),1)
	CONFIG_CFLAGS+=-DENABLE_IO_SPLICE_SYSCALL
endif

CONFIG_CFLAGS+=-DCONFIG_STACK_BACKEND=$(CONFIG_STACK_BACKEND)
CONFIG_CFLAGS+=-DCONFIG_STACK_OVERFLOW_DETECTION=$(CONFIG_STACK_OVERFLOW_DETECTION)
CONFIG_CFLAGS+=-DCONFIG_MEMALLOC_SLICE_ALIGN=$(CONFIG_MEMALLOC_SLICE_ALIGN)
CONFIG_CFLAGS+=-DCONFIG_MEMALLOC_SLICE_MAX_SIZE=$(CONFIG_MEMALLOC_SLICE_MAX_SIZE)
CONFIG_CFLAGS+=-DCONFIG_MEMALLOC_SLICE_MAX_COUNT=$(CONFIG_MEMALLOC_SLICE_MAX_COUNT)
CONFIG_CFLAGS+=-DCONFIG_SCHED_CLOCK=$(CONFIG_SCHED_CLOCK)
