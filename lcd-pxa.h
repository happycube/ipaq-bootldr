#define	LCD_MAX_BPP (16)    /* that we allow in this simple situation */
#define	LCD_BPP	    (16)
#define	LCD_XRES    (320)
#define	LCD_YRES    (240)

#define LCD_NUM_PIXELS()	    (LCD_XRES * LCD_YRES)
#define LCD_NUM_DISPLAY_BYTES(bpp)  ((LCD_NUM_PIXELS() * (bpp) + 7)/8)

#define	LCD_FB_SIZE(bpp) ((LCD_NUM_DISPLAY_BYTES(bpp)))
#define PALETTE_MEM_SIZE(x) 0

#define	LCD_FB_MAX()		    LCD_FB_SIZE(LCD_MAX_BPP)
#define	LCD_FB_IMAGE_OFFSET(bpp)    0
#define	LCD_FB_IMAGE(p, bpp)	    (((char*)(p)) + LCD_FB_IMAGE_OFFSET(bpp))
