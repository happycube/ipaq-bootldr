#ifndef PBM_H_INCLUDED
#define PBM_H_INCLUDED


#define PPM_K "P6"
#define PGM_K "P5"

enum PBM_TYPE {UNKNOWN = 0,
	       PPM,
	       PGM,

};


struct pbm_info
{
    enum PBM_TYPE type;
    unsigned long width;
    unsigned long height;
    unsigned long maxVal;
    void *data;
};





void display_pbm(unsigned char *src,unsigned long size);





#endif  //PBM_H_INCLUDED
