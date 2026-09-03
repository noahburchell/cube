STD     := gnu23

CFLAGS  ?= -O2
override CFLAGS += -std=$(STD) -Wall -Wextra -Isrc -MMD -MP
LDFLAGS ?=
LDLIBS  := -lm

SAN ?=
ifneq ($(SAN),)
override CFLAGS  += -fsanitize=$(SAN) -fno-omit-frame-pointer -fno-sanitize-recover=all -g -O1
override LDFLAGS += -fsanitize=$(SAN)
endif

PREFIX  ?= /usr
BINDIR  ?= $(PREFIX)/bin
DESTDIR ?=

BIN     := cube
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:src/%.c=build/%.o)
DEP     := $(OBJ:.o=.d)

CC_CANDIDATES := gcc clang cc gcc-15 gcc-14 clang-21 clang-20 clang-19 \
                 gcc15 gcc14 clang20 clang19 egcc
CC_IS_SET     := $(filter-out default undefined file,$(origin CC))
GOALS         := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)

CC_PROBE := \#include <stddef.h>\n\#include <stdckdint.h>\nconstexpr int N = 1000;\nstatic_assert(N == 1000);\n[[maybe_unused]] static void *p = nullptr;\nint main(void){size_t r;struct{int a;}s={};return ckd_mul(&r,(size_t)N,(size_t)2)||s.a;}\n

cc-ok = $(shell printf '$(CC_PROBE)' \
          | $(1) -std=$(STD) -fsyntax-only -x c - >/dev/null 2>&1 && echo $(1))

ifneq ($(filter-out clean,$(GOALS)),)

ifneq ($(CC_IS_SET),)
ifeq ($(call cc-ok,$(CC)),)
$(error $(CC) lacks the C23 features cube uses (constexpr, stdckdint.h): \
        needs gcc >= 14 or clang >= 19)
endif
else
CC := $(shell for c in $(CC_CANDIDATES); do \
                printf '$(CC_PROBE)' \
                  | "$$c" -std=$(STD) -fsyntax-only -x c - >/dev/null 2>&1 \
                  && { echo "$$c"; break; }; \
              done)
ifeq ($(CC),)
$(error no compiler with the C23 features cube uses (constexpr, stdckdint.h) found: \
        needs gcc >= 14 or clang >= 19 (tried $(CC_CANDIDATES)); \
        override with 'make CC=/path/to/cc')
endif
endif

endif

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

install: $(BIN)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 $(BIN) $(DESTDIR)$(BINDIR)/$(BIN)

clean:
	rm -rf build $(BIN)

-include $(DEP)

.PHONY: all install clean
