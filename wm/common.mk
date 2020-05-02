project_root := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
export project_root

BUILD_DIR ?= $(project_root)/build
BIN_NAME = $(notdir $(BIN))

PREFIX = /usr
LIBDPAW_MAJOR = 0
LIBDPAW_MINOR = 0
LIBDPAW_PATCH = 0

CC_OPTS += -Wall -Wextra -pedantic -std=c11
CC_OPTS += -D_DEFAULT_SOURCE
CC_OPTS += -Iinclude/

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
LD_OPTS += -Wl,--gc-sections -Wl,--as-needed
endif

subproject += $(patsubst %/makefile,%,$(wildcard */makefile))

.PHONY: all clean build install

SOURCES += $(shell find src/ -type f -iname "*.c")
HEADERS += $(shell find include/ -type f)

OBJECTS += $(patsubst %.c,$(BUILD_DIR)/%.c.o,$(SOURCES))

all: $(BIN) $(subproject:%=%@build)

$(subproject:%=%@build):
	$(MAKE) -C "$(@:%@build=%)" all

$(subproject:%=%@install):
	$(MAKE) -C "$(@:%@install=%)" install

$(subproject:%=%@clean):
	$(MAKE) -C "$(@:%@clean=%)" clean

$(BIN): $(OBJECTS)
	@mkdir -p "$(dir $@)"
	$(CC) -o "$@" $(LD_OPTS) $(LDFLAGS) $^ $(LOADLIBES) $(LD_LIBS) $(LDLIBS)

$(BUILD_DIR)/%.c.o: %.c $(HEADERS)
	@mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CFLAGS) $(CPPFLAGS) "$<"

clean: $(subproject:%=%@clean)
	rm -rf $(BIN) $(BUILD_DIR)

install: $(subproject:%=%@install)
