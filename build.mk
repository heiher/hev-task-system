# Build

rwildcard=$(foreach d,$(wildcard $1*), \
          $(call rwildcard,$d/,$2) \
          $(filter $(subst *,%,$2),$d))

SRCFILES=$(filter-out $(SRCDIR)/sys/windows/% $(wildcard $(SRCDIR)/arch/*/hev-task-arch-*.c),$(call rwildcard,$(SRCDIR)/,*.c *.S))
