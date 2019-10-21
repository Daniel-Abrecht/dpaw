
CC_OPTS += -Wall -Wextra -pedantic -std=c99
CC_OPTS += -Iinclude/ -Ibuild/include/
CC_OPTS += -Werror -g -Og

CC_OPTS += -fsanitize=address
LD_OPTS += -fsanitize=address

LD_OPTS += -lX11 -lXinerama -g

SOURCES = $(shell find src/ -iname "*.c")
HEADERS = $(shell find include/ -iname "*.h")

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
	xinit ./test-xinitrc -- "$$(which Xephyr)" :100 -ac +xinerama -screen 400x600 -screen 800x600 -host-cursor
