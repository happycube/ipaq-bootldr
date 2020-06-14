#ifndef ZUTILS_H_INCLUDED
#define ZUTILS_H_INCLUDED

#define ZLIB_CHUNK_SIZE 64

struct bz_stream 
{
    z_stream stream; // the standard zlib stream
    unsigned long crc32;
    unsigned long read_crc32;
    int err;
};

int verifyGZipImage(unsigned long address, size_t *sizeP);
int gzReadChunk(struct bz_stream *z,unsigned char *buf);
void gzInitStream(unsigned long srcAddr,unsigned long size,struct bz_stream *z);
int isGZipRegion(unsigned long address);
int gzRead(struct bz_stream *z,unsigned char *buf,int num);
int gUnZip(unsigned long address,size_t *pSize,unsigned long dAddress);


    







#endif //ZUTILS_H_INCLUDED
