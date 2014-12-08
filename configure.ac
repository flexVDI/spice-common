AC_PREREQ([2.63])

AC_INIT([spice-common],
        [noversion],
        [spice-devel@lists.freedesktop.org])

AC_CONFIG_SRCDIR([common/bitops.h])
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR([build-aux])

# For automake >= 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# For autoconf <= 2.63
m4_ifndef([AS_VAR_APPEND],
          AC_DEFUN([AS_VAR_APPEND], $1=$$1$2))

# Checks for programs
AM_INIT_AUTOMAKE([1.11 dist-xz no-dist-gzip tar-ustar foreign -Wall -Werror])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])
LT_INIT

AC_PROG_CC
AC_PROG_CC_C99
if test "x$ac_cv_prog_cc_c99" = xno; then
    AC_MSG_ERROR([C99 compiler is required.])
fi
AM_PROG_CC_C_O

SPICE_CHECK_SYSDEPS

# Checks for libraries
AC_CONFIG_SUBDIRS([spice-protocol])
PROTOCOL_CFLAGS='-I ${top_srcdir}/spice-protocol'
AC_SUBST(PROTOCOL_CFLAGS)

PKG_CHECK_MODULES(PIXMAN, pixman-1 >= 0.17.7)
AC_SUBST(PIXMAN_CFLAGS)

SPICE_CHECK_SMARTCARD(SPICE_COMMON)
AC_SUBST(SPICE_COMMON_CFLAGS)
AC_SUBST(SPICE_COMMON_LIBS)

AC_ARG_ENABLE(celt051,
[  --disable-celt051       Disable celt051 audio codec (enabled by default)],,
[enable_celt051="yes"])

if test "x$enable_celt051" = "xyes"; then
    PKG_CHECK_MODULES(CELT051, celt051 >= 0.5.1.1, have_celt051=yes, have_celt051=no)
    AC_SUBST(CELT051_CFLAGS)
    AC_SUBST(CELT051_LIBS)
    AC_SUBST(CELT051_LIBDIR)
else
    have_celt051=no
fi

AM_CONDITIONAL([HAVE_CELT051], [test "x$have_celt051" = "xyes"])
AM_COND_IF([HAVE_CELT051], AC_DEFINE([HAVE_CELT051], 1, [Define if we have celt051 codec]))

PKG_CHECK_MODULES([OPUS], [opus >= 0.9.14], have_opus=yes, have_opus=no)
AM_CONDITIONAL([HAVE_OPUS], [test "x$have_opus" = "xyes"])
AM_COND_IF([HAVE_OPUS], AC_DEFINE([HAVE_OPUS], 1, [Define if we have Opus]))

AC_ARG_ENABLE([opengl],
  AS_HELP_STRING([--enable-opengl=@<:@yes/no@:>@],
                 [Enable opengl support (not recommended) @<:@default=no@:>@]),
  [],
  [enable_opengl="no"])
AM_CONDITIONAL(SUPPORT_GL, test "x$enable_opengl" = "xyes")

if test "x$enable_opengl" = "xyes"; then
   AC_CHECK_LIB(GL, glBlendFunc, GL_LIBS="$GL_LIBS -lGL", enable_opengl=no)
   AC_CHECK_LIB(GLU, gluSphere, GL_LIBS="$GL_LIBS -lGLU", enable_opengl=no)
   AC_DEFINE([USE_OPENGL], [1], [Define to build with OpenGL support])
   AC_DEFINE([GL_GLEXT_PROTOTYPES], [], [Enable GLExt prototypes])

   if test "x$enable_opengl" = "xno"; then
      AC_MSG_ERROR([GL libraries not available])
   fi
fi
AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)

# The End!
AC_CONFIG_FILES([
  Makefile
  common/Makefile
  python_modules/Makefile
])

AH_BOTTOM([
/* argh.. this is evil */
#if defined(FIXME_SERVER_SMARTCARD) && defined(USE_SMARTCARD)
%:undef USE_SMARTCARD
#endif
])

AC_OUTPUT
