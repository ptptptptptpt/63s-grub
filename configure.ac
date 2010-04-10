dnl Configure script for GRUB.
dnl Copyright 1999,2000,2001,2002,2003,2004,2005 Free Software Foundation, Inc.

dnl Permission to use, copy, modify and distribute this software and its
dnl documentation is hereby granted, provided that both the copyright
dnl notice and this permission notice appear in all copies of the
dnl software, derivative works or modified versions, and any portions
dnl thereof, and that both notices appear in supporting documentation.
dnl
dnl THE FREE SOFTWARE FOUNDATION ALLOWS FREE USE OF THIS SOFTWARE IN ITS
dnl "AS IS" CONDITION.  THE FREE SOFTWARE FOUNDATION DISCLAIMS ANY
dnl LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE
dnl USE OF THIS SOFTWARE.

AC_PREREQ(2.57)
AC_INIT([63s-GRUB], [V1.2], [earthengine@gmail.com])
AC_CONFIG_SRCDIR([stage2/stage2.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE


AC_CANONICAL_HOST

case "$host_cpu" in
i[[3456]]86) host_cpu=i386 ;;
x86_64) host_cpu=x86_64 ;;
*) AC_MSG_ERROR([unsupported CPU type]) ;;
esac

AC_SUBST(host_cpu)
AC_SUBST(host_vendor)

#
# Options
#

AM_MAINTAINER_MODE

AC_PATH_PROG(PERL, perl)
if test -z "$PERL"; then
  AC_MSG_ERROR([perl not found])
fi

AC_ARG_WITH([qemu],
	[AC_HELP_STRING([--with-qemu], [specify a qemu emulator with full path])])

if test x"$with_qemu" != xno; then
  QEMU=$with_qemu
else
  QEMU="no"
fi

if test -z "$QEMU"; then
  AC_PATH_PROG(QEMU, kvm)
fi
if test -z "$QEMU"; then
  AC_PATH_PROG(QEMU, qemu-kvm)
fi
if test -z "$QEMU"; then
  AC_PATH_PROG(QEMU, qemu)
fi
if test -z "$QEMU"; then
  QEMU="no"
fi
if test x$QEMU == xno; then
  AC_MSG_WARN([qemu test disabled, you will not be able to run "make test".])
fi

# This should be checked before AC_PROG_CC
if test "x$CFLAGS" = x; then
  default_CFLAGS=yes
fi

#
# Programs
#

AC_CHECK_TOOL(CC, gcc)
AC_PROG_CC
AM_PROG_AS
AM_PROG_CC_C_O
# We need this for older versions of Autoconf.
_AM_DEPENDENCIES(CC)

dnl Because recent automake complains about AS, set it here.
CCAS="$CC"
AC_SUBST(CCAS)

AC_ARG_WITH(binutils,
  [  --with-binutils=DIR     search the directory DIR to find binutils])

if test "x$with_binutils" != x; then
dnl AC_PATH_TOOL is not seen in autoconf 2.13, so use AC_PATH_PROG
dnl instead for now. It is preferable when you cross-compile GRUB.
dnl  AC_PATH_TOOL(RANLIB, ranlib, :, "$with_binutils:$PATH")
  AC_PATH_PROG(RANLIB, ranlib, :, "$with_binutils:$PATH")
else
  AC_PROG_RANLIB
fi

