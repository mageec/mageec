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
mageechash_LDADD = ../ml/libmageec_learner.la libmageec.la

# Build MAGEEC Database Tool
bin_PROGRAMS += mageecdb
mageecdb_SOURCES = dbtool.cpp
mageecdb_LDADD = ../ml/libmageec_learner.la libmageec.la

# Also build library as a python module
if ENABLE_PYTHON
libmageec_la_SOURCES += python.cpp
libmageec_la_LIBADD += -l$(BOOST_PYTHON_LIB)
libmageec_la_CPPFLAGS = $(PYTHON_CPPFLAGS) $(BOOST_CPPFLAGS)
libmageec_la_LDFLAGS += $(PYTHON_LDFLAGS) $(BOOST_LDFLAGS)

install-exec-hook:
	ln -s -f $(DESTDIR)$(libdir)/libmageec.so $(PYTHON_PACKAGES)/mageec.so
uninstall-hook:
	rm $(PYTHON_PACKAGES)/mageec.so

endif
