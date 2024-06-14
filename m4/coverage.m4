AC_DEFUN([AC_CHECK_ENABLE_COVERAGE],
[
  AC_MSG_CHECKING([for enabling coverage])
  AC_ARG_ENABLE([coverage],
                AS_HELP_STRING([--enable-coverage],
                               [Enable coverage]),
                [cutter_enable_coverage=$enableval],
                [cutter_enable_coverage=no])
  AC_MSG_RESULT($cutter_enable_coverage)
  AC_ARG_VAR([LCOV_EXTRA_PARAMS], [Extra command line parameters for lcov])
  LCOV_EXTRA_PARAMS=""
  cutter_enable_coverage_report_lcov=no
  if test "x$cutter_enable_coverage" != "xno"; then
    ltp_version_list="1.6 1.7 1.8 1.9 1.10 1.11 1.12 1.13 1.14 1.15 1.16"
    ltp_version_list_v2="2.0-1 2.1-1"
    AC_PATH_TOOL(LCOV, lcov)
    AC_PATH_TOOL(GENHTML, genhtml)

    if test -x "$LCOV"; then
      AC_CACHE_CHECK([for ltp version],
                     cutter_cv_ltp_version,
                     [
        ltp_version=`$LCOV --version 2>/dev/null | $SED -e 's/^.* //'`
        cutter_cv_ltp_version="$ltp_version (NG)"
        for ltp_check_version in $ltp_version_list; do
          if test "$ltp_version" = "$ltp_check_version"; then
            cutter_cv_ltp_version="$ltp_check_version (ok)"
          fi
        done
        for ltp_check_version in $ltp_version_list_v2; do
          if test "$ltp_version" = "$ltp_check_version"; then
            cutter_cv_ltp_version="$ltp_check_version (ok)"
            LCOV_EXTRA_PARAMS="$LCOV_EXTRA_PARAMS --ignore-errors mismatch,inconsistent"
          fi
        done
      ])
    fi

    AC_MSG_CHECKING([for enabling coverage report by LCOV])
    case "$cutter_cv_ltp_version" in
      *\(ok\)*)
        cutter_enable_coverage_report_lcov=yes
        ;;
      *)
        cutter_enable_coverage_report_lcov=no
        ;;
    esac
    AC_MSG_RESULT($cutter_enable_coverage_report_lcov)
  fi
])

AC_DEFUN([AC_CHECK_COVERAGE],
[
  ac_check_coverage_makefile=$1
  if test -z "$ac_check_coverage_makefile"; then
    ac_check_coverage_makefile=Makefile
  fi
  AC_SUBST(ac_check_coverage_makefile)

  AC_CHECK_ENABLE_COVERAGE

  COVERAGE_CFLAGS=
  COVERAGE_LIBS=
  if test "$cutter_enable_coverage" = "yes"; then
    COVERAGE_CFLAGS="--coverage"
    COVERAGE_LIBS="-lgcov"
  fi
  AC_SUBST(COVERAGE_CFLAGS)
  AC_SUBST(COVERAGE_LIBS)
  AM_CONDITIONAL([ENABLE_COVERAGE], [test "$cutter_enable_coverage" = "yes"])
  AM_CONDITIONAL([ENABLE_COVERAGE_REPORT_LCOV],
                 [test "$cutter_enable_coverage_report_lcov" = "yes"])

  COVERAGE_INFO_FILE="coverage.info"
  AC_SUBST(COVERAGE_INFO_FILE)

  COVERAGE_REPORT_DIR="coverage"
  AC_SUBST(COVERAGE_REPORT_DIR)

  if test "$GENHTML_OPTIONS" = ""; then
    GENHTML_OPTIONS=""
  fi
  AC_SUBST(GENHTML_OPTIONS)

  if test "$cutter_enable_coverage_report_lcov" = "yes"; then
    AC_CONFIG_COMMANDS([coverage-report-lcov], [
      if test -e "$ac_check_coverage_makefile" && \
         grep -q '^coverage:' $ac_check_coverage_makefile; then
        : # do nothing
      else
        sed -e 's/^        /	/g' <<EOS >>$ac_check_coverage_makefile
.PHONY: coverage-clean coverage-report coverage coverage-force

coverage-clean:
	\$(LCOV) --compat-libtool --zerocounters --directory . \\
	  \$(LCOV_EXTRA_PARAMS) \\
	  --output-file \$(COVERAGE_INFO_FILE)

coverage-report:
	\$(LCOV) --compat-libtool --directory . \\
	  \$(LCOV_EXTRA_PARAMS) \\
	  --capture --output-file \$(COVERAGE_INFO_FILE)
	\$(LCOV) --compat-libtool --directory . \\
	  \$(LCOV_EXTRA_PARAMS) \\
	  --extract \$(COVERAGE_INFO_FILE) "\`(cd '\$(top_srcdir)'; pwd)\`/*" \\
	  --output-file \$(COVERAGE_INFO_FILE)
	\$(GENHTML) --highlight --legend \\
	  --output-directory \$(COVERAGE_REPORT_DIR) \\
	  --prefix "\`(cd '\$(top_srcdir)'; pwd)\`" \\
	  \$(GENHTML_OPTIONS) \$(COVERAGE_INFO_FILE)

coverage: coverage-clean check coverage-report

coverage-force:
	\$(MAKE) \$(AM_MAKEFLAGS) coverage-clean
	\$(MAKE) \$(AM_MAKEFLAGS) check || :
	\$(MAKE) \$(AM_MAKEFLAGS) coverage-report
EOS
      fi
    ],
    [ac_check_coverage_makefile="$ac_check_coverage_makefile"])
  fi
])
