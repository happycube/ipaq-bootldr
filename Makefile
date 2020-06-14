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
# Compaq Personal Server Monitor Makefile
# David Panariti -- port to NetBSD
# Jamey Hicks -- PCI configuration setup
# Chris McKillop -- port to QNX
#
.SUFFIXES: .small_o

VERSION_MAJOR = 2
VERSION_MINOR = 21
VERSION_MICRO = 12
VERSION_SPECIAL=

cvs_ver = BOOTLDR_${VERSION_MAJOR}_${VERSION_MINOR}_${VERSION_MICRO}


OS_NAME := $(shell uname -s | tr '[A-Z]' '[a-z]' )

#
# These are OS neutral settings.
#
include config.mk

#
# These are OS specific settings - "ln -s os_<os>.mk os.mk"
# Where <os> is one of linux, qnx, or netbsd.
#
include os_$(OS_NAME).mk

#
# Local Overrides - no error if not present.
#  - Use this file to override anything in os.mk or config.mk.
#
-include config.local.mk


#
# Normally you need not worry about the stuff below.
#

VER_DEFS = -DVERSION_MAJOR=$(VERSION_MAJOR) 
VER_DEFS += -DVERSION_MINOR=$(VERSION_MINOR) 
VER_DEFS += -DVERSION_MICRO=$(VERSION_MICRO) 
VER_DEFS += -DVERSION_SPECIAL=\"$(VERSION_SPECIAL)\"


GLOBAL_DEFS = -DBOOTLDR -DDATE=\"`date +%y-%m-%d_%H:%M`\" 

LOCAL_CFLAGS = -Wall -fpic

INCLUDES = $(OS_INCLUDES) $(LOCAL_INCLUDES) -I.
ALL_DEFS = $(ARCH_DEFS) $(VER_DEFS) $(FLASH_DEFS) $(GLOBAL_DEFS) $(OS_DEFS) $(LOCAL_DEFS)
CFLAGS = $(ALL_DEFS) $(OS_CFLAGS) $(LOCAL_CFLAGS) $(INCLUDES)
ARCH_DEFS = -DCONFIG_MACH_$(CONFIG_MACH)=1 -DCONFIG_$(CONFIG_ARCH)=1
SMALL_ARCH_DEFS = -DCONFIG_SMALL -DCONFIG_MACH_$(CONFIG_MACH)=1 -DCONFIG_$(CONFIG_ARCH)=1
SMALL_DEFS = $(SMALL_ARCH_DEFS) $(VER_DEFS) $(FLASH_DEFS) $(GLOBAL_DEFS) $(OS_WINCE_DEFS) 
SMALL_DEFS += $(LOCAL_DEFS) $(OS_DEFS)
SMALL_CFLAGS = $(SMALL_DEFS) $(OS_CFLAGS) $(LOCAL_CFLAGS) $(INCLUDES)
ASMFLAGS = $(ALL_DEFS) $(OS_ASMFLAGS) $(LOCAL_ASMFLAGS)
CLIBS = $(OS_CLIBS) $(LOCAL_CLIBS) -lz -lc
SMALL_CLIBS = $(OS_CLIBS) $(LOCAL_CLIBS) -lz -lc

.PHONY : mk_date_code_
.PHONY : tagver

TARGETS = bootldr
ifeq ($(CONFIG_MACH), IPAQ)
# TARGETS += ram-bootldr wince-bootldr 
    TARGETS += ram-bootldr 
    SHORT_ARCH_STR = sa
endif
ifeq ($(CONFIG_ARCH), PXA)
# TARGETS += ram-bootldr wince-bootldr 
    TARGETS += ram-bootldr 
    SHORT_ARCH_STR = pxa
endif
ifeq ($(CONFIG_MACH), SKIFF)
    SHORT_ARCH_STR = skiff
endif


all: $(TARGETS)

HDRS =  bootconfig.h bootldr.h btflash.h btpci.h buttons.h cpu.h \
	cyclone_boot.h hal.h heap.h pcireg.h regs-21285.h bsdsum.h modem.h \
	splashz.h aux_micro.h boot_flags.h lcd.h list.h \
	params.h partition.h pbm.h zUtils.h ymodem.h crc.h jffs.h md5.h skiff.h

