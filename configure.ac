AC_INIT([ai-cage], [0.1], [])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_PROG_CC

AC_ARG_WITH([config-dir],
  [AS_HELP_STRING([--with-config-dir=PATH], [config file directory @<:@/etc/ai@:>@])],
  [config_dir="$withval"],
  [config_dir="/etc/ai"])
AC_SUBST([CONFIG_DIR], [$config_dir])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
