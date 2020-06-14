/*-------------------------------------------------------------------------
 * Filename:      types.h
 * Version:       $Id: types.h,v 1.1 2001/08/14 21:17:39 jamey Exp $
 * Copyright:     Copyright (C) 1999, Erik Mouw
 * Author:        Erik Mouw <J.A.K.Mouw@its.tudelft.nl>
 * Description:   Define some handy types for the blob
 * Created at:    Tue Aug 24 19:04:22 1999
 * Modified by:   Erik Mouw <J.A.K.Mouw@its.tudelft.nl>
 * Modified at:   Tue Sep 28 23:45:06 1999
 *-----------------------------------------------------------------------*/
/*
 * types.h: Some handy types and macros for blob
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

#ident "$Id: types.h,v 1.1 2001/08/14 21:17:39 jamey Exp $"

#ifndef BLOB_TYPES_H
#define BLOB_TYPES_H




typedef unsigned long	u32;
typedef unsigned short	u16;
typedef	unsigned char	u8;
typedef unsigned long	__u32;
typedef unsigned short	__u16;
typedef	unsigned char	__u8;
typedef __SIZE_TYPE__	size_t;




#ifndef NULL
#define NULL (void *)0
#endif




#endif