#ifeq ($(OS_NAME), linux)
    HDRS+=splashz_linux.h
#endif

# BOOTO = boot.o
ifeq ($(CONFIG_ARCH), SA110)
  BOOTO = boot.o mmu-strongarm.o 
endif
ifeq ($(CONFIG_ARCH), SA1100)
  BOOTO = boot-sa1100.o mmu-strongarm.o
endif
ifeq ($(CONFIG_ARCH), PXA)
  BOOTO = boot-pxa.o mmu-pxa.o
endif
OTHEROBJS = bootldr.o \
	btflash.o flashif.o \
	buttons.o \
	heap.o \
	iohandles.o \
	xmodem.o \
	bootcmds.o \
        bsdsum.o \
        modem.o \
	getcmd.o \
	jffs2_commands.o \
	zUtils.o \
	commands.o \
	serial.o \
	util.o \
	params.o \
	infrared.o

SMALLOBJS = bootldr.small_o \
	btflash.small_o flashif.small_o \
	buttons.small_o \
	heap.small_o \
	xmodem.small_o \
        bsdsum.small_o \
        modem.small_o \
	getcmd.small_o \
	jffs2_commands.small_o \
	zUtils.small_o \
	commands.small_o \
	serial.small_o \
	infrared.small_o \
	util.small_o \
	params.small_o \
	bootcmds.small_o 

ifeq ($(CONFIG_FAKELIBC), y)
	OTHEROBJS += fakelibc.o
	SMALLOBJS += fakelibc.small_o
	ARCH_DEFS += -DCONFIG_FAKELIBC
	SMALL_ARCH_DEFS += -DCONFIG_FAKELIBC	
endif

ifeq ($(CONFIG_BIG_KERNEL),y)
ARCH_DEFS += -DCONFIG_BIG_KERNEL
SMALL_ARCH_DEFS += -DCONFIG_BIG_KERNEL
else
endif


LDFLAGS_C002 = $(LDOPT_PREFIX)-Twinceld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
LDFLAGS_C340 = $(LDOPT_PREFIX)-Tramld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
LDFLAGS = -Tbootld.ld -Bstatic $(OS_LDFLAGS)
ifeq ($(CONFIG_POWERMGR),y)
ARCH_DEFS += -DCONFIG_POWERMGR
endif

ifeq ($(CONFIG_VFAT_DEBUG),y)
  ARCH_DEFS += -DCONFIG_VFAT_DEBUG
endif

ifeq ($(CONFIG_MACH), ASSABET)
  ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  CONFIG_LCD  = y
  CONFIG_JFFS = y
  CONFIG_MD5  = y
  CONFIG_GZIP = y
  OTHEROBJS  += gpio.o 
  SMALLOBJS += 	gpio.small_o hal.small_o 


endif
ifeq ($(CONFIG_MACH), IPAQ)
  LDBASE_C002 = 0xc0022000
  LDBASE_C340 = 0xc3400000
  LDFLAGS_C002 = $(LDOPT_PREFIX)-Twinceld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDFLAGS_C340 = $(LDOPT_PREFIX)-Tramld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  # LDFLAGS = $(LDOPT_PREFIX)-Ttext=0x0 $(LDOPT_PREFIX)-Tdata=0x10700 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDFLAGS = $(LDOPT_PREFIX)-Tbootld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDBASE_0000 = 0x00
  LDFLAGS_HI = $(LDOPT_PREFIX)-Ttext=0x00020000 $(LDOPT_PREFIX)-Tdata=0x00030700 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  SMALL_ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  OTHEROBJS  += aux_micro.o 
  SMALLOBJS += aux_micro.o
  OTHEROBJS  += gpio.o 
  SMALLOBJS += 	gpio.small_o
  CONFIG_HAL=y
  CONFIG_LCD= y
  ## CONFIG_JFFS=y
  CONFIG_REFLASH=y
  CONFIG_MD5=y
  CONFIG_GZIP=y
  CONFIG_PROTECT_BOOTLDR=y
  CONFIG_H3600_SLEEVE = y
  CONFIG_PCMCIA=y
  CONFIG_IDE=y
  CONFIG_VFAT=y
  CONFIG_PACKETIZE=y
