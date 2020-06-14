/* include/config.h.  Generated automatically by configure.  */
/* include/config.h.in.  Generated automatically from configure.in by autoheader 2.13.  */
/*-------------------------------------------------------------------------
 * Filename:      acconfig.h
 * Version:       $Id: config.h,v 1.1 2001/08/14 21:17:39 jamey Exp $
 * Copyright:     Copyright (C) 1999, Erik Mouw
 * Author:        Erik Mouw <J.A.K.Mouw@its.tudelft.nl>
 * Description:   Definitions for GNU autoconf/automake/libtool
 * Created at:    Wed Jul 28 22:35:51 1999
 * Modified by:   Erik Mouw <J.A.K.Mouw@its.tudelft.nl>
 * Modified at:   Sun Oct  3 18:47:48 1999
 *-----------------------------------------------------------------------*/
/*
 * acconfig.h: Definitions for GNU autoconf/automake/libtool
 *
 * Copyright (C) 1999  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* 
 *
 * WARNING:
 *   Don't try to edit config.h.in or config.h, because your changes
 *   will be overwritten once you run automake, autoconf or
 *   configure. Edit acconfig.h instead.
 *
 */

#ident "$Id: config.h,v 1.1 2001/08/14 21:17:39 jamey Exp $"

#ifndef BLOB_CONFIG_H
#define BLOB_CONFIG_H


/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define to enable run-time debug information */
/* #undef BLOB_DEBUG */

/* Define for Assabet boards */
/* #undef ASSABET */

/* Define for Brutus boards */
/* #undef BRUTUS */

/* Define for LART boards */
/* #undef LART */

/* Define for NESA boards */
#define NESA 1

/* Define for PLEB boards */
/* #undef PLEB */

/* Define if your system uses an SA-1100 CPU */
#define USE_SA1100 1

/* Define if your system uses an SA-1110 CPU */
/* #undef USE_SA1110 */

/* Define if your system uses SDRAM */
/* #undef USE_SDRAM */

/* Define if your system uses EDO DRAM */
#define USE_EDODRAM 1

/* Define if your sytem uses serial port 1 */
/* #undef USE_SERIAL1 */

/* Define if your system uses serial port 3 */
#define USE_SERIAL3 1

/* Define if you want the jffs2 kernel loader */
#define JFFS2_LOAD 1

/* Define if you want the cramfs kernel loader */
#define CRAMFS_LOAD 1

/* Define if you want the zImage kernel loader */
#define ZIMAGE_LOAD 1

/* Name of package */
#define PACKAGE "blob"

/* Version number of package */
#define VERSION "1.0.8-pre2"


#endif
