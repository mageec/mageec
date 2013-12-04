SUBDIRS = C5
# Compilation Flags
AM_CFLAGS = -g -O2 -I$(top_srcdir)/include -DLIBEXECDIR='"$(libexecdir)"'
AM_LDFLAGS = 
# Force us to be really specific with what we mean
#AM_CFLAGS += -Wall -Wextra -Werror 
AM_CXXFLAGS = $(CXXFLAGS) $(AM_CFLAGS)

# Build Machine Learning Library
lib_LTLIBRARIES = libmageec_learner.la
libmageec_learner_la_SOURCES = ml.cpp database.cpp
libmageec_learner_la_LDFLAGS = -release 0.1.0
