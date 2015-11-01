ACLOCAL_AMFLAGS = -I m4
lib_LTLIBRARIES = modt-c++.la
modt_c___la_SOURCES = modt-c++.cpp emacs-module.cpp
modt_c___la_CPPFLAGS = -I../../src
modt_c___la_CXXFLAGS = -ggdb3 -pedantic-errors -std=c++14 -stdlib=libc++ -Wall -Wextra -Weverything -Werror -Wno-c++98-compat-pedantic -Wno-padded -Wno-weak-vtables
modt_c___la_LDFLAGS = -stdlib=libc++ -lc++ -module

TESTS = test.sh
test.sh: modt-c++.la