# optimization flags
if test "x$ac_cv_prog_gcc" = xyes; then
  if test "x$default_CFLAGS" = xyes; then
    # Autoconf may set CFLAGS to -O2 and/or -g. So eliminate them.
    CFLAGS="`echo $CFLAGS | sed -e 's/-g//g' -e 's/-O[[0-9]]//g'` -g"
    # If the user specify the directory for binutils, add the option `-B'.
    if test "x$with_binutils" != x; then
      CFLAGS="-B$with_binutils/ $CFLAGS"
    fi
    STAGE1_CFLAGS="-O2 -fno-strict-aliasing"
    GRUB_CFLAGS="-O2 -fno-strict-aliasing"
    AC_CACHE_CHECK([whether optimization for size works], ac_cv_size_flag, [
      ac_cv_saved_CFLAGS=$CFLAGS
      CFLAGS="-Os -g"
      AC_TRY_COMPILE(, , ac_cv_size_flag=yes, ac_cv_size_flag=no)
      CFLAGS=$saved_CFLAGS
    ])
    if test "x$ac_cv_size_flag" = xyes; then
      STAGE2_CFLAGS="-Os -fno-strict-aliasing"
    else
      STAGE2_CFLAGS="-O2 -fno-strength-reduce -fno-unroll-loops -fno-strict-aliasing"
    fi
    # OpenBSD has a GCC extension for protecting applications from
    # stack smashing attacks, but GRUB doesn't want this feature.
    AC_CACHE_CHECK([whether gcc has -fno-stack-protector],
		   ac_cv_no_stack_protector_flag, [
      saved_CFLAGS=$CFLAGS
      CFLAGS="-fno-stack-protector"
      AC_TRY_COMPILE(,
		     ,
		     ac_cv_no_stack_protector_flag=yes,
		     ac_cv_no_stack_protector_flag=no)
      CFLAGS=$saved_CFLAGS
    ])
    if test "x$ac_cv_no_stack_protector_flag" = xyes; then
      STAGE2_CFLAGS="$STAGE2_CFLAGS -fno-stack-protector"
    fi
  fi
fi

AC_SUBST(STAGE1_CFLAGS)
AC_SUBST(STAGE2_CFLAGS)
AC_SUBST(GRUB_CFLAGS)

# Enforce coding standards.
CPPFLAGS="$CPPFLAGS -Wall -Wmissing-prototypes -Wunused -Wshadow"
CPPFLAGS="$CPPFLAGS -Wpointer-arith"

AC_CACHE_CHECK([whether -Wundef works], ac_cv_undef_flag, [
  saved_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="-Wundef"
  AC_TRY_COMPILE(, , ac_cv_undef_flag=yes, ac_cv_undef_flag=no)
  CPPFLAGS="$saved_CPPFLAGS"
])

# The options `-falign-*' are supported by gcc 3.0 or later.
# Probably it is sufficient to only check for -falign-loops.
AC_CACHE_CHECK([whether -falign-loops works], [ac_cv_falign_loop_flag], [
  saved_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="-falign-loops=1"
  AC_TRY_COMPILE(, , [ac_cv_falign_loop_flag=yes], [ac_cv_falign_loop_flag=no])
  CPPFLAGS="$saved_CPPFLAGS"
])

# Force no alignment to save space.
if test "x$ac_cv_falign_loop_flag" = xyes; then
  CPPFLAGS="$CPPFLAGS -falign-jumps=1 -falign-loops=1 -falign-functions=1"
else
  CPPFLAGS="$CPPFLAGS -malign-jumps=1 -malign-loops=1 -malign-functions=1"
fi

if test "x$ac_cv_undef_flag" = xyes; then
  CPPFLAGS="$CPPFLAGS -Wundef"
fi

if test "x$with_binutils" != x; then
dnl  AC_PATH_TOOL(OBJCOPY, objcopy, , "$with_binutils:$PATH")
  AC_PATH_PROG(OBJCOPY, objcopy, , "$with_binutils:$PATH")
else
  AC_CHECK_TOOL(OBJCOPY, objcopy)
fi

# Defined in acinclude.m4.
grub_ASM_USCORE
grub_PROG_OBJCOPY_ABSOLUTE
if test "x$grub_cv_prog_objcopy_absolute" != xyes; then
  AC_MSG_ERROR([GRUB requires a working absolute objcopy; upgrade your binutils])
fi

grub_ASM_PREFIX_REQUIREMENT

grub_ASM_ADDR32
if test "x$grub_cv_asm_addr32" != xyes; then
  AC_MSG_ERROR([GRUB requires GAS .code16 addr32 support; upgrade your binutils])
fi

grub_ASM_ABSOLUTE_WITHOUT_ASTERISK

