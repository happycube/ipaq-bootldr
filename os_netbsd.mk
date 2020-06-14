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

# LDFLAGS = -T 0x41000000 -Bstatic
# CFLAGS = -O2 $(CDEFS)
# CLIBS = -lc -lm
# OSDEFS = -DNoLibC

#
# Dunno what needs to be done for NetBSD.
#
-include os_linux.mk

