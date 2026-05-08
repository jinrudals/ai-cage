AC_INIT([ai-cage], [0.2], [])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])

AC_PROG_CC

AC_ARG_WITH([config-dir],
  [AS_HELP_STRING([--with-config-dir=PATH], [config file directory @<:@/etc/ai@:>@])],
  [config_dir="$withval"],
  [config_dir="/etc/ai"])
AC_SUBST([CONFIG_DIR], [$config_dir])

AC_ARG_WITH([backend],
  [AS_HELP_STRING([--with-backend=BACKEND], [sandbox backend: landlock or apptainer @<:@landlock@:>@])],
  [backend="$withval"],
  [backend="landlock"])
AC_SUBST([BACKEND], [$backend])

AC_ARG_WITH([apptainer-bin],
  [AS_HELP_STRING([--with-apptainer-bin=PATH], [path to apptainer binary @<:@/usr/bin/apptainer@:>@])],
  [apptainer_bin="$withval"],
  [apptainer_bin="/usr/bin/apptainer"])
AC_SUBST([APPTAINER_BIN], [$apptainer_bin])

AC_ARG_WITH([apptainer-images],
  [AS_HELP_STRING([--with-apptainer-images=PATH], [path to .sif images directory])],
  [apptainer_images="$withval"],
  [apptainer_images="/usr/local/share/ai-cage/images"])
AC_SUBST([APPTAINER_IMAGES], [$apptainer_images])

AM_CONDITIONAL([BACKEND_LANDLOCK], [test "x$backend" = "xlandlock"])
AM_CONDITIONAL([BACKEND_APPTAINER], [test "x$backend" = "xapptainer"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
