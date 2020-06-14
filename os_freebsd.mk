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
# change this to point to the Linux include directory
#
LINUX_DIR=$(HOME)/linux/kernel/include

#
# Location of the gcc arm libs.
#
ARM_GCC_LIBS=/skiff/local/lib/gcc-lib/arm-linux/2.95.2

#
# point these at your zlib.h, zconf.h and libz.a files.
#
LIBZ_INCLUDE_DIR=/skiff/local/arm-linux/include
LIBZ_LIB_DIR=/skiff/local/arm-linux/lib


OS_INCLUDES = -I$(LINUX_DIR) -I$(LIBZ_INCLUDE_DIR) 
OS_DEFS = -UBOOT_SILENT -UNO_SPLASH -DSPLASH_LINUX
OS_CFLAGS = -O -fPIC
OS_ASMFLAGS = -x assembler-with-cpp -c
OS_CLIBS = -L$(ARM_GCC_LIBS) -lgcc -L$(LIBZ_LIB_DIR)


OBJCOPYFLAGS = -R .comment -R .stab -R .stabstr
BOOTLDR_SIZE = `ls -l ./bootldr | $AWK '{print \$5}'`

CROSS_COMPILE = arm-linux-

MD5SUM=md5
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
CONFIG_GIMP_SPLASH_PNM=../pics/splash.pnm