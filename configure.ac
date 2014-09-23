AC_INIT([loftp], 0.0.1)
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_COPYRIGHT([
loftp
])

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG([C++])

AC_MSG_CHECKING([if able to compile lambda expressions with c++ 11])
CPPFLAGS="$CPPFLAGS -std=c++0x"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
  auto a = [] () { };
]])], [AC_MSG_RESULT([yes])], [AC_MSG_ERROR([could not use c++ 11])])

AC_CHECK_HEADERS([portaudio.h],[],[AC_MSG_ERROR([Missing a required header, please install it])])
AC_CHECK_LIB([portaudio], [Pa_GetVersionText], [], [AC_MSG_ERROR([libmicrohttpd not installed])])
AC_CHECK_LIB([mpg123], [mpg123_init], [], [AC_MSG_ERROR([libmicrohttpd not installed])])
AC_CHECK_LIB([curl], [curl_global_init], [], [AC_MSG_ERROR([libcurl not installed])])

AC_MSG_CHECKING([for an available audio device through portaudio])
AC_TRY_RUN([
#include <portaudio.h>
#include <iostream>
int main() {
  int success = Pa_Initialize();
  int device_count =  Pa_GetDeviceCount();
  return device_count > 0 ? 0 : 1;
}], [AC_MSG_RESULT([ok])], [AC_MSG_ERROR([Unable to find an audio device to use with portaudio..])])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
