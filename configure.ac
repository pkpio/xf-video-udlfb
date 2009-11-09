#
#
#        Copyright 2009 Roberto De Ioris <roberto@unbit.it>
#
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, version 2.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * A copy of the General Public License is included with the source
# * distribution of this driver, as COPYING.


AC_PREREQ(2.57)
AC_INIT([xf86-video-displaylink],
        0.0.1,
        [],
        xf86-video-displaylink)

AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE([dist-bzip2])

AM_MAINTAINER_MODE

# Checks for programs.
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC

AH_TOP([#include "xorg-server.h"])

AC_ARG_WITH(xorg-module-dir,
            AC_HELP_STRING([--with-xorg-module-dir=DIR],
                           [Default xorg module directory [[default=$libdir/xorg/modules]]]),
            [moduledir="$withval"],
            [moduledir="$libdir/xorg/modules"])

# Checks for extensions
XORG_DRIVER_CHECK_EXT(RANDR, randrproto)
XORG_DRIVER_CHECK_EXT(RENDER, renderproto)
XORG_DRIVER_CHECK_EXT(XV, videoproto)

# Checks for pkg-config packages
PKG_CHECK_MODULES(XORG, [xorg-server >= 1.0.99.901 xproto fontsproto $REQUIRED_MODULES])
sdkdir=$(pkg-config --variable=sdkdir xorg-server)

save_CFLAGS="$CFLAGS"
CFLAGS="$XORG_CFLAGS"
AC_CHECK_DECL(xf86ConfigIsaEntity,
	      [AC_DEFINE(HAVE_ISA, 1, [Have ISA support])],
	      [],
	      [#include "xf86.h"])
CFLAGS="$save_CFLAGS"

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC

AC_SUBST([XORG_CFLAGS])
AC_SUBST([moduledir])

DRIVER_NAME=displaylink
AC_SUBST([DRIVER_NAME])

XORG_MANPAGE_SECTIONS
XORG_RELEASE_VERSION

AC_OUTPUT([
	Makefile
	src/Makefile
])
