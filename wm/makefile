BIN = bin/dpaw

SOURCES += $(shell find include/-dpaw/ -type f -iname "*.c")

CC_OPTS += -Ibuild/include/

LD_LIBS += -Llib -ldpaw-$(LIBDPAW_MAJOR)
LD_LIBS += -lX11 -lXi -lXrandr -lXRes -ldl

include common.mk

$(BIN): | lib/libdpaw-wm-$(LIBDPAW_MAJOR).so

lib/libdpaw-wm-$(LIBDPAW_MAJOR).so: api@build

build/include/%.c.o: CC_OPTS += -DGENERATE_DEFINITIONS

install:
	mkdir -p "$(DESTDIR)$(PREFIX)/bin/"
	cp bin/dpaw "$(DESTDIR)$(PREFIX)/bin/dpaw"
