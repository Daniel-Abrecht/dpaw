
CC_OPTS += -Wall -Wextra -pedantic -std=c11
CC_OPTS += -Iinclude/ -Ibuild/include/
CC_OPTS += -Werror -g -O0
LD_OPTS += -g

CC_OPTS += -fsanitize=address
LD_OPTS += -fsanitize=address

LD_OPTS += -lX11 -lXinerama -lXi -lXrandr

SOURCES = $(shell find src/ -iname "*.c") $(shell find include/ -iname "*.c")
HEADERS = $(shell find include/ -type f)

OBJECTS = $(patsubst %.c,build/%.c.o,$(SOURCES))

all: bin/dpawin

bin/dpawin: $(OBJECTS)
	mkdir -p "$(dir $@)"
	$(CC) -o "$@" $(LD_OPTS) $^

build/include/%.c.o: CC_OPTS += -DGENERATE_DEFINITIONS
build/%.c.o: %.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CFLAGS) $(CPPFLAGS) "$<"

clean:
	rm -rf build bin

test-run: all
	xinit ./test-xinitrc -- "$$(which Xephyr)" :100 -ac +xinerama -screen 400x600 -host-cursor
