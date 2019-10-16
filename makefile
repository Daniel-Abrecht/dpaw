
CC_OPTS += -Wall -Wextra -pedantic -std=c99
CC_OPTS += -Iinclude/ -Ibuild/include/
CC_OPTS += -Werror -g -Og

LD_OPTS += -lX11 -g

SOURCES = $(wildcard src/*.c) $(wildcard src/**/*.c)
HEADERS = $(wildcard include/*.h) $(wildcard include/**/*.h)

OBJECTS = $(patsubst src/%.c,build/%.c.o,$(SOURCES))

all: bin/dpawin

bin/dpawin: $(OBJECTS)
	mkdir -p "$(dir $@)"
	$(CC) -o "$@" $(LD_OPTS) $^

build/%.c.o: src/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CFLAGS) $(CPPFLAGS) "$<"

clean:
	rm -rf build bin

test-run: all
	xinit ./test-xinitrc -- "$$(which Xephyr)" :100 -ac -screen 800x600 -host-cursor
