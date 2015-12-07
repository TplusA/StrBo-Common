AC_DEFUN([AC_CHECK_CUTTER],
[
  AC_ARG_WITH([cutter],
              AS_HELP_STRING([--with-cutter],
                             [Use Cutter (default: auto)]),
              [cutter_with_value=$withval],
              [cutter_with_value=auto])
  if test -z "$cutter_use_cutter"; then
    if test "x$cutter_with_value" = "xno"; then
      cutter_use_cutter=no
    else
      m4_ifdef([PKG_CHECK_MODULES], [
	PKG_CHECK_MODULES(CUTTER, cutter $1,
			  [cutter_use_cutter=yes],
			  [cutter_use_cutter=no])
        ],
        [cutter_use_cutter=no])
    fi
  fi
  if test "$cutter_use_cutter" != "no"; then
    _PKG_CONFIG(CUTTER, variable=cutter, cutter)
    CUTTER=$pkg_cv_CUTTER
  fi
  ac_cv_use_cutter="$cutter_use_cutter" # for backward compatibility
  AC_SUBST([CUTTER_CFLAGS])
  AC_SUBST([CUTTER_LIBS])
  AC_SUBST([CUTTER])
])

AC_DEFUN([AC_CHECK_GCUTTER],
[
  AC_CHECK_CUTTER($1)
  if test "$cutter_use_cutter" = "no"; then
    cutter_use_gcutter=no
  fi
  if test "x$cutter_use_gcutter" = "x"; then
    m4_ifdef([PKG_CHECK_MODULES], [
      PKG_CHECK_MODULES(GCUTTER, gcutter $1,
			[cutter_use_gcutter=yes],
			[cutter_use_gcutter=no])
      ],
      [cutter_use_gcutter=no])
  fi
  ac_cv_use_gcutter="$cutter_use_gcutter" # for backward compatibility
  AC_SUBST([GCUTTER_CFLAGS])
  AC_SUBST([GCUTTER_LIBS])
])

AC_DEFUN([AC_CHECK_CPPCUTTER],
[
  AC_CHECK_CUTTER($1)
  if test "$cutter_use_cutter" = "no"; then
    cutter_use_cppcutter=no
  fi
  if test "x$cutter_use_cppcutter" = "x"; then
    m4_ifdef([PKG_CHECK_MODULES], [
      PKG_CHECK_MODULES(CPPCUTTER, cppcutter $1,
			[cutter_use_cppcutter=yes],
			[cutter_use_cppcutter=no])
      ],
      [cutter_use_cppcutter=no])
  fi
  ac_cv_use_cppcutter="$cutter_use_cppcutter" # for backward compatibility
  AC_SUBST([CPPCUTTER_CFLAGS])
  AC_SUBST([CPPCUTTER_LIBS])
])

AC_DEFUN([AC_CHECK_GDKCUTTER_PIXBUF],
[
  AC_CHECK_GCUTTER($1)
  if test "$cutter_use_cutter" = "no"; then
    cutter_use_gdkcutter_pixbuf=no
  fi
  if test "x$cutter_use_gdkcutter_pixbuf" = "x"; then
    m4_ifdef([PKG_CHECK_MODULES], [
      PKG_CHECK_MODULES(GDKCUTTER_PIXBUF, gdkcutter-pixbuf $1,
			[cutter_use_gdkcutter_pixbuf=yes],
			[cutter_use_gdkcutter_pixbuf=no])
      ],
      [cutter_use_gdkcutter_pixbuf=no])
  fi
  ac_cv_use_gdkcutter_pixbuf="$cutter_use_gdkcutter_pixbuf" # for backward compatibility
  AC_SUBST([GDKCUTTER_PIXBUF_CFLAGS])
  AC_SUBST([GDKCUTTER_PIXBUF_LIBS])
])

AC_DEFUN([AC_CHECK_SOUPCUTTER],
[
  AC_CHECK_GCUTTER($1)
  if test "$cutter_use_cutter" = "no"; then
    cutter_use_soupcutter=no
  fi
  if test "$cutter_use_soupcutter" != "no"; then
    m4_ifdef([PKG_CHECK_MODULES], [
      PKG_CHECK_MODULES(SOUPCUTTER, soupcutter $1,
			[cutter_use_soupcutter=yes],
			[cutter_use_soupcutter=no])
      ],
      [cutter_use_soupcutter=no])
  fi
  ac_cv_use_soupcutter="$cutter_use_soupcutter" # for backward compatibility
  AC_SUBST([SOUPCUTTER_CFLAGS])
  AC_SUBST([SOUPCUTTER_LIBS])
])
