dnl SPICE_MANUAL
dnl ------------
dnl Check if user wants manuals to be compiled and
dnl if all programs (asciidoc and a2x) are available
dnl ------------
dnl Shell defines:
dnl - have_asciidoc yes or not is asciidoc program is available
dnl Automake macros:
dnl - A2X a2x program or empty
dnl - ASCIIDOC asciidoc program or emtpy
dnl - BUILD_MANUAL if asciidoc and a2x are available
dnl - BUILD_HTML_MANUAL if asciidoc is available (html can be produced)
dnl - BUILD_CHUNKED_MANUAL if a2x is available
AC_DEFUN([SPICE_MANUAL],[
    AC_ARG_ENABLE([manual],
                   AS_HELP_STRING([--enable-manual=@<:@auto/yes/no@:>@],
                                  [Build SPICE manual]),
                   [],
                   [enable_manual="auto"])
    if test "x$enable_manual" != "xno"; then
        AC_PATH_PROG([ASCIIDOC], [asciidoc])
        AS_IF([test -z "$ASCIIDOC" && test "x$enable_manual" = "xyes"],
              [AC_MSG_ERROR([asciidoc is missing and build of manual was requested])])
        AC_PATH_PROG([A2X], [a2x])
        AS_IF([test -z "$A2X" && test "x$enable_manual" = "xyes"],
              [AC_MSG_ERROR([a2x is missing and build of manual was requested])])
    fi
    AS_IF([test -n "$ASCIIDOC"], [have_asciidoc=yes], [have_asciidoc=no])
    AM_CONDITIONAL([BUILD_MANUAL], [test -n "$ASCIIDOC" || test -n "$A2X"])
    AM_CONDITIONAL([BUILD_HTML_MANUAL], [test -n "$ASCIIDOC"])
    AM_CONDITIONAL([BUILD_CHUNKED_MANUAL], [test -n "$A2X"])
])
