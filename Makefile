# Makefile for hev-task-system

PROJECT=hev-task-system

CROSS_PREFIX :=
PP=$(CROSS_PREFIX)cpp
CC=$(CROSS_PREFIX)gcc
LD=$(CROSS_PREFIX)ld
AR=$(CROSS_PREFIX)ar
CCFLAGS=-O3 -pipe -Werror -Wall -I include $(CFLAGS)
LDFLAGS=$(LFLAGS)

CCFLAGS+=$(shell $(CC) -Werror -fno-stack-clash-protection -E -x c /dev/null \
           >/dev/null 2>&1 && echo -fno-stack-clash-protection)
APP_CCFLAGS=$(CCFLAGS)
APP_LDFLAGS=$(LDFLAGS) -L $(BINDIR) -l $(PROJECT) -pthread

SRCDIR=src
BINDIR=bin
APPDIR=apps
TESTDIR=tests
BENCHDIR=benches
BUILDDIR=build

EXEEXT=
STATIC_TARGET=$(BINDIR)/lib$(PROJECT).a
SHARED_TARGET=$(BINDIR)/lib$(PROJECT).so

# Native Windows: clang targeting MSVC, built with the very same rules.
# Run make from a Visual Studio developer prompt (x64, x86 or arm64) with
# a POSIX sh.exe on PATH, e.g. Git for Windows:
#   make ARCH=x64 tests
ifeq ($(OS),Windows_NT)
ifeq ($(MSYSTEM),)
WIN_NATIVE=1
ARCH?=x64
CROSS_TARGET_x64=x86_64
CROSS_TARGET_x86=i686
CROSS_TARGET_arm64=aarch64
CC=clang --target=$(CROSS_TARGET_$(ARCH))-pc-windows-msvc
PP=$(CC)
AR=llvm-ar
EXEEXT=.exe
STATIC_TARGET=$(BINDIR)/$(PROJECT).lib
CCFLAGS+=-DEXPORT_SYMBOL= -D_CRT_SECURE_NO_WARNINGS \
         -D_WINSOCK_DEPRECATED_NO_WARNINGS -Wno-microsoft-enum-forward-reference
APP_CCFLAGS+=-include sys/hev-sys-private.h
APP_LDFLAGS=$(LDFLAGS) -L $(BINDIR) -l $(PROJECT) -lkernel32 -lws2_32
ifneq ($(ARCH),arm64)
# Coroutine switching is incompatible with the CET shadow stack.
APP_LDFLAGS+=-Wl,/CETCOMPAT:NO
endif
endif
endif

$(SHARED_TARGET) : CCFLAGS+=-fPIC -fvisibility=hidden
$(SHARED_TARGET) : LDFLAGS+=-shared -pthread

-include build.mk
-include configs.mk
CCFLAGS+=-I$(SRCDIR) $(CONFIG_CFLAGS)

CCSRCS=$(filter %.c,$(SRCFILES))
ASSRCS=$(filter %.S,$(SRCFILES))
LDOBJS=$(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(CCSRCS)) \
       $(patsubst $(SRCDIR)/%.S,$(BUILDDIR)/%.o,$(ASSRCS))
DEPEND=$(LDOBJS:.o=.dep)

BUILDMSG="\e[1;31mBUILD\e[0m %s\n"
LINKMSG="\e[1;34mLINK\e[0m  \e[1;32m%s\e[0m\n"
CLEANMSG="\e[1;34mCLEAN\e[0m %s\n"
TESTMSG="\e[1;34mTEST\e[0m  \e[1;32m%s\e[0m\n"
BENCHMSG="\e[1;35mBENCH\e[0m \e[1;32m%s\e[0m\n"

ifeq ($(MSYSTEM),MSYS)
	LDFLAGS+=-lmsys-2.0 -lws2_32
endif

V:=
ECHO_PREFIX:=@
ifeq ($(V),1)
    undefine ECHO_PREFIX
endif

.PHONY: static shared clean tests benches apps

static : $(STATIC_TARGET)

shared : $(SHARED_TARGET)

clean :
	$(ECHO_PREFIX) $(RM) -rf $(BINDIR) $(BUILDDIR)
	@printf $(CLEANMSG) $(PROJECT)

$(STATIC_TARGET) : $(LDOBJS)
	$(ECHO_PREFIX) mkdir -p $(dir $@)
	$(ECHO_PREFIX) $(AR) csq $@ $^
	@printf $(LINKMSG) $@

$(SHARED_TARGET) : $(LDOBJS)
	$(ECHO_PREFIX) mkdir -p $(dir $@)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(LDFLAGS)
	@printf $(LINKMSG) $@

