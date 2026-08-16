CC      := gcc
CFLAGS  := -std=gnu23 -Wall -Wextra -O2 -Isrc -MMD -MP
LDLIBS  := -lm

BIN     := cube
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:src/%.c=build/%.o)
DEP     := $(OBJ:.o=.d)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(BIN)

-include $(DEP)

.PHONY: all clean
