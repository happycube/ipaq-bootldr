#include "bootldr.h"
#include "heap.h"
#include "zlib.h"
#include "zUtils.h"
#include <string.h>


static Byte *check_header(Byte *data);
static  uLong getLong (Byte *d);


static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */
/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */


static Byte *check_header(Byte *data)
{
    int method; /* method byte */
    int flags;  /* flags byte */
    uInt len;
    int c;

    /* Check the gzip magic header */
    for (len = 0; len < 2; len++) {
	c = *data++;
	if (c != gz_magic[len]) {
	    return --data;
	}
    }
    method = *data++;
    flags = *data++;


    /* Discard time, xflags and OS code: */
    for (len = 0; len < 6; len++) data++;

    if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
	len  =  (uInt)*data++;
	len += ((uInt)*data++)<<8;
	/* len is garbage if EOF but the loop below will quit anyway */
	while (len-- != 0)
	    data++;
    }
    if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
	while ((c = *data++) != 0) ;
    }
    if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
	while ((c = *data++) != 0 ) ;
    }
    if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
	for (len = 0; len < 2; len++)
	    data++;
    }
    return data;
}

static  uLong getLong (Byte *d)
{
    uLong x = (uLong)*d++;
    int c;

    x += ((uLong)*d++)<<8;
    x += ((uLong)*d++)<<16;
    c = (int) *d;
    x += ((uLong)c)<<24;
    return x;
}



int verifyGZipImage(unsigned long address, size_t *sizeP)
{
    char tiny_buf[ZLIB_CHUNK_SIZE];
    struct bz_stream z;
    unsigned long size = *sizeP;
    unsigned long ctr = 0;
    
    putstr("Verifying gzipped image\r\n");
    
    gzInitStream(address,size,&z);
    z.err = Z_OK;    
    while (z.err == Z_OK){
	if ((ctr++ % SZ_1K) == 0)
	    putstr(".");
	if ((ctr++ % SZ_64K) == 0)
	    putstr("\r\n");
	gzReadChunk(&z,tiny_buf);
    }
    putstr("\r\n");
    
    
    *sizeP = z.stream.total_out;
    
    
    putLabeledWord("verifyGZipImage: calculated CRC = 0x",z.crc32);
    putLabeledWord("verifyGZipImage: read       CRC = 0x",z.read_crc32);
    return (z.crc32 == z.read_crc32);
    
}



/******************************************
 *  sets up the stream for use by gzReadChunk
 *  
 *  
 *
 * 
 ******************************************/

void gzInitStream(unsigned long srcAddr,unsigned long size,struct bz_stream *z)
{

    z->crc32 = 0L;    
    z->stream.next_in = (Bytef*)srcAddr;
    // strip the header
    z->stream.next_in = (Byte *) check_header(z->stream.next_in)  ;

    z->stream.avail_in = size;
    z->stream.zalloc = (alloc_func)zalloc;
    z->stream.zfree = (free_func)zfree;
    z->err = inflateInit2(&(z->stream), -15);
    if (z->err != Z_OK) {
	putLabeledWord("inflateInit2 failed", z->err);
	return;
    }
    
}
/**************************************************/
/*  reads a chunk (usually 64 bytes) worth of data from the
 *  mem region.  returns num bytes stuffed in buf.
 *  the stream must be set up by the caller
 *
 * 
 ******************************************/

int gzReadChunk(struct bz_stream *z,unsigned char *buf)
{    
    return gzRead(z,buf,ZLIB_CHUNK_SIZE);
}


int gzRead(struct bz_stream *z,unsigned char *buf,int num)
{    
    int ret =0;
    
    z->stream.next_out = buf;
    z->stream.avail_out = num;
    
    if (z->stream.avail_in>0) {
	//putLabeledWord("pre inflate z_err = 0x",z->err);	
        z->err = inflate(&(z->stream), Z_FULL_FLUSH);
	//putLabeledWord("post inflate z_err = 0x",z->err);
	//putLabeledWord("post inflate z->stream.total_out = 0x",z->stream.total_out);	
        if ((z->err != Z_OK) && (z->err != Z_STREAM_END)){
	    putLabeledWord("inflate error :0x",z->err);
	    putstr("stream msg = ");
	    putstr(z->stream.msg);
	    putstr("\r\n");
	}
	z->crc32 = crc32(z->crc32,buf,num - z->stream.avail_out);
	ret = num - z->stream.avail_out;	
    }
    if (z->err == Z_STREAM_END){
	z->read_crc32 = getLong(z->stream.next_in);
	if (z->read_crc32 != z->crc32){
	    putLabeledWord("calc crc32 is 0x",z->crc32);
	    putLabeledWord("read crc32 is 0x",z->read_crc32);
	}
	
	inflateEnd(&(z->stream));
    }
    return ret;
    
}


int isGZipRegion(unsigned long address)
{
    uInt len;
    int c;
    Byte *data = (Byte *) address;
    
    
    /* Check the gzip magic header */
    for (len = 0; len < 2; len++) {
	c = *data++;
	if (c != gz_magic[len]) {
	    return 0;
	}	
    }
    return 1;
}

int gUnZip(unsigned long address, size_t *pSize, unsigned long dAddress)
{    
    char tiny_buf[ZLIB_CHUNK_SIZE];
    struct bz_stream z;
    Byte *p;
    unsigned long size = *pSize;
    
    
    gzInitStream(address,size,&z);
    
    
    
    p = (Byte *) dAddress;
    
    while ((size = gzReadChunk(&z,tiny_buf))) {
	memcpy(p,tiny_buf,size);
	p += size;	
	if (z.err != Z_OK) break;
	
    }
    putLabeledWord("finished loading with runningCRC = 0x",z.crc32);
    

    *pSize = z.stream.total_out;
    
    putLabeledWord("uncompressed length = 0x", *pSize);
    putLabeledWord("total_in = 0x", z.stream.total_in);
    putLabeledWord("read_crc returns = 0x", z.read_crc32);
    
    size = crc32(0,(const void *) dAddress,*pSize);

    putLabeledWord("crc32 static calc= 0x", size);

    return (size == z.read_crc32);
    
}

