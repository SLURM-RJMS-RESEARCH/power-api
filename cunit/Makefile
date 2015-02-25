###############################################################################
#
# Makefile for the power api tests.
#
# Author: Benoit Pradelle <pradelle@reservoir.com>
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


CC=gcc
CFLAGS=-O3 -std=c99 -Wall -I../include
LDFLAGS=-lcunit -L../lib -lpower-api -Wl,-rpath=$(realpath ../lib)

TEST=../bin/test-driver
TEST_SOURCES := $(wildcard *.c)
TEST_OBJECTS := $(TEST_SOURCES:.c=.o)

.PHONY: all clean distclean

all: $(TEST)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@


$(TEST): $(TEST_OBJECTS)
	mkdir -p ../bin
	$(CC) $(TEST_OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm -f *.o

distclean: clean
	rm -f $(TEST)

