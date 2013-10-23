# Compilation Flags
AM_CFLAGS = -g -O2 -I$(shell $(CC) -print-file-name=plugin)/include \
  -I$(top_srcdir)/include
AM_LDFLAGS =
# Force us to be really specific with what we mean
AM_CFLAGS += -Wall -Wextra -Werror 
AM_CXXFLAGS = $(CXXFLAGS) $(AM_CFLAGS)

# Build GCC Plugin
lib_LTLIBRARIES = libmageec_gcc.la
libmageec_gcc_la_SOURCES = feature-extract.cpp gcc-plugin.cpp \
  gcc-callbacks.cpp plugin-info.cpp
libmageec_gcc_la_LDFLAGS = -release 0.1.0
libmageec_gcc_la_LIBADD = ../mageec/libmageec.la

# GCC plugin specific headers for distribution
EXTRA_DIST = $(srcdir)/mageec-plugin.h
