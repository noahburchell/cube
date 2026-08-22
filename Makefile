STD     := gnu23

CFLAGS  ?= -O2
override CFLAGS += -std=$(STD) -Wall -Wextra -Isrc -MMD -MP
LDFLAGS ?=
LDLIBS  := -lm

PREFIX  ?= /usr
BINDIR  ?= $(PREFIX)/bin
DESTDIR ?=

BIN     := cube
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:src/%.c=build/%.o)
DEP     := $(OBJ:.o=.d)

CC_CANDIDATES := gcc clang cc gcc-15 gcc-14 clang-21 clang-20 clang-19
CC_IS_SET     := $(filter-out default undefined file,$(origin CC))
GOALS         := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)

cc-has-std = $(shell echo 'int main(void){return 0;}' \
               | $(1) -std=$(STD) -fsyntax-only -x c - >/dev/null 2>&1 && echo $(1))

ifneq ($(filter-out clean,$(GOALS)),)

ifneq ($(CC_IS_SET),)
ifeq ($(call cc-has-std,$(CC)),)
$(error $(CC) does not support -std=$(STD): C23 needs gcc >= 14 or clang >= 19)
endif
else
CC := $(shell for c in $(CC_CANDIDATES); do \
                echo 'int main(void){return 0;}' \
                  | "$$c" -std=$(STD) -fsyntax-only -x c - >/dev/null 2>&1 \
                  && { echo "$$c"; break; }; \
              done)
ifeq ($(CC),)
$(error no compiler supporting -std=$(STD) found: C23 needs gcc >= 14 or clang >= 19 \
        (tried $(CC_CANDIDATES)); override with 'make CC=/path/to/cc')
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
	install -Dm755 $(BIN) $(DESTDIR)$(BINDIR)/$(BIN)

clean:
	rm -rf build $(BIN)

-include $(DEP)

.PHONY: all install clean
