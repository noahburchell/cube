CC      ?= gcc
CFLAGS  ?= -O2
CFLAGS  += -std=gnu23 -Wall -Wextra -Isrc -MMD -MP
LDFLAGS ?=
LDLIBS  := -lm

PREFIX  ?= /usr
BINDIR  ?= $(PREFIX)/bin
DESTDIR ?=

BIN     := cube
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:src/%.c=build/%.o)
DEP     := $(OBJ:.o=.d)

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
