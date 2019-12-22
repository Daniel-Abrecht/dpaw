
CC_OPTS += -Wall -Wextra -pedantic -std=c11
CC_OPTS += -Iinclude/ -Ibuild/include/
CC_OPTS += -Werror -g -O0 -D_DEFAULT_SOURCE
LD_OPTS += -g -rdynamic

ifndef NO_ASAN
CC_OPTS += -fsanitize=address
LD_OPTS += -fsanitize=address
endif

LD_OPTS += -lX11 -lXi -lXrandr

SOURCES = $(shell find src/ -iname "*.c") $(shell find include/ -iname "*.c")
HEADERS = $(shell find include/ -type f)

OBJECTS = $(patsubst %.c,build/%.c.o,$(SOURCES))

all: bin/dpaw

bin/dpaw: $(OBJECTS)
	mkdir -p "$(dir $@)"
	$(CC) -o "$@" $(LD_OPTS) $^

build/include/%.c.o: CC_OPTS += -DGENERATE_DEFINITIONS
build/%.c.o: %.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CFLAGS) $(CPPFLAGS) "$<"

clean:
	rm -rf build bin

test-run-valgrind:
	@if ldd bin/dpaw | grep -q 'libasan\.so'; \
	then \
	  echo "Program was compiled with ASAN. Valgrind and ASAN won't work together, recompile this with 'make NO_ASAN=1 clean all' first."; \
	  exit 1; \
	fi
	VALGRIND=1 $(MAKE) test-run

test-run: all
	xpra start-desktop --start-via-proxy=no --no-daemon --systemd-run=no --exit-with-children --terminate-children --start-child="$$(realpath test-xinitrc)" --attach --sharing=no --window-close=shutdown
