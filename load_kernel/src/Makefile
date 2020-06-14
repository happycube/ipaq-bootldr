#
# These are OS neutral settings.
#
include ../../config.mk

#
# Local Overrides - no error if not present.
#  - Use this file to override anything in os.mk or config.mk.
#
-include ../../config.local.mk


ARCH_DEFS = -DCONFIG_MACH_$(CONFIG_MACH)=1 -DCONFIG_$(CONFIG_ARCH)=1

all: libload_kernel.a

CFLAGS=-I../include -I. -I/usr/include -O2 $(LOCAL_CFLAGS) ${ARCH_DEFS}


OBJECTS= compr_rtime.o compr_rubin.o compr_zlib.o cramfs.o crc32.o jffs2.o mini_inflate.o zImage.o pushpull.o

libload_kernel.a: $(OBJECTS)
	$(AR) r libload_kernel.a $(OBJECTS) 


clean:
	rm -f $(OBJECTS) libload_kernel.a 

distclean: clean
