BIN = ../lib/libdpaw-wm.so
BUILD_DIR = ../build/api

CC_OPTS += -fPIC
LD_OPTS += --shared -Wl,-soname,$(BIN_NAME).$(LIBDPAW_MAJOR)
LD_OPTS += -Wl,--no-undefined
LD_LIBS += -lX11

include ../common.mk

all: $(BIN)

install:
	mkdir -p "$(DESTDIR)$(PREFIX)/include/"
	cp -r "include/api/" "$(DESTDIR)$(PREFIX)/include/"
	mkdir -p "$(DESTDIR)$(PREFIX)/lib/"
	cp "$(BIN)" "$(DESTDIR)$(PREFIX)/lib/$(BIN_NAME).$(LIBDPAW_MAJOR).$(LIBDPAW_MINOR).$(LIBDPAW_PATCH)"
	ln -sf "$(BIN_NAME).$(LIBDPAW_MAJOR).$(LIBDPAW_MINOR).$(LIBDPAW_PATCH)" "$(DESTDIR)$(PREFIX)/lib/$(BIN_NAME)"
	ln -sf "$(BIN_NAME).$(LIBDPAW_MAJOR).$(LIBDPAW_MINOR).$(LIBDPAW_PATCH)" "$(DESTDIR)$(PREFIX)/lib/$(BIN_NAME).$(LIBDPAW_MAJOR)"
	ln -sf "$(BIN_NAME).$(LIBDPAW_MAJOR).$(LIBDPAW_MINOR).$(LIBDPAW_PATCH)" "$(DESTDIR)$(PREFIX)/lib/$(BIN_NAME).$(LIBDPAW_MAJOR).$(LIBDPAW_MINOR)"
