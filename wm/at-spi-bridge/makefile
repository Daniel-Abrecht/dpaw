BIN = ../bin/at-spi-bridge
BUILD_DIR=../build/at-spi-bridge

CC_OPTS += -Ibuild/include/
LD_LIBS += -L../lib -ldpaw-wm
CC_OPTS += $(shell pkgconf --cflags atspi-2)
LD_LIBS += $(shell pkgconf --libs atspi-2)

include ../common.mk

$(BIN): | ../lib/libdpaw-wm.so

../lib/libdpaw-wm.so:
	make -C .. lib/libdpaw-wm.so

install:
	mkdir -p "$(DESTDIR)$(prefix)/bin/"
	cp $(BIN) "$(DESTDIR)$(prefix)/bin/$(BIN_NAME)"
