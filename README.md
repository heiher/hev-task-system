# HevTaskSystem

HevTaskSystem is a simple, lightweight multi-task system (coroutines) for Linux.

The task system is executed by a Linux process/thread. In task system, you can
creates many tasks and attachs to task system. When a task yield or blocked by
I/O, the scheduler will pick a suitable task from running list and switch to.
The memory space, file descriptors and other resources are shared for all tasks
in task system. Every task has a private, standalone task structure (#HevTask)
and stack in heap of the process.

In task, you can allocate memory from heap, read and write data from to stack,
and do I/O operations in synchronized mode.

## How to Build

**Linux**:
```bash
git clone https://gitlab.com/hev/hev-task-system
cd hev-task-system
make

# demos
make apps

# tests
make tests
```

**Android**:
```bash
mkdir hev-task-system
cd hev-task-system
git clone https://gitlab.com/hev/hev-task-system jni
ndk-build
```

## Demos
1. [simple](https://gitlab.com/hev/hev-task-system/blob/master/apps/simple.c)
1. [timeout](https://gitlab.com/hev/hev-task-system/blob/master/apps/timeout.c)
1. [wakeup](https://gitlab.com/hev/hev-task-system/blob/master/apps/wakeup.c)
1. [echo-server](https://gitlab.com/hev/hev-task-system/blob/master/apps/echo-server.c)
1. [gtk](https://gitlab.com/hev/hev-task-system/blob/master/apps/gtk.c)
1. [curl](https://gitlab.com/hev/hev-task-system/blob/master/apps/curl.c)

## Authors
* **Heiher** - https://hev.cc

## License
LGPL