$(BUILDDIR)/%.dep : $(SRCDIR)/%.c
	$(ECHO_PREFIX) mkdir -p $(dir $@)
	$(ECHO_PREFIX) $(PP) $(CCFLAGS) -MM -MT$(@:.dep=.o) -MF$@ $< 2>/dev/null

$(BUILDDIR)/%.dep : $(SRCDIR)/%.S
	$(ECHO_PREFIX) mkdir -p $(dir $@)
	$(ECHO_PREFIX) $(PP) $(CCFLAGS) -MM -MT$(@:.dep=.o) -MF$@ $< 2>/dev/null

$(BUILDDIR)/%.o : $(SRCDIR)/%.c
	$(ECHO_PREFIX) mkdir -p $(dir $@)
	$(ECHO_PREFIX) $(CC) $(CCFLAGS) -c -o $@ $<
	@printf $(BUILDMSG) $<

$(BUILDDIR)/%.o : $(SRCDIR)/%.S
	$(ECHO_PREFIX) mkdir -p $(dir $@)
	$(ECHO_PREFIX) $(CC) $(CCFLAGS) -c -o $@ $<
	@printf $(BUILDMSG) $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPEND)
endif


TESTS=$(patsubst $(TESTDIR)/%.c,%,$(wildcard $(TESTDIR)/*.c))
BENCHES=$(patsubst $(BENCHDIR)/%.c,%,$(wildcard $(BENCHDIR)/*.c))
ifeq ($(WIN_NATIVE),1)
# Skip what needs POSIX /dev, openat, UDP socketpair or a peer pthread.
TESTS:=$(filter-out io-open io-openat io-creat io-socket \
	task-dns-fork task-dns-shared task-dns-signal,$(TESTS))
BENCHES:=$(filter-out poll-latency,$(BENCHES))
endif

tests : $(patsubst %,$(BINDIR)/test-%$(EXEEXT),$(TESTS))
	$(ECHO_PREFIX) for test in $^; do printf $(TESTMSG) $$test; $$test; done

$(BINDIR)/test-%$(EXEEXT) : $(TESTDIR)/%.c $(STATIC_TARGET)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(APP_CCFLAGS) $(APP_LDFLAGS)
	@printf $(LINKMSG) $@

benches : $(patsubst %,$(BINDIR)/bench-%$(EXEEXT),$(BENCHES))
	$(ECHO_PREFIX) for bench in $^; do printf $(BENCHMSG) $$bench; $$bench; done

$(BINDIR)/bench-%$(EXEEXT) : $(BENCHDIR)/%.c $(STATIC_TARGET)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(APP_CCFLAGS) $(APP_LDFLAGS)
	@printf $(LINKMSG) $@

apps : \
	$(BINDIR)/simple$(EXEEXT) \
	$(BINDIR)/wakeup$(EXEEXT) \
	$(BINDIR)/channel$(EXEEXT) \
	$(BINDIR)/timeout$(EXEEXT) \
	$(BINDIR)/echo-server$(EXEEXT) \
	$(BINDIR)/call$(EXEEXT)

$(BINDIR)/simple$(EXEEXT) : $(APPDIR)/simple.c $(STATIC_TARGET)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(APP_CCFLAGS) $(APP_LDFLAGS)
	@printf $(LINKMSG) $@

$(BINDIR)/wakeup$(EXEEXT) : $(APPDIR)/wakeup.c $(STATIC_TARGET)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(APP_CCFLAGS) $(APP_LDFLAGS)
	@printf $(LINKMSG) $@

$(BINDIR)/channel$(EXEEXT) : $(APPDIR)/channel.c $(STATIC_TARGET)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(APP_CCFLAGS) $(APP_LDFLAGS)
	@printf $(LINKMSG) $@

$(BINDIR)/timeout$(EXEEXT) : $(APPDIR)/timeout.c $(STATIC_TARGET)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(APP_CCFLAGS) $(APP_LDFLAGS)
	@printf $(LINKMSG) $@

$(BINDIR)/echo-server$(EXEEXT) : $(APPDIR)/echo-server.c $(STATIC_TARGET)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(APP_CCFLAGS) $(APP_LDFLAGS)
	@printf $(LINKMSG) $@

$(BINDIR)/call$(EXEEXT) : $(APPDIR)/call.c $(STATIC_TARGET)
	$(ECHO_PREFIX) $(CC) -o $@ $^ $(APP_CCFLAGS) $(APP_LDFLAGS)
	@printf $(LINKMSG) $@
