#include "bootldr.h"
#include "commands.h"
#include "btpci.h"
#include "btflash.h"
#include "btusb.h"
#include "heap.h"
#include "modem.h"
#include "xmodem.h"
#include "lcd.h"
#include "mmu.h"
#include "params.h"
#include "zlib.h"
#if defined(__linux__) || defined(__QNXNTO__)
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
#endif
#include "lcd-pxa.h"
#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"
#include "ipaq-egpio.h"
#include "lcd-pxa.h"

#undef __REG
#define __REG(x) *((volatile unsigned long *) (x))

void set_pixel_value(unsigned short row, unsigned short col, unsigned short val);
unsigned short get_pixel_value(unsigned short row, unsigned short col);

extern struct bootblk_param param_splash_filename;
extern struct bootblk_param param_splash_partition;

enum lcd_type lcd_type = LCDP_end_sentinel;
static lcd_params_t *lcd_params;
/* extra at end for possible DMA overrun */
// this includes the palette
unsigned long lcd_frame_buffer[LCD_FB_MAX() + 16];
// start of actual image
unsigned char *lcd_image_buffer;


/* PXA LCD DMA descriptor */
struct pxafb_dma_descriptor {
	unsigned int fdadr;
	unsigned int fsadr;
	unsigned int fidr;
	unsigned int ldcmd;
} __attribute__ (( aligned (16)));

/*
 *  pxafb_disable_lcd_controller():
 *    	Disables LCD controller by and enables LDD interrupt.
 * The controller_state is not changed until the LDD interrupt is
 * received to indicate the current
 * frame has completed.  Platform specific hardware disabling is also included.
 */
static void pxafb_disable_lcd_controller(lcd_params_t*   params)
{
    putstr("Disabling LCD controller\n\r");
    
    lcd_light(0, 0);
    
    LCCR0 &= ~(LCCR0_LDM);	/* Enable LCD Disable Done Interrupt */
    LCCR0 &= ~(LCCR0_ENB);	/* Disable LCD Controller */

    LCSR = 0xffffffff;	/* Clear LCD Status Register */

    //CTL_REG_WRITE(GPIO_BASE+GPIO_GPCR_OFF,params->gpio);

    clr_h3600_egpio(IPAQ_EGPIO_LCD_ON);    
}

static unsigned short*	cur_pal;
static int		cur_bpp = LCD_BPP;


static struct pxafb_dma_descriptor dmas; 

/*
 *  pxafb_enable_lcd_controller():
 *    	Enables LCD controller.  If the controller is already enabled, it is first disabled.
 *      This forces all changes to the LCD controller registers to be done when the 
 *      controller is disabled.  Platform specific hardware enabling is also included.
 */
static void
pxafb_enable_lcd_controller(
    lcd_params_t*   params)
{
    struct pxafb_dma_descriptor *dma = &dmas;

    pxafb_disable_lcd_controller(params);
    
    putstr("Enabling LCD controller\n\r"); drain_uart();

#if 0
    /* make sure the GPIO pins are carrying LCD data */
    GPIO_GAFR_WRITE(params->gafr);/* alt function */
    GPIO_GPDR_WRITE(params->gpdr);/* outputs */
#endif   
 
    /* Make sure the mode bits are present in the first palette entry */
    cur_pal = params->v_palette_base;
    cur_bpp = params->bits_per_pixel;
    lcd_image_buffer = (unsigned char *) (unsigned short*)LCD_FB_IMAGE(lcd_frame_buffer,
							   cur_bpp);
    dma->fsadr = vaddr_to_paddr (lcd_image_buffer);
    dma->fidr = 0;
    dma->ldcmd = LCD_FB_SIZE (cur_bpp);
    dma->fdadr = vaddr_to_paddr (dma);
    
    putLabeledWord("fdaddr=", dma->fdadr);
    putLabeledWord("dma=", dma);
    
    /* Sequence from 11.7.10 */
    LCCR3 = params->lccr3;
    LCCR2 = params->lccr2;
    LCCR1 = params->lccr1;
    LCCR0 = params->lccr0 & ~LCCR0_ENB;

    FDADR0 = dma->fdadr;
    FDADR1 = 0;
    LCCR0 = params->lccr0;

    putLabeledWord("LCCR0=", LCCR0);
    putLabeledWord("LCCR0VAL=", params->lccr0);
    
    //set_egpio(EGPIO_IPAQ_LCD_ON | EGPIO_IPAQ_LCD_PCI |
    //EGPIO_IPAQ_LCD_5V_ON | EGPIO_IPAQ_LVDD_ON);
    set_h3600_egpio(IPAQ_EGPIO_LCD_ON);

#if 0
    CTL_REG_WRITE(GPIO_BASE+GPIO_GPDR_OFF,params->gpdr);
    CTL_REG_WRITE(GPIO_BASE+GPIO_GPSR_OFF,params->gpio);
#endif   
 
    putstr("after EGPIO \n\r"); drain_uart();
    putLabeledWord("FDADR0=", FDADR0);
    putLabeledWord("LCCR0=", LCCR0);
    putLabeledWord("LCCR1=", LCCR1);
    putLabeledWord("LCCR2=", LCCR2);
    putLabeledWord("LCCR3=", LCCR3);
    putLabeledWord("params=0x", (unsigned long)params);

    set_h3600_egpio(IPAQ_EGPIO_LCD_ON);
    lcd_light(1, 5);
}



