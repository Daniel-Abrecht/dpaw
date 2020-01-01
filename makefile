
project += $(patsubst %/makefile,%,$(wildcard */makefile))

all: build

build: $(project:%=build@%)
install: $(project:%=install@%)
clean: $(project:%=clean@%)

build@%:
	$(MAKE) -C "$(@:build@%=%)" all

install@%:
	$(MAKE) -C "$(@:install@%=%)" install

clean@%:
	$(MAKE) -C "$(@:clean@%=%)" clean
