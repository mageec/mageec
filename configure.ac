#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mageec], [0.1.0])
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

# Options to control compilation
AC_ARG_ENABLE([python], [
  AS_HELP_STRING([--enable-python], [Also build as python module.])
], [], [enable_python=no])

# Libaries we need to build MAGEEC
AC_CHECK_HEADERS(libelf.h gelf.h, [], [
  AC_MSG_ERROR([Unable to find libelf headers.])
])
AC_CHECK_LIB(elf, elf_version, [], [
  AC_MSG_ERROR([Unable to find libelf.])
])
AC_CHECK_HEADERS(gcrypt.h, [], [
  AC_MSG_ERROR([Unable to find libgcrypt headers.])
])
AC_CHECK_LIB(gcrypt, gcry_md_open, [], [
  AC_MSG_ERROR([Unable to find libgcrypt.])
])
AC_CHECK_HEADERS(sqlite3.h, [], [
  AC_MSG_ERROR([Unable to find sqlite3 headers.])
])
AC_CHECK_LIB(sqlite3, sqlite3_open, [], [
  AC_MSG_ERROR([Unable to find sqlite3.])
])

AC_CHECK_HEADERS(`gcc -print-file-name=plugin/include/gcc-plugin.h`, [], [
  AC_MSG_ERROR([Unable to find gcc-plugin headers.])
])

if test x$enable_python == xyes; then
  AX_PYTHON_DEVEL
  AX_BOOST_BASE
  AX_BOOST_PYTHON
  AM_PATH_PYTHON(2.6, [], [
    AC_MSG_ERROR([Cannot find python path.])
  ])
fi
AM_CONDITIONAL([ENABLE_PYTHON], [test x$enable_python == xyes])

# Doxygen
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN"; then
	AC_MSG_WARN([Doxygen not found, will not generate Doxygen config.])
fi
AM_CONDITIONAL([HAVE_DOXYGEN],
[test -n "$DOXYGEN"])AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/Doxyfile])])

# Output files
AC_OUTPUT(Makefile \
          mageec/Makefile \
          ml/Makefile \
          ml/C5/Makefile \
          gcc_plugin/Makefile \
          doc/Makefile \
          include/Makefile \
          testsuite/Makefile \
          testsuite/mageec.tests/Makefile)