endif
ifeq ($(CONFIG_MACH), JORNADA720)
  ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  OTHEROBJS  += jornada720.o
  OTHEROBJS  += gpio.o 
  SMALLOBJS += 	gpio.small_o
endif
ifeq ($(CONFIG_MACH), JORNADA56X)
  ARCH_DEFS += -DCONFIG_MACH_JORNADA56X=1 -DCONFIG_INTEL_FLASH=1
  OTHEROBJS  += jornada720.o
  OTHEROBJS  += gpio.o 
  SMALLOBJS += 	gpio.small_o

endif
ifeq ($(CONFIG_MACH), SKIFF)
#  LDFLAGS = $(LDOPT_PREFIX)-Ttext=0x41000000 $(LDOPT_PREFIX)-Tdata=0x4100d000 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDBASE_0000 = 0x41000000
  LDFLAGS = $(LDOPT_PREFIX)-Tbootld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS) 
  ARCH_DEFS += -DCONFIG_AMD_FLASH=1
  OTHEROBJS  += gpio.o 
  SMALLOBJS += 	gpio.small_o
  CONFIG_PROTECT_BOOTLDR=n
  CONFIG_PCI = y
  CONFIG_PCMCIA=y
  CONFIG_IDE=y
  CONFIG_VFAT=y

endif
ifeq ($(CONFIG_MACH), SPOT)
#  ARCH_DEFS += -DCONFIG_MACH_SPOT=1 -DCONFIG_INTEL_FLASH=1 -DCONFIG_DRAM_BANK1 -DCONFIG_DRAM_BANK2 -DCONFIG_DRAM_BANK3
  ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  CONFIG_JFFS = y
  CONFIG_MD5  = y
  CONFIG_GZIP = y
  OTHEROBJS  += gpio.o 
  SMALLOBJS += 	gpio.small_o

endif
ifeq ($(CONFIG_MACH), GATOR)
  ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  CONFIG_JFFS = y
  CONFIG_MD5  = y
  CONFIG_GZIP = y
  OTHEROBJS  += gpio.o 
  SMALLOBJS += 	gpio.small_o

endif
ifeq ($(CONFIG_MACH), H3900)
  LDBASE_C002 = 0xA0022000
  LDBASE_C340 = 0xA3400000
  LDFLAGS_C002 = $(LDOPT_PREFIX)-Twinceld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDFLAGS_C340 = $(LDOPT_PREFIX)-Tramld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  # LDFLAGS = $(LDOPT_PREFIX)-Ttext=0x0 $(LDOPT_PREFIX)-Tdata=0x10700 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDFLAGS = $(LDOPT_PREFIX)-Tbootld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDBASE_0000 = 0x00
  LDFLAGS_HI = $(LDOPT_PREFIX)-Ttext=0x00020000 $(LDOPT_PREFIX)-Tdata=0x00030700 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  ARCH_DEFS += -DCONFIG_INTEL_FLASH=1 $(GCCOPT_PREFIX)-march=armv4
  SMALL_ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  OTHEROBJS  += aux_micro.o
  SMALLOBJS += aux_micro.o
  OTHEROBJS  += gpio.o 
  SMALLOBJS += 	gpio.small_o
  CONFIG_HAL=y
  CONFIG_LCD  = y
  CONFIG_REFLASH = y
  ## CONFIG_JFFS = y
  CONFIG_MD5  = y
  CONFIG_GZIP = y
  CONFIG_PROTECT_BOOTLDR=y
  CONFIG_H3600_SLEEVE = y
  CONFIG_PCMCIA=y
  CONFIG_IDE=y
  CONFIG_VFAT=y
  CONFIG_PACKETIZE=y
