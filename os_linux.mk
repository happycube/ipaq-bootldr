# 
#  Copyright 2000 Compaq Computer Corporation.                       
#                                                                    
#  Copying or modifying this code for any purpose is permitted,        
#  provided that this copyright notice is preserved in its entirety     
#  in all copies or modifications.  COMPAQ COMPUTER CORPORATION          
#  MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS        
#  OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR           
#  PURPOSE.                                                                 
# 

#
# point these at your zlib.h, zconf.h and libz.a files.
#
#LIBZ_INCLUDE_DIR=/skiff/local/arm-linux/include
#LIBZ_LIB_DIR=/skiff/local/arm-linux/lib
LIBZ_INCLUDE_DIR=/opt/hhcross/2.95.3/include
LIBZ_LIB_DIR=/opt/hhcross/2.95.3/lib

# this *should* pick out the zlibs for the toolchain you are currently using :) if not, use the above
#LIBZ_INCLUDE_DIR=`which arm-linux-gcc| sed -e 's/bin\/arm-linux-gcc/arm-linux\/include/'`
#LIBZ_LIB_DIR=`/usr/bin/which arm-linux-gcc| sed -e "s/bin\/arm-linux-gcc/arm-linux\/lib/"`


OS_INCLUDES = -I.  -I$(LIBZ_INCLUDE_DIR) 
OS_DEFS = -UBOOT_SILENT -UNO_SPLASH -DSPLASH_LINUX
# used for osloader friendly app
OS_WINCE_DEFS = -UBOOT_SILENT -DNO_SPLASH -USPLASH_LINUX
OS_CFLAGS = -O -fPIC -march=armv4
OS_ASMFLAGS = -x assembler-with-cpp -c
OS_CLIBS = `$(CROSS_COMPILE)$(CC) --print-libgcc-file-name`  -L$(LIBZ_LIB_DIR)


OBJCOPYFLAGS = -R .comment -R .stab -R .stabstr
BOOTLDR_SIZE = `ls -l ./bootldr | $AWK '{print \$5}'`

CROSS_COMPILE = arm-linux-

MD5SUM=md5sum
AWK = awk
ASM = gcc
CC = gcc
HOSTCC = gcc
LD = ld
STRIP = strip
NM = nm
OBJCOPY = objcopy

#
# gimp produced .pnm file (SaveAs xxx.pnm)
#
CONFIG_GIMP_SPLASH_PNM=pics/splash.pnm
