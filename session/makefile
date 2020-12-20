prefix=/usr/local

export prefix
export DESTDIR

files=$(shell find files -type f)

all:
clean:
install: $(files:%=%@install)

files/%@install:
	file="$(@:files/%@install=%)"; \
	mkdir -p "$(DESTDIR)$(prefix)/$$(dirname "$$file")"; \
	case "$$file" in \
	  *.in) \
	    sed 's/\$$\$$/\x1/g' <"files/$$file" | envsubst | sed 's/\x1/\$$/g' >"$(DESTDIR)$(prefix)/$$(expr "$$file" : "\(.*\).in")" ; \
	    chmod --reference="files/$$file" "$(DESTDIR)$(prefix)/$$(expr "$$file" : "\(.*\).in")"; \
	  ;; \
	  *) cp -a "files/$$file" "$(DESTDIR)$(prefix)/$$file" ;; \
	esac
