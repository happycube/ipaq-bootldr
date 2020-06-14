#include "bootldr.h"
#include "heap.h"
#include "lcd.h"
#include "pbm.h"


#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"

#ifndef DIM
#define	DIM(a)	(sizeof(a)/sizeof(a[0]))
#endif



extern byte strtoul_err;

static int getPBMInfo(unsigned char *src,struct pbm_info *pI);
static unsigned char *skipSpaces(unsigned char *src);
static unsigned char *skipComments(unsigned char *src);
static void extractPPMToColor(unsigned char *fb,struct pbm_info *pI);
static void extractPGMToColor(unsigned char *fb,struct pbm_info *pI);


struct maxValMap{
    unsigned char val;
    unsigned char shift;
};


static struct maxValMap mvMap[] = {
    {255,4},
    {127,3},
    {65,2},
    {31,1},
    {15,0},
    {7,0},
    {3,0}
};


static unsigned char *skipSpaces(unsigned char *src)
{
    // skip newlines
    while (isspace(*src))
	*src++;
    return src;
    
}

static unsigned char *skipComments(unsigned char *src)
{
    // skip to the end of the comment line
    while ((*src != '\r') && (*src != '\n'))
	*src++;
    src = skipSpaces(src);
    return src;

}

static void extractPPMToColor(unsigned char *fb,struct pbm_info *pI)
{
    char *pSrc;    
    int i;
    unsigned short  *pShort = (unsigned short *) fb;
    unsigned char shift;

    putLabeledWord("lcd fb max = ",LCD_FB_MAX());
    
    pSrc = pI->data;
    // now pSrc points to the start of the image data
    shift = 4;    
    for (i=0; i < DIM(mvMap);i++)
	if (pI->maxVal == mvMap[i].val)
	    shift = mvMap[i].shift;
    
    for (i=0; i < pI->width*pI->height;i++){
	*pShort = 0x0000;
	*pShort |= ((*pSrc++ & (0xf<<shift))>>shift)<< 8; // red
	*pShort |= ((*pSrc++ & (0xf<<shift))>>shift)<< 4; // green
	*pShort++ |= ((*pSrc++ & (0xf<<shift))>>shift)<< 0; // blue

    }
}


static void extractPGMToColor(unsigned char *fb,struct pbm_info *pI)
{
    char *pSrc;    
    int i;
    unsigned short  *pShort = (unsigned short *) fb;
    unsigned char shift;

    putLabeledWord("lcd fb max = ",LCD_FB_MAX());
    
    pSrc = pI->data;
    // now pSrc points to the start of the image data
    shift = 4;    
    for (i=0; i < DIM(mvMap);i++)
	if (pI->maxVal == mvMap[i].val)
	    shift = mvMap[i].shift;
    
    for (i=0; i < pI->width*pI->height;i++){
	*pShort = 0x0000;
	*pShort |= ((*pSrc & (0xf<<shift))>>shift)<< 8; // red
	*pShort |= ((*pSrc & (0xf<<shift))>>shift)<< 4; // green
	*pShort++ |= ((*pSrc++ & (0xf<<shift))>>shift)<< 0; // blue

    }
}





static int getPBMInfo(unsigned char *src,struct pbm_info *pI)
{
    char s[10];
    char *pSrc = src;
    char *pS = &s[0];
    unsigned long w,h,m;

    // get the pbm type
    memcpy(pS,pSrc,2);
    pS+=2;
    pSrc+=2;
    *pS++ = '\0';
    if (!strcmp(s,PPM_K)){
	putstr("this is a ppm file\r\n");
	pI->type = PPM;
	
    }
    else if (!strcmp(s,PGM_K)){
	putstr("this is a pgm file\r\n");
	pI->type = PGM;
    }
    else{
	putstr("unknown file type, exiting...\r\n");
	pI->type = UNKNOWN;
	return 0;
    }
    // skip newlines
    pSrc = skipSpaces(pSrc);
    if (*pSrc == '#')
	pSrc = skipComments(pSrc);
    // get the width
    pS = &s[0];
    while (!isspace(*pSrc))
	*pS++ = *pSrc++;
    *pS++ = '\0';
    w = strtoul(s,NULL,0);
    if (strtoul_err) {
	putstr("bad width\n\r");
	return 0;
    }
    
    pSrc = skipSpaces(pSrc);
    // get the height
    pS = &s[0];
    while (!isspace(*pSrc))
	*pS++ = *pSrc++;
    *pS++ = '\0';
    h = strtoul(s,NULL,0);
    if (strtoul_err) {
	putstr("bad height\n\r");
	return 0;
    }

    // skip spaces
    pSrc = skipSpaces(pSrc);
    if (*pSrc == '#')
	pSrc = skipComments(pSrc);
    // get the maxVal
    pS = &s[0];
    while (!isspace(*pSrc))
	*pS++ = *pSrc++;
    *pS++ = '\0';
    m = strtoul(s,NULL,0);
    if (strtoul_err) {
	putstr("bad width\n\r");
	return 0;
    }

    pSrc = skipSpaces(pSrc);

    pI->width = w;
    pI->height = h;
    pI->maxVal = m;    
    pI->data = pSrc;
    return 1;
    
}



void display_pbm(unsigned char *src,unsigned long size)
{
    char* fb = lcd_get_image_buffer();
    struct pbm_info I;
    
    putLabeledWord("loading pbm from location: ", (u32)src);
    putLabeledWord("of size: ",size);
    putLabeledWord("fb at location: ", (u32)fb);

    if (!getPBMInfo(src,&I)){
	putstr("error reading pbm header\r\n");
	return;
    }
    

    
    putLabeledWord("width :",I.width);
    putLabeledWord("height :",I.height);
    putLabeledWord("maxVal :",I.maxVal);
    putLabeledWord("data :",(u32)I.data);

    if (I.type == PPM)
	extractPPMToColor(fb,&I);
    else if (I.type == PGM)
	extractPGMToColor(fb,&I);
    
    lcd_display_bitmap(fb, 320*240, lcd_type, NULL);    
    
}

