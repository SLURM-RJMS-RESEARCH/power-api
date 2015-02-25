###############################################################################
#
# Makefile for Power API
#
# Author: Tom Henretty <henretty@reservoir.com>
#         Athanasios Konstantinidis <konstantinidis@reservoir.com>
#
###############################################################################
#
# Copyright 2013-2015 Reservoir Labs, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###############################################################################

#===---------------------------------------------------------------------------
# Variables
#===---------------------------------------------------------------------------

# Use GCC
CC := gcc

# Static and shared libraries
STATIC_LIB := lib/libpower-api.a
SHARED_LIB := lib/libpower-api.so

# Vars for sources etc.
SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard include/*.h)
OBJECTS := $(SOURCES:.c=.o)

TOOLS_TARGET := tools/emeas

# Flags
EXTRA_CFLAGS :=$(shell ./extra_flags.sh --cflags)
EXTRA_LDFLAGS :=$(shell ./extra_flags.sh --ldflags)
CFLAGS = -std=gnu99 -g -O3 -Wall -Wextra -march=native -Iinclude `pkg-config --cflags glib-2.0` $(EXTRA_CFLAGS)

$(SHARED_LIB): CFLAGS += -fPIC -shared
$(STATIC_LIB): CFLAGS += -fPIC

LDFLAGS = `pkg-config --libs glib-2.0` -lrt -g $(EXTRA_LDFLAGS)

#===---------------------------------------------------------------------------
# Targets
#===---------------------------------------------------------------------------
.PHONY: all clean distclean test tools doc docs html latex 

# Libraries
all: $(SHARED_LIB) $(STATIC_LIB)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

$(STATIC_LIB): $(OBJECTS)
	mkdir -p lib
	$(AR) rvs $@ $(OBJECTS) && ranlib $@ 

$(SHARED_LIB): $(OBJECTS)
	mkdir -p lib
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)


# Test driver
test:
	$(MAKE) -C cunit
	sudo modprobe msr
	sudo chmod 666 /dev/cpu/*/msr
	sudo bin/test-driver

# tools
tools: $(SHARED_LIB)
	$(MAKE) -C tools

# Docs
doc: docs
docs: html latex

html: doc/html/index.html
doc/html/index.html: $(SOURCES) $(HEADERS) doc/doxyfiles/html.Doxyfile
	doxygen doc/doxyfiles/html.Doxyfile
	tar czvf doc/html-dev-guide.tar.gz doc/html

latex: doc/latex/refman.pdf
doc/latex/refman.pdf: $(SOURCES) $(HEADERS) doc/doxyfiles/latex.Doxyfile
	doxygen doc/doxyfiles/latex.Doxyfile
	cp doc/data/img/lablogored.pdf doc/latex
	cp doc/data/img/lablogored-crop.pdf doc/latex
	cp doc/data/latex/doxygen.sty doc/latex
	make -C doc/latex
	cp doc/latex/refman.pdf doc/developer-guide.pdf


# Clean up
clean:
	rm -rf $(OBJECTS) doc/html/ doc/html-user doc/latex doc/latex-user
	$(MAKE) -C cunit clean
	$(MAKE) -C tools clean

distclean: clean
	rm -f $(STATIC_LIB) $(SHARED_LIB)
	$(MAKE) -C cunit distclean
	$(MAKE) -C tools distclean
