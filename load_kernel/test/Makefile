DEFS = -DHAVE_CONFIG_H -I/usr/include -I. -I../include

load_kernel_test_OBJECTS =  \
load_kernel.o \
load_kernel_test.o \
mini_inflate.o \
jffs2.o \
compr_rtime.o \
compr_rubin.o \
compr_zlib.o \
cramfs.o zImage.o crc32.o

load_kernel_test_extra_SOURCES =  \
mini_inflate.c \
jffs2.c \
compr_rtime.c \
compr_rubin.c \
compr_zlib.c \
cramfs.c zImage.c crc32.c

CC=gcc

#CFLAGS = -O2  -Wall -fomit-frame-pointer -g
CFLAGS = -O2  -Wall -g
COMPILE = $(CC) $(DEFS) $(CFLAGS) -DUSER_SPACE_TEST
LINK = $(CC) $(CFLAGS) $(LDFLAGS) -o $@


load_kernel_test: $(load_kernel_test_OBJECTS)
	@rm -f load_kernel_test
	$(LINK) $(load_kernel_test_OBJECTS) -o $@

$(load_kernel_test_extra_SOURCES):
	ln -s ../src/$@

.c.o:
	$(COMPILE) -c $<

clean:
	@rm -f *.o load_kernel_test