endif
ifeq ($(CONFIG_MACH), IPAQ3)
  LDBASE_C002 = 0xA0022000
  LDBASE_C340 = 0xA3400000
  LDFLAGS_C002 = $(LDOPT_PREFIX)-Twinceld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDFLAGS_C340 = $(LDOPT_PREFIX)-Tramld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  # LDFLAGS = $(LDOPT_PREFIX)-Ttext=0x0 $(LDOPT_PREFIX)-Tdata=0x10700 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDFLAGS = $(LDOPT_PREFIX)-Tbootld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDBASE_0000 = 0x00
  LDFLAGS_HI = $(LDOPT_PREFIX)-Ttext=0x00020000 $(LDOPT_PREFIX)-Tdata=0x00030700 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  ARCH_DEFS += -DCONFIG_INTEL_FLASH=1 -march=armv4
  SMALL_ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  OTHEROBJS  += aux_micro.o
  SMALLOBJS += aux_micro.o
  CONFIG_LCD  = n
  ## CONFIG_JFFS = y
  CONFIG_MD5  = y
  CONFIG_GZIP = y
  CONFIG_PROTECT_BOOTLDR=n
  CONFIG_H3600_SLEEVE = n
  CONFIG_PCMCIA=n
  CONFIG_IDE=n
  CONFIG_VFAT=n
  CONFIG_PACKETIZE=y
endif

ifeq ($(CONFIG_MACH), PXA1)
  LDBASE_C002 = 0xa0022000
  LDBASE_C340 = 0xa3400000
  LDFLAGS_C002 = $(LDOPT_PREFIX)-Twinceld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDFLAGS_C340 = $(LDOPT_PREFIX)-Tramld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  # LDFLAGS = $(LDOPT_PREFIX)-Ttext=0x0 $(LDOPT_PREFIX)-Tdata=0x10700 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDFLAGS = $(LDOPT_PREFIX)-Tbootld.ld $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  LDBASE_0000 = 0x00
  LDFLAGS_HI = $(LDOPT_PREFIX)-Ttext=0x00020000 $(LDOPT_PREFIX)-Tdata=0x00030700 $(LDOPT_PREFIX)-Bstatic $(OS_LDFLAGS)
  ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  SMALL_ARCH_DEFS += -DCONFIG_INTEL_FLASH=1
  CONFIG_LCD  = n
  CONFIG_JFFS = y
  CONFIG_MD5  = y
  CONFIG_GZIP = y
  CONFIG_PROTECT_BOOTLDR=y
  CONFIG_H3600_SLEEVE = n
  CONFIG_PCMCIA=n
  CONFIG_IDE=n
  CONFIG_VFAT=n
endif

ifeq ($(CONFIG_LOAD_KERNEL), y)
  ARCH_DEFS += -DCONFIG_LOAD_KERNEL=1
  OTHEROBJS += lkernel.o
  CLIBS += -Lload_kernel/src -lload_kernel -lz -lc
  ODEPS += load_kernel/src/libload_kernel.a
endif
ifeq ($(CONFIG_PCI), y)
  ARCH_DEFS += -DCONFIG_PCI=1
  OTHEROBJS += btpci.o
endif
ifeq ($(CONFIG_BZIP), y)
  CLIBS += -lbzip
  SMALL_CLIBS += -lbzip
endif
ifeq ($(CONFIG_GZIP), y)
  CFLAGS += -DCONFIG_GZIP
  CLIBS += -lz -lc
  SMALL_CLIBS += -lz -lc
endif
ifeq ($(CONFIG_REFLASH),y)
  GLOBAL_DEFS += -DCONFIG_REFLASH
  OTHEROBJS += reflash.o
  SMALLOBJS += reflash.small_o
endif
ifeq ($(CONFIG_LCD), y)
  GLOBAL_DEFS += -DCONFIG_LCD
  ifeq ($(CONFIG_ARCH),PXA)
    OTHEROBJS += lcd-pxa.o lcd-generic.o
    SMALLOBJS += lcd-pxa.small_o lcd-generic.small_o 
  endif
  ifeq ($(CONFIG_ARCH),SA1100)
    # use lcd-generic here one day
    OTHEROBJS += lcd.o
    SMALLOBJS += lcd.small_o
  endif
  OTHEROBJS += pbm.o
  SMALLOBJS += pbm.small_o
