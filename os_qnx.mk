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
# These are standard QNX locations - can still be overriden in config.local.mk
#
ARM_GCC_LIBS=/usr/lib/gcc-lib/ntoarm/2.95.3/pic/le
ARM_LIBS=/armle/lib
ARM_USR_LIBS=/armle/usr/lib

OS_INCLUDES = 
OS_DEFS = -UBOOT_SILENT -UNO_SPLASH -DSPLASH_QNX=1 -DNO_HILITE_BUTTON=1
OS_CFLAGS = -Vgcc_ntoarmle -O -fPIC -I.
OS_ASMFLAGS = -x assembler-with-cpp -c -I.
OS_CLIBS = -L$(ARM_GCC_LIBS) -L$(ARM_LIBS) -L$(ARM_USR_LIBS) -lgcc
OS_LDFLAGS = -nostdlib 

OBJCOPYFLAGS = -R .comment -R .stab -R .stabstr
BOOTLDR_SIZE = `ls -l ./bootldr | $AWK '{print \$5}'`

CROSS_COMPILE = 
LDOPT_PREFIX=-Wl,
GCCOPT_PREFIX=-Wc,

MD5SUM=cksum
AWK = awk
ASM = qcc -Vgcc_ntoarmle
CC = qcc 
HOSTCC = qcc
LD = qcc -Vgcc_ntoarmle
STRIP = ntoarm-strip
NM = ntoarm-nm
OBJCOPY = ntoarm-objcopy

CONFIG_LOAD_KERNEL=n
CONFIG_USE_DATE_CODE=n
CONFIG_FAKELIBC=y

