# Configure paths for BTK+
# Owen Taylor     1997-2001

# Version number used by aclocal, see `info automake Serials`.
# Increment on every change.
#serial 1

dnl AM_PATH_BTK_2_0([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for BTK+, and define BTK_CFLAGS and BTK_LIBS, if bthread is specified in MODULES, 
dnl pass to pkg-config
dnl
AC_DEFUN([AM_PATH_BTK_2_0],
[dnl 
dnl Get the cflags and libraries from pkg-config
dnl
AC_ARG_ENABLE(btktest, [  --disable-btktest       do not try to compile and run a test BTK+ program],
		    , enable_btktest=yes)

  pkg_config_args=btk+-2.0
  for module in . $4
  do
      case "$module" in
         bthread) 
             pkg_config_args="$pkg_config_args bthread-2.0"
         ;;
      esac
  done

  no_btk=""

  AC_REQUIRE([PKG_PROG_PKG_CONFIG])
  PKG_PROG_PKG_CONFIG([0.7])

  min_btk_version=ifelse([$1], ,2.0.0,$1)
  AC_MSG_CHECKING(for BTK+ - version >= $min_btk_version)

  if test x$PKG_CONFIG != xno ; then
    ## don't try to run the test against uninstalled libtool libs
    if $PKG_CONFIG --uninstalled $pkg_config_args; then
	  echo "Will use uninstalled version of BTK+ found in PKG_CONFIG_PATH"
	  enable_btktest=no
    fi

    if $PKG_CONFIG --atleast-version $min_btk_version $pkg_config_args; then
	  :
    else
	  no_btk=yes
    fi
  fi

  if test x"$no_btk" = x ; then
    BTK_CFLAGS=`$PKG_CONFIG $pkg_config_args --cflags`
    BTK_LIBS=`$PKG_CONFIG $pkg_config_args --libs`
    btk_config_major_version=`$PKG_CONFIG --modversion btk+-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    btk_config_minor_version=`$PKG_CONFIG --modversion btk+-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    btk_config_micro_version=`$PKG_CONFIG --modversion btk+-2.0 | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_btktest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $BTK_CFLAGS"
      LIBS="$BTK_LIBS $LIBS"
dnl
dnl Now check if the installed BTK+ is sufficiently new. (Also sanity
dnl checks the results of pkg-config to some extent)
dnl
      rm -f conf.btktest
      AC_TRY_RUN([
#include <btk/btk.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  fclose (fopen ("conf.btktest", "w"));

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_btk_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_btk_version");
     exit(1);
   }

  if ((btk_major_version != $btk_config_major_version) ||
      (btk_minor_version != $btk_config_minor_version) ||
      (btk_micro_version != $btk_config_micro_version))
    {
      printf("\n*** 'pkg-config --modversion btk+-2.0' returned %d.%d.%d, but BTK+ (%d.%d.%d)\n", 
             $btk_config_major_version, $btk_config_minor_version, $btk_config_micro_version,
             btk_major_version, btk_minor_version, btk_micro_version);
      printf ("*** was found! If pkg-config was correct, then it is best\n");
      printf ("*** to remove the old version of BTK+. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If pkg-config was wrong, set the environment variable PKG_CONFIG_PATH\n");
      printf("*** to point to the correct configuration files\n");
    } 
  else if ((btk_major_version != BTK_MAJOR_VERSION) ||
	   (btk_minor_version != BTK_MINOR_VERSION) ||
           (btk_micro_version != BTK_MICRO_VERSION))
    {
      printf("*** BTK+ header files (version %d.%d.%d) do not match\n",
	     BTK_MAJOR_VERSION, BTK_MINOR_VERSION, BTK_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     btk_major_version, btk_minor_version, btk_micro_version);
    }
  else
    {
      if ((btk_major_version > major) ||
        ((btk_major_version == major) && (btk_minor_version > minor)) ||
        ((btk_major_version == major) && (btk_minor_version == minor) && (btk_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of BTK+ (%d.%d.%d) was found.\n",
               btk_major_version, btk_minor_version, btk_micro_version);
        printf("*** You need a version of BTK+ newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** BTK+ is always available from ftp://ftp.btk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the pkg-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of BTK+, but you can also set the PKG_CONFIG environment to point to the\n");
        printf("*** correct copy of pkg-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_btk=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_btk" = x ; then
     AC_MSG_RESULT(yes (version $btk_config_major_version.$btk_config_minor_version.$btk_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$PKG_CONFIG" = "no" ; then
       echo "*** A new enough version of pkg-config was not found."
       echo "*** See http://pkgconfig.sourceforge.net"
     else
       if test -f conf.btktest ; then
        :
       else
          echo "*** Could not run BTK+ test program, checking why..."
	  ac_save_CFLAGS="$CFLAGS"
	  ac_save_LIBS="$LIBS"
          CFLAGS="$CFLAGS $BTK_CFLAGS"
          LIBS="$LIBS $BTK_LIBS"
          AC_TRY_LINK([
#include <btk/btk.h>
#include <stdio.h>
],      [ return ((btk_major_version) || (btk_minor_version) || (btk_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding BTK+ or finding the wrong"
          echo "*** version of BTK+. If it is not finding BTK+, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means BTK+ is incorrectly installed."])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     BTK_CFLAGS=""
     BTK_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(BTK_CFLAGS)
  AC_SUBST(BTK_LIBS)
  rm -f conf.btktest
])
