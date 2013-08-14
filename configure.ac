#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(mageec, 0.1.0)
AM_INIT_AUTOMAKE()
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([mageec/mageec.cpp])
AC_CONFIG_SRCDIR([gcc_plugin/gcc-plugin.cpp])
AC_CONFIG_SRCDIR([ml/ml.cpp])

# Disable static libraries by default for now
LT_INIT([disable-static])

# Checks for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_ENABLE_SHARED

# Doxygen
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN"; then
	AC_MSG_WARN([Doxygen not found, will not generate Doxygen config.])
fi
AM_CONDITIONAL([HAVE_DOXYGEN],
[test -n "$DOXYGEN"])AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])

# Output files
AC_OUTPUT(Makefile mageec/Makefile ml/Makefile gcc_plugin/Makefile)
