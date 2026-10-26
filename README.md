# HevTaskSystem

[![status](https://github.com/heiher/hev-task-system/actions/workflows/build.yaml/badge.svg?branch=main&event=push)](https://github.com/heiher/hev-task-system)

HevTaskSystem is a simple, lightweight multi-task system (coroutines) that
runs on top of native threads.

Each thread that initializes the task system owns an independent scheduler
with an I/O reactor and timer. Any number of tasks can be attached to it,
and they run cooperatively: when a task yields, sleeps, or waits for I/O,
the scheduler picks the highest-priority runnable task and switches context
to it. Tasks within one task system share the memory space, file
descriptors, and other resources of the enclosing native thread, while each
task keeps its private task structure (#HevTask) and a stack allocated on
the heap.

Within a task, you can allocate memory from the heap, read and write data,
and perform I/O operations in a synchronous style - blocking the current
task only, never the underlying thread or the other tasks.

## Features

* Simple/lightweight task.
* I/O operations wrapper.
* Inter-task synchronous. (Mutex/Condition)
* Inter-task communications. (Channel)
* Slice-based memory allocator.
* Call on new stack.
* Stack memory management. (Release unused stack pages)
* Multi-thread support.
* Multi-platform support. (Linux/BSD/macOS/Windows)

## How to Build

### Unix

```bash
git clone https://gitlab.com/hev/hev-task-system
cd hev-task-system
make

# Build shared library
make shared

# Link with librt (only for glibc versions before 2.17)
make LDFLAGS=-lrt

# Disable stack overflow detection
make ENABLE_STACK_OVERFLOW_DETECTION=0

# Set stack backend to heap (Recommended for 32-bit)
make CONFIG_STACK_BACKEND=STACK_HEAP

# Disable sliced memory allocator
make ENABLE_MEMALLOC_SLICE=0

# Disable I/O splice by splice syscall (for old Linux kernel)
make ENABLE_IO_SPLICE_SYSCALL=0

# Set scheduler I/O poll interval in nanoseconds (default: 50000, 0 polls on every schedule pass)
make CONFIG_SCHED_IO_POLL_INTERVAL=0

# Demos
make apps

# Tests
make tests

# Benchmarks
make benches
```

### Android

```bash
mkdir hev-task-system
cd hev-task-system
git clone https://gitlab.com/hev/hev-task-system jni
ndk-build
```

### iOS and MacOS

```bash
git clone https://gitlab.com/hev/hev-task-system
cd hev-task-system
# will generate HevTaskSystem.xcframework
./build-apple.sh
```

### Native Visual C++ / MSVC (Windows x64)

For native MSVC, use the scripts at the project root (PowerShell is required):

```bat
build-msvc.bat -Target all
build-msvc.bat -Target run-tests -TestTimeoutSeconds 60
build-msvc.bat -Target all -Arch arm64
```

Requires **Visual Studio 2022 17.5+** (or newer) and **clang-cl** in `PATH` with the MSVC/Windows SDK components.

### Windows (MSYS2)

```bash
export MSYS=winsymlinks:native
git clone https://gitlab.com/hev/hev-task-system
cd hev-task-system
make
```

## Demos

1. [simple](https://gitlab.com/hev/hev-task-system/blob/main/apps/simple.c)
1. [channel](https://gitlab.com/hev/hev-task-system/blob/main/apps/channel.c)
1. [timeout](https://gitlab.com/hev/hev-task-system/blob/main/apps/timeout.c)
1. [wakeup](https://gitlab.com/hev/hev-task-system/blob/main/apps/wakeup.c)
1. [echo-server](https://gitlab.com/hev/hev-task-system/blob/main/apps/echo-server.c)
1. [call](https://gitlab.com/hev/hev-task-system/blob/main/apps/call.c)
1. [gtk](https://gitlab.com/hev/hev-task-system/blob/main/apps/gtk.c)
1. [curl](https://gitlab.com/hev/hev-task-system/blob/main/apps/curl.c)

## Contributors

* **hev** - https://hev.cc
* **mike wang** - https://github.com/mikewang000000
* **sergey larin** - https://github.com/cerg2010cerg2010
* **yujinpan** - https://github.com/yujinpan

## License

MIT
