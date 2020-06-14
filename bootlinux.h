/****************************************************************************/
/* Copyright 2002 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * support for booting linux
 *
 */

/*
 * Maintainer: Jamey Hicks (jamey@crl.dec.com)
 */

extern void setup_ramdisk_from_flash(void);
void setup_linux_params(long bootimg_dest, long memc_ctrl_reg, const char *cmdline);

