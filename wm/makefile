BIN = bin/dpaw

SOURCES += $(shell find include/-dpaw/ -type f -iname "*.c")

CC_OPTS += -Ibuild/include/

LD_LIBS += -Llib -ldpaw-wm
LD_LIBS += -lX11 -lXi -lXrandr -lXRes -ldl

include common.mk

$(BIN): | lib/libdpaw-wm.so

lib/libdpaw-wm.so: api@build

build/include/%.c.o: CC_OPTS += -DGENERATE_DEFINITIONS

install:
	mkdir -p "$(DESTDIR)$(prefix)/bin/"
	install bin/dpaw "$(DESTDIR)$(prefix)/bin/dpaw"
