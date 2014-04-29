###############################################################################
#
# Makefile for Power API
#
# Author: Tom Henretty <henretty@reservoir.com>
#         Athanasios Konstantinidis <konstantinidis@reservoir.com>
#
###############################################################################
#
# Copyright 2013-2014 Reservoir Labs, Inc.
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

PAPI_HOME ?= /usr/local/papi-5.2.0
PAPI_LIB := $(PAPI_HOME)/lib

# Static and shared libraries
STATIC_LIB := lib/libpower_api.a
SHARED_LIB := lib/libpower_api.so

# Vars for sources etc.
SOURCES := src/power_api.c
HEADERS := include/power_api.h
OBJECTS := $(SOURCES:.c=.o)

TEST_TARGET := bin/test-driver
TEST_SOURCES := $(wildcard cunit/*.c)
TEST_HEADERS := $(wildcard include/*.h)
TEST_OBJECTS := $(TEST_SOURCES:.c=.o)

# Flags 
SOFLAGS := -fPIC -shared -L./lib -lecount -Wl,-rpath=$(PAPI_LIB)

$(SHARED_LIB): CFLAGS := -std=c99 -O2 -march=native -Iinclude `pkg-config --libs --cflags glib-2.0` $(SOFLAGS)

$(STATIC_LIB): CFLAGS := -std=c99 -O2 -march=native -Iinclude `pkg-config --libs --cflags glib-2.0` -fPIC 

$(TEST_TARGET): CFLAGS := -std=c99 -O2 -Iinclude `pkg-config --libs --cflags glib-2.0`
$(TEST_TARGET): LDFLAGS := -L/usr/lib -lcunit -L./lib -Wl,-rpath=./lib -lpower_api -lglib-2.0

LDFLAGS := -lrt


#===---------------------------------------------------------------------------
# Targets
#===---------------------------------------------------------------------------
.PHONY: all clean test docs html latex 


# Libraries
all: make_lib_dir ecount_lib $(SHARED_LIB) $(STATIC_LIB) 

make_lib_dir:
	mkdir -p lib

ecount_lib: 
	make -f Makefile_ecount PAPI_HOME=$(PAPI_HOME)

$(STATIC_LIB): $(OBJECTS)
	$(AR) rvs $@ $^ && ranlib $@ 

$(SHARED_LIB): $(OBJECTS)
	$(CC) $(CFLAGS) $(SOFLAGS) -o $(SHARED_LIB) $(OBJECTS)


# Test driver
$(TEST_TARGET): $(OBJECTS) $(TEST_OBJECTS)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_OBJECTS) $(LDFLAGS)

test: $(TEST_TARGET)
	sudo modprobe msr
	sudo chmod 666 /dev/cpu/*/msr
	sudo ./bin/test-driver


# Docs
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
	make -f Makefile_ecount clean
	rm -rf $(OBJECTS) $(TEST_OBJECTS) doc/html/ doc/html-user doc/latex doc/latex-user lib/ bin/test-driver

