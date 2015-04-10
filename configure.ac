AC_INIT([loftp], 0.0.1)
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_COPYRIGHT([
This code came from the playground source code repository
for the Loftili application.
Licensed under the MIT license
Copyright Danny Hadley 2015
])

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG([C++])

AC_MSG_CHECKING([if able to compile lambda expressions with c++ 11])
CPPFLAGS="$CPPFLAGS -std=c++0x"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
  auto a = [] () { };
]])], [AC_MSG_RESULT([yes])], [AC_MSG_ERROR([could not use c++ 11])])


AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
