
ROOT = ../..

CC      = clang++
LD      = clang++
CFLAGS  = -ggdb3 -pedantic-errors -std=c++14 -stdlib=libc++ -Wall -Wextra -Weverything -Werror -Wno-c++98-compat-pedantic -Wno-padded -Wno-weak-vtables -isystem /usr/local/include
LDFLAGS =

all: modt-c++.so modt-c++.doc

modt-c++.so: modt-c++.o emacs-module.o
	$(CC) $(CFLAGS) -shared $(LDFLAGS) -o $@ $^

%.o: %.cpp emacs-module.hpp
	$(CC) $(CFLAGS) -I$(ROOT)/src -fPIC -c $<

%.doc: %.cpp
	$(ROOT)/lib-src/make-docfile $< > $@