endif
ifeq ($(CONFIG_JFFS), y)
  ARCH_DEFS += -DCONFIG_JFFS=1
  OTHEROBJS += jffs.o crc.o
endif
ifeq ($(CONFIG_MD5), y)
  ARCH_DEFS += -DCONFIG_MD5=1
  OTHEROBJS += md5.o
  SMALLOBJS += md5.o
endif

## for second bank of dram
ifeq ($(CONFIG_DRAM_BANK1), y)
  ARCH_DEFS += -DCONFIG_DRAM_BANK1
endif

ifeq ($(CONFIG_PROTECT_BOOTLDR),y)
  ARCH_DEFS += -DCONFIG_PROTECT_BOOTLDR
endif

ifeq ($(CONFIG_ACCEPT_GPL),y)
  GLOBAL_DEFS += -DCONFIG_ACCEPT_GPL
  OTHEROBJS += bootlinux.o
  SMALLOBJS += bootlinux.o
endif
ifeq ($(CONFIG_HAL),y)
  GLOBAL_DEFS += -DCONFIG_HAL
  HDRS += hal.h
  OTHEROBJS += hal.o
  SMALLOBJS += hal.small_o
endif
ifeq ($(CONFIG_YMODEM),y)
  GLOBAL_DEFS += -DCONFIG_YMODEM
  OTHEROBJS += ymodem.o crc.o
  SMALLOBJS += ymodem.o crc.o
endif
ifeq ($(CONFIG_H3600_SLEEVE), y)
  ARCH_DEFS += -DCONFIG_H3600_SLEEVE
  HDRS += h3600_sleeve.h
  OTHEROBJS += h3600_sleeve.o h3600_pcmcia_sleeve.o
endif
ifeq ($(CONFIG_PCMCIA),y)
  ARCH_DEFS += -DCONFIG_PCMCIA
  HDRS += pcmcia.h
  OTHEROBJS += pcmcia.o
endif
ifeq ($(CONFIG_IDE),y)
  ARCH_DEFS += -DCONFIG_IDE
  HDRS += ide.h
  OTHEROBJS += ide.o
endif
ifeq ($(CONFIG_VFAT),y)
  ARCH_DEFS += -DCONFIG_VFAT
  HDRS += fs.h vfat.h
  OTHEROBJS += vfat.o
endif
ifeq ($(CONFIG_USB),y)
  HDRS += 
  ifeq ($(CONFIG_ARCH),SA1100)
    ARCH_DEFS += -DCONFIG_USB
    OTHEROBJS += usb-sa1110.o int.o intasm.o dma-sa1100.o
  endif
endif
ifeq ($(CONFIG_PACKETIZE),y)
  ARCH_DEFS += -DCONFIG_PACKETIZE
endif

OTHEROBJS += div0.o

OBJS = $(BOOTO) $(OTHEROBJS)
WINCE_OBJS = $(BOOTO) $(SMALLOBJS)

VER_STR=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)$(VERSION_SPECIAL)

#
# patch the a.out header with a branch around itself.
# needs to be done for the first download of a new
# a.out bootldr.
# The a.out bootldr will patch the header iff it detects an ARM
# ZMAGIC magic number.
#

bootldr: $(HDRS) $(OBJS) bootld.ld zbsdchksum $(ODEPS)
	$(MK_DATE_CODE)
	$(CROSS_COMPILE)$(LD) -v $(LDFLAGS) -o bootldr-elf $(BOOTO) $(OTHEROBJS) $(mak_DC_OBJ) $(CLIBS)
	$(CROSS_COMPILE)$(NM) -v -l bootldr-elf > bootldr.nm
	$(CROSS_COMPILE)$(OBJCOPY) -O binary -S bootldr-elf bootldr $(OBJCOPYFLAGS)
	./zbsdchksum 
	cp bootldr bootldr-$(SHORT_ARCH_STR)-$(VER_STR).bin
	gzip -c bootldr > bootldr.gz
	gzip -c bootldr-$(SHORT_ARCH_STR)-$(VER_STR).bin > bootldr-$(SHORT_ARCH_STR)-$(VER_STR).bin.gz
	$(MD5SUM) bootldr-$(SHORT_ARCH_STR)-$(VER_STR).bin > bootldr-$(SHORT_ARCH_STR)-$(VER_STR).bin.md5sum