grub_CHECK_START_SYMBOL
grub_CHECK_USCORE_START_SYMBOL
if test "x$grub_cv_check_start_symbol" != "xyes" \
	-a "x$grub_cv_check_uscore_start_symbol" != "xyes"; then
  AC_MSG_ERROR([Neither start nor _start is defined])
fi

grub_CHECK_USCORE_USCORE_BSS_START_SYMBOL
grub_CHECK_USCORE_EDATA_SYMBOL
grub_CHECK_EDATA_SYMBOL
if test "x$grub_cv_check_uscore_uscore_bss_start_symbol" != "xyes" \
	-a "x$grub_cv_check_uscore_edata_symbol" != "xyes" \
	-a "x$grub_cv_check_edata_symbol" != "xyes"; then
  AC_MSG_ERROR([None of __bss_start, _edata, edata defined])
fi

grub_CHECK_END_SYMBOL
grub_CHECK_USCORE_END_SYMBOL
if test "x$grub_cv_check_end_symbol" != "xyes" \
	-a "x$grub_cv_check_uscore_end_symbol" != "xyes"; then
  AC_MSG_ERROR([Neither end nor _end is defined])
fi

# Check for curses libraries.
AC_ARG_WITH(curses,
  [  --without-curses        do not use curses])

# Get the filename or the whole disk and open it.
# Known to work on NetBSD.
AC_CHECK_LIB(util, opendisk, [GRUB_LIBS="$GRUB_LIBS -lutil"
  AC_DEFINE(HAVE_OPENDISK, 1, [Define if opendisk() in -lutil can be used])])

# Unless the user specify --without-curses, check for curses.
if test "x$with_curses" != "xno"; then
  AC_CHECK_LIB(ncurses, wgetch, [GRUB_LIBS="$GRUB_LIBS -lncurses"
  AC_DEFINE(HAVE_LIBCURSES, 1, [Define if you have a curses library])],
    [AC_CHECK_LIB(curses, wgetch, [GRUB_LIBS="$GRUB_LIBS -lcurses"
       AC_DEFINE(HAVE_LIBCURSES, 1, [Define if you have a curses library])])])
fi

AC_SUBST(GRUB_LIBS)

# Check for headers.
AC_CHECK_HEADERS(string.h strings.h ncurses/curses.h ncurses.h curses.h)

# Check for user options.

# filesystems support.

AC_ARG_ENABLE(reiserfs,
  [  --enable-reiserfs     enable ReiseFS support in Stage 2])

if test x"$enable_reiserfs" = xyes; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_REISERFS=1"
fi

AC_ARG_ENABLE(ext2fs,
  [  --enable-ext2fs        enable ext2fs support in Stage 2])

if test x"$enable_ext2fs" = xyes; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_EXT2FS=1"
fi

AC_ARG_ENABLE(fat,
  [  --disable-fat           disable FAT support in Stage 2])

if test x"$enable_fat" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_FAT=1"
fi

AC_ARG_ENABLE(ntfs,
  [  --disable-ntfs          disable NTFS support in Stage 2])

if test x"$enable_ntfs" != xno; then
  FSYS_CFLAGS="$FSYS_CFLAGS -DFSYS_NTFS=1"
fi

FSYS_CFLAGS="$FSYS_CFLAGS -DNO_DECOMPRESSION=1"
GRAPHICS_SUPPORT=0
HERCULES_SUPPORT=0
SERIAL_SUPPORT=0
UUID_SUPPORT=0

CFLAGS="-Wall -Werror $CFLAGS"
if test "x$host_cpu" = xx86_64; then
  CFLAGS="-m32 $CFLAGS"
fi

dnl Now substitute the variables.
AC_SUBST(FSYS_CFLAGS)
AC_SUBST(NET_CFLAGS)
AC_SUBST(NET_EXTRAFLAGS)
AC_SUBST(NETBOOT_DRIVERS)

dnl Because recent automake complains about CCASFLAGS, set it here.
CCASFLAGS='$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)'
AC_SUBST(CCASFLAGS)

dnl Output.
AC_CONFIG_FILES([Makefile stage1/Makefile stage2/Makefile test/Makefile])
AC_OUTPUT