void
setup_mono4_palette()
{

    int		    num_pixels,pixel;
    pixel = 1;	
    for (num_pixels=0; num_pixels < 8; num_pixels++){	    
	lcd_frame_buffer[num_pixels] = 0;
	lcd_frame_buffer[num_pixels] = ((pixel-1) & 0xffff);
	lcd_frame_buffer[num_pixels] |= (pixel & 0xffff) << 16;
	pixel += 2;
    }
}


unsigned short primaries[] =
{
    0xF800,			/* red */
    0x07e0,			/* green */
    0x001e,			/* blue */
};
    
void
lcd_setup_default_image(
    int	params_id)
{
    int	    x, y;
    int	    pixel;
    unsigned short*   bufp = (unsigned short*)LCD_FB_IMAGE(lcd_frame_buffer,
							   cur_bpp);
    putstr("setup def img\n\r"); drain_uart();
    
    pixel = 0;
    for (y = 0; y < LCD_YRES; y++) {
	for (x = 0; x < LCD_XRES; x++) {
	    *bufp++ = pixel;
	}
	if ((y % (LCD_YRES/3)) == 0)
	    pixel = primaries[y / (LCD_YRES/3)];
    }

    putstr("img done\n\r"); drain_uart();
}

    
lcd_params_t lcd_color_params = {
    0,
    //LCCR0
    (LCCR0_PAS | LCCR0_LDM | LCCR0_SFM | LCCR0_IUM | LCCR0_EFM | LCCR0_QDM | LCCR0_BM  | LCCR0_OUM | LCCR0_ENB),
    //LCCR1
    LCCR1_DisWdth( LCD_XRES ) +
      LCCR1_HorSnchWdth( 3 ) +
      LCCR1_BegLnDel( 0xC ) +
      LCCR1_EndLnDel( 0x11 ),
    //LCCR2
    LCCR2_DisHght( LCD_YRES ) +
      LCCR2_VrtSnchWdth( 3 )+
      LCCR2_BegFrmDel( 6 ) +
      LCCR2_EndFrmDel( 17 ),
    //LCCR3
    (LCCR3_16BPP | LCCR3_HorSnchL | LCCR3_VrtSnchL | /* PCD */ 0xa),
    // 3600 doesnt use gpio for lcd on
    0,
    EGPIO_IPAQ_LCD_ON | EGPIO_IPAQ_LCD_PCI |
    EGPIO_IPAQ_LCD_5V_ON | EGPIO_IPAQ_LVDD_ON,
    0xff<<2,
    0xff<<2,
    //FrameBuffer Base
    (unsigned short*)lcd_frame_buffer,
    //BitPerPixel
    LCD_BPP
};

/*
 * must match the values of the LCDP_ enums
 */
lcd_params_t*	params_list[] =
{
    &lcd_color_params,		/* color LCD panel */
};