#	cp bootldr-$(VER_STR).bin* /mirror/linux/feeds/bootldr	



winceld.ld:	bootldr.ld.in Makefile
	sed s/@LOAD_BASE_ADDR@/$(LDBASE_C002)/g bootldr.ld.in > $@

ramld.ld:	bootldr.ld.in Makefile
	sed s/@LOAD_BASE_ADDR@/$(LDBASE_C340)/g bootldr.ld.in > $@

bootld.ld:	bootldr.ld.in Makefile
	sed s/@LOAD_BASE_ADDR@/$(LDBASE_0000)/g bootldr.ld.in > $@

wince-bootldr: $(HDRS) $(WINCE_OBJS) winceld.ld zbsdchksum
	$(CROSS_COMPILE)$(LD) -v $(LDFLAGS_C002) -o wince-bootldr-elf $(BOOTO) $(SMALLOBJS) $(mak_DC_OBJ) $(SMALL_CLIBS)
	$(CROSS_COMPILE)$(NM) -v -l wince-bootldr-elf > wince-bootldr.nm
	$(CROSS_COMPILE)$(OBJCOPY) -O binary -S wince-bootldr-elf wince-bootldr $(OBJCOPYFLAGS)
	./zbsdchksum $@
	cp wince-bootldr bootldr-c000-$(VER_STR)
	$(MD5SUM) bootldr-c000-$(VER_STR) > bootldr-c000-$(VER_STR).md5sum

ram-bootldr: $(HDRS) $(OBJS) ramld.ld zbsdchksum
	$(CROSS_COMPILE)$(LD) -v $(LDFLAGS_C340) -o ram-bootldr-elf $(BOOTO) $(OTHEROBJS) $(mak_DC_OBJ) $(CLIBS)
	$(CROSS_COMPILE)$(NM) -v -l ram-bootldr-elf > ram-bootldr.nm
	$(CROSS_COMPILE)$(OBJCOPY) -O binary -S ram-bootldr-elf ram-bootldr $(OBJCOPYFLAGS)
	./zbsdchksum $@
	cp ram-bootldr bootldr-c340-$(VER_STR)
	gzip -c ram-bootldr > ram-bootldr.gz
	gzip -c bootldr-c340-$(VER_STR) > bootldr-c340-$(VER_STR).gz
	$(MD5SUM) bootldr-c340-$(VER_STR) > bootldr-c340-$(VER_STR).md5sum

bootldr.hex: bootldr
	hexdump -v -e '"0x%x\n"' bootldr > bootldr.hex

