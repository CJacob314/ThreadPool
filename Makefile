CXX = g++
CXXFLAGS = -Wall -Wextra -Wpedantic -O3
.DEFAULT_GOAL := all

# Allow overriding this variable to customize installation directory
INCLUDEDIR ?= /usr/local/include

# Header-only lib filename
HEADER = ThreadPool

.PHONY: all clean install uninstall

all:
	@echo -e 'Nothing to compile for templated library\nRun with `sudo make install` to move the header-only library to the directory given by the $$INCLUDEDIR variable\n\t/usr/local/include by default (if not set)'

install:
	install -d $(INCLUDEDIR)
	install -m 644 $(HEADER) $(INCLUDEDIR)

uninstall:
	rm -f $(INCLUDEDIR)/$(HEADER)

clean:
	@echo "Nothing to clean for templated library"
