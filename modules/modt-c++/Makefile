
ROOT = ../..

CC      = clang++
LD      = clang++
CFLAGS  = -ggdb3 -pedantic-errors -std=c++14 -Wall -Weverything -Werror -Wno-c++98-compat-pedantic -Wno-padded -Wno-weak-vtables -isystem /usr/local/include
LDFLAGS =

all: modt-c++.so modt-c++.doc

%.so: %.o
	$(CC) -shared $(LDFLAGS) -o $@ $<

%.o: %.cpp emacs-module.hpp
	$(CC) $(CFLAGS) -I$(ROOT)/src -fPIC -c $<

%.doc: %.cpp
	$(ROOT)/lib-src/make-docfile $< > $@
