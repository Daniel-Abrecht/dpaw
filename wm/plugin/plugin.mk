REFIX=/usr
PROJECT=../..

NAME=$(notdir $(abspath .))

CC_OPTS += -Wall -Wextra -pedantic -std=c11
CC_OPTS += -D_DEFAULT_SOURCE
CC_OPTS += -I$(PROJECT)/include/api/ -I$(PROJECT)/build/include/api/

ifndef RELEASE
CC_OPTS += -Werror -g -Og
LD_OPTS += -g -rdynamic
ifndef NO_ASAN
CC_OPTS += -fsanitize=address
LD_OPTS += -fsanitize=address
endif
endif

CC_OPTS += -fPIC -fvisibility=hidden
LD_OPTS += --shared -Wl,--no-undefined

ifdef RELEASE
CC_OPTS += -ffunction-sections -fdata-sections
LD_OPTS += -Wl,--gc-sections
endif

SOURCES = $(patsubst ./%,%,$(shell find -iname "*.c"))
HEADERS = $(shell find $(PROJECT)/include/api/ -type f)
HEADERS += $(PROJECT)/build/include/api/dpaw/plugin/all.h

OBJECTS = $(patsubst %.c,$(PROJECT)/build/plugin/$(NAME)/%.c.o,$(SOURCES))

all: $(PROJECT)/bin/plugin/$(NAME).so

$(PROJECT)/build/include/api/dpaw/plugin/all.h:
	make -C $(PROJECT) build/include/api/dpaw/plugin/all.h

$(PROJECT)/bin/plugin/$(NAME).so: $(OBJECTS)
	mkdir -p "$(dir $@)"
	$(CC) -o "$@" $(LD_OPTS) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS)

$(PROJECT)/build/plugin/$(NAME)/%.c.o: %.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CFLAGS) $(CPPFLAGS) "$<"

clean:
	rm -rf ../build/plugin/$(NAME) ../bin/plugin/$(NAME)
