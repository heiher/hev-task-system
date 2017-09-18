# Makefile for hev-task-system

PROJECT=hev-task-system

# Configs
ENABLE_PTHREAD := 1
ENABLE_STACK_OVERFLOW_DETECTION := 1
ENABLE_MEMALLOC_SLICE := 1

PP=cpp
CC=cc
LD=ld
AR=ar
CCFLAGS=-O3 -Werror -Wall
LDFLAGS=

SRCDIR=src
BINDIR=bin
BUILDDIR=build

ifeq ($(ENABLE_PTHREAD),1)
	CCFLAGS+=-DENABLE_PTHREAD
endif

ifeq ($(ENABLE_STACK_OVERFLOW_DETECTION),1)
	CCFLAGS+=-DENABLE_STACK_OVERFLOW_DETECTION
endif

ifeq ($(ENABLE_MEMALLOC_SLICE),1)
	CCFLAGS+=-DENABLE_MEMALLOC_SLICE
endif

STATIC_TARGET=$(BINDIR)/libhev-task-system.a
SHARED_TARGET=$(BINDIR)/libhev-task-system.so

$(SHARED_TARGET) : CCFLAGS+=-fPIC
$(SHARED_TARGET) : LDFLAGS+=-shared -pthread

CCOBJS = $(wildcard $(SRCDIR)/*.c)
ASOBJS = $(wildcard $(SRCDIR)/*.S)
LDOBJS = $(patsubst $(SRCDIR)%.c,$(BUILDDIR)%.o,$(CCOBJS))
LDOBJS += $(patsubst $(SRCDIR)%.S,$(BUILDDIR)%.o,$(ASOBJS))
DEPEND = $(LDOBJS:.o=.dep)

BUILDMSG="\e[1;31mBUILD\e[0m $<"
LINKMSG="\e[1;34mLINK\e[0m  \e[1;32m$@\e[0m"
CLEANMSG="\e[1;34mCLEAN\e[0m $(PROJECT)"

static : $(STATIC_TARGET)

shared : $(SHARED_TARGET)

clean : 
	@$(RM) $(BINDIR)/* $(BUILDDIR)/*
	@echo -e $(CLEANMSG)

$(STATIC_TARGET) : $(LDOBJS)
	@$(AR) csq $@ $^
	@echo -e $(LINKMSG)

$(SHARED_TARGET) : $(LDOBJS)
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo -e $(LINKMSG)

$(BUILDDIR)/%.dep : $(SRCDIR)/%.c
	@$(PP) $(CCFLAGS) -MM -MT $(@:.dep=.o) -o $@ $<

$(BUILDDIR)/%.dep : $(SRCDIR)/%.S
	@$(PP) $(CCFLAGS) -MM -MT $(@:.dep=.o) -o $@ $<

$(BUILDDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CCFLAGS) -c -o $@ $<
	@echo -e $(BUILDMSG)

$(BUILDDIR)/%.o : $(SRCDIR)/%.S
	@$(CC) $(CCFLAGS) -c -o $@ $<
	@echo -e $(BUILDMSG)

-include $(DEPEND)