lcd_params_t*
lcd_get_params(
    int		    params_id,
    lcd_params_t*   params)
{
    if (params_id >= LCDP_CUSTOM) {
	if (params_id > LCDP_CUSTOM) {
	    putLabeledWord("illegal params_id: 0x", params_id);
	    return (NULL);
	}
    }
    else
	params = params_list[params_id];

    return (params);
}


void
lcd_default_init(
    int		    params_id,
    lcd_params_t*   params)
{
    cur_pal = lcd_color_params.v_palette_base;
    lcd_setup_default_image(params_id);
    params = lcd_get_params(params_id, params);
    if (params == NULL) {
	putstr("lcd_ddefault_init: params is NULL\n\r");
	return;
    }
	

    pxafb_enable_lcd_controller(params);
}

void
lcd_display_bitmap(
    char*	    bitmap_in,
    size_t	    len,
    int		    params_id,
    lcd_params_t*   params)
{
    int	    pal_size = PALETTE_MEM_SIZE(params->bits_per_pixel);
    int	    num_pixels;
    unsigned long	    pixel;
    unsigned short* bufp = (unsigned short*)LCD_FB_IMAGE(lcd_frame_buffer,
							 cur_bpp);
    unsigned short* bmp = (unsigned short*)bitmap_in;
    unsigned short* bmp2 = (unsigned short*)bitmap_in + len;

    
#if 0
    putLabeledWord("params_id 1=", params_id);
    putLabeledWord("cur_bpp 1=", cur_bpp);
    putLabeledWord("bufp 1=", bufp);
    putLabeledWord("bitmap_in 1=", bitmap_in);
    putLabeledWord("lcd_frame_buffer 1=",lcd_frame_buffer );
    putLabeledWord("len =",len );
    putLabeledWord("LCD_FB_MAX =",LCD_FB_MAX() );        
#endif
    params = lcd_get_params(params_id, params);
    if (params == NULL) {
	putstr("lcd_display_bitmap: params is NULL\n\r");
	return;
    }
	
    if (len > LCD_FB_MAX() - pal_size){
#if 0
	putstr ("image too big, Downsizing...\r\n");
	putLabeledWord("len =", len);
	putLabeledWord("len avail=",LCD_FB_MAX() - pal_size );
#endif
	len = LCD_FB_MAX() - pal_size;
    }

    
    pxafb_disable_lcd_controller(params);

    /*
     * LCD's pixel format:
     *  for the 3600
     *  1111 11
     *  5432 1098 7654 3210
     *  rrrr 0ggg g00b bbb0
     *  our data is in the std 
     *	rrrr rggg gggb bbbb format
     *	3800 uses std format
     *
     *	3100 uses 2 pixels per byte, 4 bits per pixel
     *  
     * 
     * */

#if 0
    putLabeledWord("num_pixels2=", LCD_NUM_PIXELS());
    putLabeledWord("params_id_2=", params_id);
#endif

    if (params_id == LCDP_3900) {
	putstr("we are a 3900 display\r\n");
	// for the upside down 3600's we swap bytes
	for (num_pixels = LCD_NUM_PIXELS(); num_pixels; num_pixels--) {
	    pixel = *bmp;
	    *bmp++ = *bmp2;
	    *bmp2-- = (unsigned short) pixel;
	}
	for (num_pixels = LCD_NUM_PIXELS(); num_pixels; num_pixels--) {
	    pixel = *bmp++;
	    *bufp++ = pixel;
	}
    } else {
      for (num_pixels = LCD_NUM_PIXELS(); num_pixels; num_pixels--) {
	pixel = *bmp++;
	*bufp++ = pixel;
      }
    }

    pxafb_enable_lcd_controller(params);

}

void lcd_on(void)
{
  if (lcd_params)
    pxafb_enable_lcd_controller(lcd_params);
}

void
lcd_off(
    enum lcd_type params_id)
{
    if (!lcd_params)
	return;
    
    if (params_id == LCDP_3900) {
	clr_h3600_egpio(IPAQ_EGPIO_LCD_ON);
    }
    
