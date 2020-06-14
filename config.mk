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
# one of SA110, SA1100, or PXA
# what these are:
# skiff: Compaq Personal Server research prototype
# ipaq: Compaq iPAQ - H31xx, H36xx, H37xx, H38xx
# assabet: Intel SA1110 evaluation board 
# what setting to use
# skiff: SA110
# Ipaq 31xx,36xxx,37xx,38xx: SA1100
# Ipaq 39xx,54xx: PXA

CONFIG_ARCH=SA1100

#
# one of SKIFF, IPAQ, ASSABET, JORNADA720, JORNADA56X, SPOT, GATOR, H3900
# currently unused for SA1110 platforms
# what setting to use
# skiff: SKIFF
# Ipaq 31xx,36xxx,37xx,38xx: IPAQ
# Ipaq 39xx,54xx: H3900

CONFIG_MACH=IPAQ


#
# use this to increase kernel partition size  
# (and to swap it with params sector)
#
CONFIG_BIG_KERNEL=y

#
# activate power management code?
#
CONFIG_POWERMGR=y

#
# activate code to track last compile/link?
#
CONFIG_USE_DATE_CODE=y

#
# allow load of kernel from JFFS2 partition?
#
CONFIG_LOAD_KERNEL=y


#
# Protect bootldr flash sector on boot?
#
CONFIG_PROTECT_BOOTLDR=y

#
# Accept GPL code as part of bootldr?
#
CONFIG_ACCEPT_GPL=y

#
# ymodem? (depends on CONFIG_ACCEPT_GPL)
#
CONFIG_YMODEM=y

#
# fake libc?
#
CONFIG_FAKELIBC=n


CONFIG_USB=y
