#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([tanidvr], [1.4.1], [])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/config.h.in])
AM_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h],,AC_MSG_FAILURE([absent required header]))

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero gethostbyname gettimeofday memmove memset socket],,AC_MSG_FAILURE([required function not present]))

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