    pxafb_disable_lcd_controller(lcd_params);
}

    
void
lcd_light(
	  int	on_off,
	  int	level)
{
#ifdef CONFIG_MACH_H3900
  if (machine_is_h3900()) {
    if (on_off)
      {
	H3900_ASIC3_GPIO_B_OUT |= GPIO3_FL_PWR_ON;
	H3800_ASIC2_PWM_0_TimeBase = 0;
	H3800_ASIC2_PWM_0_PeriodTime = 10;
	H3800_ASIC2_PWM_0_DutyTime = level;
	H3800_ASIC2_PWM_0_TimeBase = 0x17;
	H3800_ASIC2_CLOCK_Enable |= ASIC2_CLOCK_PWM | ASIC2_CLOCK_EX1;
      } else {
	H3900_ASIC3_GPIO_B_OUT &= ~GPIO3_FL_PWR_ON;
	H3800_ASIC2_PWM_0_TimeBase = 0;
	H3800_ASIC2_CLOCK_Enable &= ~(ASIC2_CLOCK_PWM | ASIC2_CLOCK_EX1);
      }
  }
#endif
}

void
lcd_fill(
    int	color,
    int	num)
{
    unsigned short* bufp = (unsigned short*)LCD_FB_IMAGE(lcd_frame_buffer,
							 cur_bpp);
    int	    num_pixels;


    num= (num==0)?(LCD_XRES*25):num;
    
    putLabeledWord("fb: 0x", (unsigned long)lcd_frame_buffer);
    putLabeledWord("bufp: 0x", (unsigned long)bufp);
    putLabeledWord("num: 0x", (unsigned long)num);
    putLabeledWord("col: ", (unsigned long)color);
    

    bufp += LCD_NUM_PIXELS()-1;
    //for (num_pixels = 0; num_pixels < (LCD_XRES * (25)) ; num_pixels++) {
    for (num_pixels = 0; num_pixels < num ; num_pixels++) {
	*bufp-- = color;
	
    }
    
    
    
}
    
unsigned int 
min( 
        unsigned int a, 
        unsigned int b) 
{ 
    return (a < b) ? a : b; 
}

unsigned int 
max( 
        unsigned int a, 
        unsigned int b) 
{ 
    return (a > b) ? a : b; 
}
    


char*
lcd_get_image_buffer(
    void)
{
    return (char*)LCD_FB_IMAGE(lcd_frame_buffer, cur_bpp);
}

size_t
lcd_get_image_buffer_len(
    void)
{
    return (LCD_FB_MAX());
}

void lcd_init(int mach_type)
{
     switch (mach_type){
     case MACH_TYPE_H3900:
	  lcd_type = LCDP_3900;
	  break;	    
     case MACH_TYPE_H5400:
	  return;
     default:
	  break;
     }
     lcd_params = params_list[lcd_type];
    
}

void set_pixel_value(unsigned short row, unsigned short col, unsigned short val)
{
     if (machine_is_h5400())
	  return;
    if (lcd_params->id == LCDP_3900){
	row = LCD_YRES - row;
	col = LCD_XRES - col;
	
	*(lcd_image_buffer+LCD_XRES*row*2+col*2) = val & 0x00ff;
	*(lcd_image_buffer+LCD_XRES*row*2+col*2+1) = (val & 0xff00) >> 8;
	
    }    
    else{
	*(lcd_image_buffer+LCD_XRES*row*2+col*2) = val & 0x00ff;
	*(lcd_image_buffer+LCD_XRES*row*2+col*2+1) = (val & 0xff00) >> 8;
	
    }
}

unsigned short get_pixel_value(unsigned short row, unsigned short col)
{
    unsigned short ret = 0x0;
    
    if (lcd_params->id == LCDP_3900){
	row = LCD_YRES - row;
	col = LCD_XRES - col;
	ret = *(lcd_image_buffer+LCD_XRES*row*2+col*2);
	ret |= *(lcd_image_buffer+LCD_XRES*row*2+col*2+1)<<8;
    }    
    else{
	ret = *(lcd_image_buffer+LCD_XRES*row*2+col*2);
	ret |= *(lcd_image_buffer+LCD_XRES*row*2+col*2+1)<<8;
	
    }

    return ret;
    

}

