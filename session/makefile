PREFIX=/usr

export PREFIX
export DESTDIR

files=$(shell find files -type f)

all:
clean:
install: $(files:%=%@install)

files/%@install:
	file="$(@:files/%@install=%)"; \
	mkdir -p "$(DESTDIR)$(PREFIX)/$$(dirname "$$file")"; \
	case "$$file" in \
	  *.in) \
	    sed 's/\$$\$$/\x1/g' <"files/$$file" | envsubst | sed 's/\x1/\$$/g' >"$(DESTDIR)$(PREFIX)/$$(expr "$$file" : "\(.*\).in")" ; \
	    chmod --reference="files/$$file" "$(DESTDIR)$(PREFIX)/$$(expr "$$file" : "\(.*\).in")"; \
          ;; \
	  *) cp -a "files/$$file" "$(DESTDIR)$(PREFIX)/$$file" ;; \
	esac
