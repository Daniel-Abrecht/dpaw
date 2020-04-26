
PLUGINS=$(patsubst %/makefile,../bin/plugin/%.so,$(wildcard */makefile))

all: $(PLUGINS)

clean:
	rm -rf ../bin/plugin/ ../build/plugin/

$(PLUGINS): always
	make -C $(patsubst ../bin/plugin/%.so,%/,$@)

.PHONY: always