updateosl: wince-bootldr bootldr 
	echo "/* last extracted on `date` by $(USER) on $(HOST) */" > ram-bootldr.struct
	echo "/* compiled to run at address 0xc0022000 */" >> ram-bootldr.struct
	echo " " >> ram-bootldr.struct
	echo "EMBEDDED_IMAGE ram_bootldr_image = {" >> ram-bootldr.struct 
	echo "        0x0665d3e0, 0x21b93596, 0xc2213f2c, 0xf7b740ea, /* signature */ " >> ram-bootldr.struct
	echo "        0x3,      /* structure version */" >> ram-bootldr.struct
	echo "        MAX_IMAGE_SIZE,      /* max image size */" >> ram-bootldr.struct
	echo "        0xC0022000,          /* physical address of image */" >> ram-bootldr.struct
	echo "        `ls -l ./ram-bootldr | awk '{print $$5}'`, /*  $@ image size */" >> ram-bootldr.struct
	echo "        `date -r ram-bootldr +%s`,                   /* last modified time as time_t 0=unknown*/" >> ram-bootldr.struct
	echo "        $(VERSION_MAJOR),    /* $@ major version */" >> ram-bootldr.struct
	echo "        $(VERSION_MINOR),    /* $@ minor version */" >> ram-bootldr.struct
	echo "        $(VERSION_MICRO),    /* $@ micro version */" >> ram-bootldr.struct
	hexdump -v -e '"        0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x,\n"' ram-bootldr >> ram-bootldr.struct
	echo "};" >> ram-bootldr.struct
	echo " " >> ram-bootldr.struct
	echo "/* last extracted on `date` by $(USER) on $(HOST) */" > flash-bootldr.struct
	echo "/* compiled to run at address 0x00000000 */" >> flash-bootldr.struct
	echo " " >> flash-bootldr.struct
	echo "EMBEDDED_IMAGE flash_bootldr_image = {" >> flash-bootldr.struct 
	echo "        0xce83a165, 0x4ed944c6, 0x493ff54a, 0x520596d3, /* signature */ " >> flash-bootldr.struct
	echo "        0x3,      /* structure version */" >> flash-bootldr.struct
	echo "        MAX_IMAGE_SIZE,      /* max image size */" >> flash-bootldr.struct
	echo "        0x00000000,          /* physical address of image */" >> flash-bootldr.struct
	echo "        `ls -l ./bootldr | awk '{print $$5}'`, /*  flash-bootldr image size */" >> flash-bootldr.struct
	echo "        `date -r bootldr +%s`,                   /* last modified time as time_t 0=unknown*/" >> flash-bootldr.struct
	echo "        $(VERSION_MAJOR),    /* flash-bootldr major version */" >> flash-bootldr.struct
	echo "        $(VERSION_MINOR),    /* flash-bootldr minor version */" >> flash-bootldr.struct
	echo "        $(VERSION_MICRO),    /* flash-bootldr micro version */" >> flash-bootldr.struct
	hexdump -v -e '"        0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x,\n"' bootldr >> flash-bootldr.struct
	echo "};" >> flash-bootldr.struct
	echo " " >> flash-bootldr.struct
	cat ram-bootldr.struct flash-bootldr.struct > wce-temp.struct
	cp wce-temp.struct ram-bootldr.struct
	echo '/:+++/+1,/:---/-1d' > bootldr_image.ed
	echo 'w'                 >> bootldr_image.ed
	echo '/:+++/r ram-bootldr.struct' >> bootldr_image.ed
	echo '/:+++/+1,/:---/-1s/0x        ,//g' >> bootldr_image.ed
	echo 'w'                 >> bootldr_image.ed
	echo 'q'                 >> bootldr_image.ed
	ed - ../windowsce/osloader/bootldr_image.cpp < bootldr_image.ed
	rm ram-bootldr.struct bootldr_image.ed wce-temp.struct flash-bootldr.struct

boot.o: boot.s $(HDRS)
	$(CROSS_COMPILE)$(ASM) $(ASMFLAGS) boot.s

boot-sa1100.o: boot-sa1100.s $(HDRS)
	$(CROSS_COMPILE)$(ASM) $(ASMFLAGS) $(OS_INCLUDES) boot-sa1100.s

boot-pxa.o: boot-pxa.s $(HDRS)
	$(CROSS_COMPILE)$(ASM) $(ASMFLAGS) $(OS_INCLUDES) boot-pxa.s

intasm.o: intasm.s $(HDRS)
	$(CROSS_COMPILE)$(ASM) $(ASMFLAGS) intasm.s


zerosum: zerosum.c
	$(HOSTCC) -o zerosum zerosum.c

zbsdchksum: zbsdchksum.c
	$(HOSTCC) -o zbsdchksum zbsdchksum.c

load_kernel/src/libload_kernel.a: load_kernel/src/jffs2.c load_kernel/src/mini_inflate.c load_kernel/src/compr_rubin.c load_kernel/src/compr_zlib.c load_kernel/src/pushpull.c load_kernel/src/zImage.c load_kernel/src/compr_rtime.c load_kernel/src/cramfs.c load_kernel/src/crc32.c 
	$(MAKE) -C load_kernel/src CC="$(CROSS_COMPILE)$(CC)" AR="$(CROSS_COMPILE)$(AR)" LOCAL_CFLAGS="$(LOCAL_CFLAGS)" libload_kernel.a
