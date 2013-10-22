# Compilation Flags
AM_CFLAGS = -g -O2 -I$(top_srcdir)/include
AM_LDFLAGS =
# Force us to be really specific with what we mean
AM_CFLAGS += -Wall -Wextra -Werror
AM_CXXFLAGS = $(CXXFLAGS) $(AM_CFLAGS)

# Build MAGEEC Library
lib_LTLIBRARIES = libmageec.la
libmageec_la_SOURCES = mageec.cpp hasher.cpp features.cpp passes.cpp elfhash.cpp
libmageec_la_LDFLAGS = -release 0.1.0
libmageec_la_LIBADD = ../ml/libmageec_learner.la

# Build ELF Hasher
bin_PROGRAMS = mageechash
mageechash_SOURCES = mageechash.cpp
mageechash_LDADD = libmageec.la