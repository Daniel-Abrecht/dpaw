PREFIX=/usr

CC_OPTS += -Wall -Wextra -pedantic -std=c11
CC_OPTS += -D_DEFAULT_SOURCE
CC_OPTS += -DDPAW_WM
CC_OPTS += -Iinclude/ -Ibuild/include/

ifndef RELEASE
CC_OPTS += -Werror -g -Og
LD_OPTS += -g -rdynamic
ifndef NO_ASAN
CC_OPTS += -fsanitize=address
LD_OPTS += -fsanitize=address
endif
endif

ifdef RELEASE
CC_OPTS += -ffunction-sections -fdata-sections
LD_OPTS += -Wl,--gc-sections
endif

LDLIBS += -lX11 -lXi -lXrandr -lXRes -ldl

SOURCES = $(shell find src/ -iname "*.c") $(shell find include/ -iname "*.c")
HEADERS = $(shell find include/ -type f)

OBJECTS = $(patsubst %.c,build/%.c.o,$(SOURCES))

.PHONY: all clean

all: bin/dpaw

bin/dpaw: $(OBJECTS)
	mkdir -p "$(dir $@)"
	$(CC) -o "$@" $(LD_OPTS) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS)

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

install:
	mkdir -p "$(DESTDIR)$(PREFIX)/bin/"
	cp bin/dpaw "$(DESTDIR)$(PREFIX)/bin/dpaw"
	mkdir -p "$(DESTDIR)$(PREFIX)/include/"
	cp -r include/api/* "$(DESTDIR)$(PREFIX)/include/"
#	cp -r build/include/api/* "$(DESTDIR)$(PREFIX)/include/"