#
# make a binary splash image and encode it as a char string 
# in C.
# Images are PNMs and are 320x240x8b/color
# I've only used gimp to make them.  zpnm may get confused on other
# pnm files, since the PNM spec is so liberal.
# zpnm.py (python 2.x) converts the 8bbp image to 4bpp and 
# compresses it w/ libz.  
# It is NOT in gzip format, but raw compressed data.
# 
version_${VER_STR}:version_${VER_STR}
	touch version_${VER_STR}


splashz_linux.h:version_${VER_STR} $(CONFIG_GIMP_SPLASH_PNM)
	ppmlabel -angle 180 -background transparent -colour black -y 70 -x 183 -text "Rev ${VER_STR}" $(CONFIG_GIMP_SPLASH_PNM) > $(CONFIG_GIMP_SPLASH_PNM)_tmp
	./zpnm.py $(CONFIG_GIMP_SPLASH_PNM)_tmp | \
		./bin2h.py splash_zimg > \
			$@

#
# make a pure binary image suitable for downloading with 
# bootldr's lcdzimg command.
# see splashz_linux.h for file format info.
#
splashz_linux:
	./zpnm.py $(CONFIG_GIMP_SPLASH_PNM) > $@

clean:
	rm -f $(OBJS) $(WINCE_OBJS) bootldr-elf bootldr bootldr.nm \
		ram-bootldr ram-bootldr-elf ram-bootldr.nm \
		wince-bootldr wince-bootldr-elf wince-bootldr.nm \
                zbsdchksum generated_date_code.h \
                generated_date_code.c generated_date_code.o \
                zbsdchksum \
		bootldr-* version_* *.ld \
		pics/splash.pnm_tmp
	$(MAKE) -C load_kernel/src clean

distclean: clean
	rm -f bootldr-000* bootldr-c000*
	rm -f pics/splash.pnm_tmp ram-bootldr.gz splashz_linux.h bootldr.gz
	$(MAKE) -C load_kernel/src distclean

tags etags:
	rm -f TAGS
	find . -name '*.[chs]' -exec etags -af $$PWD/TAGS {} \; -print


tagver:
	cvs tag ${cvs_ver}

ifeq ($(CONFIG_USE_DATE_CODE),y)

CFLAGS += -DCONFIG_USE_DATE_CODE

mak_DC_FILE = generated_date_code
mak_DC_H_FILE = $(mak_DC_FILE).h
mak_DC_C_FILE = $(mak_DC_FILE).c
mak_DC_OBJ = $(mak_DC_FILE).o
DATE_CODE_VAR := _bootldr_date_code
DATE_CODE_DEF	:= BOOTLDR_DATE_CODE
MK_DATE_CODE = $(MAKE) mk_date_code_
mk_date_code_:
	rm -f $(mak_DC_C_FILE)
	rm -f $(mak_DC_H_FILE)
	echo "/*" > $(mak_DC_H_FILE)
	echo " * Generated file.  DO NOT EDIT." >> $(mak_DC_H_FILE)
	echo " * Well, you can if you want to, but, why?" >> $(mak_DC_H_FILE)
	echo " */" >> $(mak_DC_H_FILE)
	echo "#define $(DATE_CODE_DEF) \"`date`\"" >> $(mak_DC_H_FILE)
	echo "/*" > $(mak_DC_C_FILE)
	echo " * Generated file.  DO NOT EDIT." >> $(mak_DC_C_FILE)
	echo " * Well, you can if you want to, but, why?" >> $(mak_DC_C_FILE)
	echo " */" >> $(mak_DC_C_FILE)
	echo "#include \"$(mak_DC_H_FILE)\"" >> $(mak_DC_C_FILE)
	echo "char $(DATE_CODE_VAR)[]=$(DATE_CODE_DEF);" >> $(mak_DC_C_FILE)
	$(CROSS_COMPILE)$(CC) -c -o $(mak_DC_OBJ) $(mak_DC_C_FILE)
else
mak_DC_OBJ=
MK_DATE_CODE =
endif

.c.small_o: $(HDRS)
	$(CROSS_COMPILE)$(CC) $(SMALL_CFLAGS) -c -o $@ $<

.c.o: $(HDRS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c -o $@ $<
