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
#if CONFIG_MACH_IPAQ
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
#endif
#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"
#include "aux_micro.h"

void set_pixel_value(unsigned short row, unsigned short col, unsigned short val);
static unsigned short get_pixel_value(unsigned short row, unsigned short col);

extern struct bootblk_param param_splash_filename;
extern struct bootblk_param param_splash_partition;



enum lcd_type lcd_type = LCDP_end_sentinel;
static lcd_params_t *lcd_params;
/* extra at end for possible DMA overrun */
// this includes the palette
unsigned long lcd_frame_buffer[LCD_FB_MAX() + 16];
// start of actual image
unsigned char *lcd_image_buffer;


/*
 *  sa1100fb_disable_lcd_controller():
 *    	Disables LCD controller by and enables LDD interrupt.
 * The controller_state is not changed until the LDD interrupt is
 * received to indicate the current
 * frame has completed.  Platform specific hardware disabling is also included.
 */
static void sa1100fb_disable_lcd_controller(lcd_params_t*   params)
{
    putstr("Disabling LCD controller\n\r");
    
    lcd_light(0, 0);
    
    LCSR = 0;	/* Clear LCD Status Register */
    LCCR0 &= ~(LCCR0_LDM);	/* Enable LCD Disable Done Interrupt */
    LCCR0 &= ~(LCCR0_LEN);	/* Disable LCD Controller */
    CTL_REG_WRITE(GPIO_BASE+GPIO_GPCR_OFF,params->gpio);
    clr_h3600_egpio(IPAQ_EGPIO_LCD_ON);    
}

unsigned short*	cur_pal;
int		cur_bpp = LCD_BPP;


/*
 *  sa1100fb_enable_lcd_controller():
 *    	Enables LCD controller.  If the controller is already enabled, it is first disabled.
 *      This forces all changes to the LCD controller registers to be done when the 
 *      controller is disabled.  Platform specific hardware enabling is also included.
 */
static void
sa1100fb_enable_lcd_controller(
    lcd_params_t*   params)
{
    sa1100fb_disable_lcd_controller(params);
    
    putstr("Enabling LCD controller\n\r"); drain_uart();

    /* make sure the GPIO pins are carrying LCD data */
    GPIO_GAFR_WRITE(params->gafr);/* alt function */
    GPIO_GPDR_WRITE(params->gpdr);/* outputs */
    
    /* Make sure the mode bits are present in the first palette entry */
    cur_pal = params->v_palette_base;
    cur_bpp = params->bits_per_pixel;
    lcd_image_buffer = (unsigned char *) (unsigned short*)LCD_FB_IMAGE(lcd_frame_buffer,
							   cur_bpp);

    cur_pal[0] = 0x2077;
    
    /* Sequence from 11.7.10 */
    LCCR3 = params->lccr3;
    LCCR2 = params->lccr2;
    LCCR1 = params->lccr1;
    LCCR0 = params->lccr0 & ~LCCR0_LEN;
    DBAR1 = (Address)vaddr_to_paddr((unsigned long)(params->v_palette_base));

    /* not used w/our panel */    
    //DBAR2 = (Address)vaddr_to_paddr((unsigned long)(params->v_palette_base+0x80));
    
    putstr("before EN\n\r"); drain_uart();

    LCCR0 |= LCCR0_LEN;
    putstr("after EN\n\r"); drain_uart();

    //set_egpio(EGPIO_IPAQ_LCD_ON | EGPIO_IPAQ_LCD_PCI |
    //EGPIO_IPAQ_LCD_5V_ON | EGPIO_IPAQ_LVDD_ON);
    set_h3600_egpio(IPAQ_EGPIO_LCD_ON);
    CTL_REG_WRITE(GPIO_BASE+GPIO_GPDR_OFF,params->gpdr);
    CTL_REG_WRITE(GPIO_BASE+GPIO_GPSR_OFF,params->gpio);
    
    putstr("after EGPIO \n\r"); drain_uart();
    putLabeledWord("DBAR1=", DBAR1);
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
    putstr("setup mono4 palette\n\r");
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
    
    /*  setup_spectral_palette(); */
    if (params_id == LCDP_3100) {
	putstr("setup mono4 palette\n\r");
	setup_mono4_palette();
    }

    putstr("pal done"); drain_uart();
    
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
    LCCR0_LEN + LCCR0_Color + LCCR0_Sngl + LCCR0_Act +
      LCCR0_LtlEnd + LCCR0_LDM + LCCR0_BAM + LCCR0_ERM + 
      LCCR0_DMADel(0),
    //LCCR1
    LCCR1_DisWdth( LCD_XRES ) +
      LCCR1_HorSnchWdth( 4 ) +
      LCCR1_BegLnDel( 0xC ) +
      LCCR1_EndLnDel( 0x11 ),
    //LCCR2
    LCCR2_DisHght( LCD_YRES + 1 ) +
      LCCR2_VrtSnchWdth( 3 )+
      LCCR2_BegFrmDel( 10 ) +
      LCCR2_EndFrmDel( 1 ),
    //LCCR3
    /* PCD */ 0x10
      | /* ACB */ 0
      | /* API */ 0
      | LCCR3_VrtSnchL
      | LCCR3_HorSnchL,
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
lcd_params_t lcd_mono_params = {
    //id
    1,
    //lccr0
    0x3,
    //lccr1
    //0x03036130,
    0x03036530,
    //lccr2
    0xa0ef,
    //lccr3
    0x28,
    //gpio
    1<<5,
    //egpio
    1<<6,
    //gpdr == gpio direction register
    0xff<<2,
    //gafr == gpio alternate funxion register
    //mono no use buitin lcd cntrl data pins
    0x0,
   
    (unsigned short*)lcd_frame_buffer,
    LCD_MONO_BPP
};

lcd_params_t lcd_3800_params = {
    2,
    //LCCR0
    LCCR0_LEN + LCCR0_Color + LCCR0_Sngl + LCCR0_Act +
      LCCR0_LtlEnd + LCCR0_LDM + LCCR0_BAM + LCCR0_ERM + 
      LCCR0_DMADel(0),
    //LCCR1
    LCCR1_DisWdth( LCD_XRES ) +
      LCCR1_HorSnchWdth( 4 ) +
      LCCR1_BegLnDel( 0xC ) +
      LCCR1_EndLnDel( 0x11 ),
    //LCCR2
    LCCR2_DisHght( LCD_YRES + 1 ) +
      LCCR2_VrtSnchWdth( 3 )+
      LCCR2_BegFrmDel( 10 ) +
      LCCR2_EndFrmDel( 1 ),
    //LCCR3
    /* PCD */ 0x10
      | /* ACB */ 0
      | /* API */ 0
      | LCCR3_VrtSnchL
      | LCCR3_HorSnchL,
    // 3800 doesnt use gpio for lcd on
    0,
    // 3800 doesnt use egpio for lcd
    0,
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
    &lcd_mono_params,		/* mono LCD panel */
    &lcd_3800_params		/* 3800 LCD panel */
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
	

    sa1100fb_enable_lcd_controller(params);
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
    unsigned long r,g,b;
    
    unsigned char* bufp_mono = (unsigned char*)LCD_FB_IMAGE(lcd_frame_buffer,
							 cur_bpp);
    unsigned short* bufp = (unsigned short*)LCD_FB_IMAGE(lcd_frame_buffer,
							 cur_bpp);
    unsigned short* bmp = (unsigned short*)bitmap_in;
    unsigned short* bmp2 = (unsigned short*)bitmap_in + len;

    
#if 1
    putLabeledWord("params_id 1=", params_id);
    putLabeledWord("cur_bpp 1=", cur_bpp);
    putLabeledWord("bufp 1=", bufp);
    putLabeledWord("bufp_mono 1=", (unsigned long) bufp_mono);    
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

    
    sa1100fb_disable_lcd_controller(params);

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
    if (params_id == LCDP_3100){
	//putstr("we are a mono display\r\n");
	// set up the mono palette
	// 1 0 * 3 2 * 5 4 * 7 6 * 9 8 * b a * d c * f e
	setup_mono4_palette();
	
	
	
	for (num_pixels = LCD_NUM_PIXELS(); num_pixels; num_pixels--) {
	    pixel = *bmp;
	    r = (pixel & 0xf800) >> 11;
	    g = (pixel & 0x07e0) >> 5;
	    b = pixel & 0x001f;
	    //pixel = (77*r+75*g+29*b)/512; // washed out
	    pixel = (77*r+75*g+29*b)/501; 
	    

#if 0
	    if (! (num_pixels % 100)){
		putLabeledWord("num=", num_pixels);
		putLabeledWord("pixel=", *bmp);
		putLabeledWord("red pre shift=", ((*bmp) & 0xf800));
		putLabeledWord("red post shift=", r);
		putLabeledWord("green=", g);
		putLabeledWord("blue=", b);
		putLabeledWord("scaled pixel=", pixel);
		
	    }
	    
#endif
	    if ((num_pixels % 2) == 0){
		*bufp_mono = (unsigned char) ((~pixel)&0xf);
	    }	    
	    else{		
		*bufp_mono |= (unsigned char) (((~pixel)&0xf)<<4);
		bufp_mono++;
		
	    }
	    bmp++;
	}
    }
    else if (params_id == LCDP_3600){
	//putstr("we are a 3600 display\r\n");
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
    }    
    else{
	//putstr("we are a 3800 display\r\n");
	for (num_pixels = LCD_NUM_PIXELS(); num_pixels; num_pixels--) {
	    pixel = *bmp++;
	    *bufp++ = pixel;
	}
    }
    sa1100fb_enable_lcd_controller(params);

}

void lcd_on(void)
{
  if (lcd_params)
    sa1100fb_enable_lcd_controller(lcd_params);
}

void
lcd_off(
    enum lcd_type params_id)
{
    if (!lcd_params)
	return;
    
if (machine_is_jornada56x()) return;
    if (params_id == LCDP_3600) {
	clr_h3600_egpio(IPAQ_EGPIO_LCD_ON);
    }
    
    sa1100fb_disable_lcd_controller(lcd_params);
}

    
void
lcd_light(
    int	on_off,
    int	level)
{
    char    level_buf[] = {0x02, 0x01, level};
    
    if (machine_is_jornada56x()) return;
    if (on_off)
	auxm_send_cmd(0x0d, 3, level_buf);
    else {
	level_buf[1] = 0;
	auxm_send_cmd(0x0d, 3, level_buf);
    }
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
    
    

    bufp += LCD_NUM_PIXELS()-1;
    //for (num_pixels = 0; num_pixels < (LCD_XRES * (25)) ; num_pixels++) {
    for (num_pixels = 0; num_pixels < num ; num_pixels++) {
	*bufp-- = color;
	
    }
    
    
    
}
    
void
lcd_bar(int color,
    int	percent)
{
    lcd_progress_bar(LCD_BAR_START_ROW,
		     LCD_BAR_START_COL,
		     LCD_BAR_END_ROW,
		     LCD_BAR_END_COL,
		     percent,
		     LCD_BAR_DIRECTION,
		     color); 
}

void
lcd_bar_clear()
{
    lcd_clear_region(LCD_BAR_START_ROW,
		     LCD_BAR_START_COL,
		     LCD_BAR_END_ROW,
		     LCD_BAR_END_COL);
}

void
lcd_progress_bar(
				 unsigned int start_row,
				 unsigned int start_col,
				 unsigned int end_row,
				 unsigned int end_col,
				 unsigned int percent,
				 unsigned int direction,
				 int color
				 )
{
  switch (direction) {
  case LCD_VERTICAL:
    if (end_row > start_row)
      lcd_fill_region(start_row + ((100 - percent) * (end_row - start_row)) / 100, start_col,
		      end_row, end_col, color);
    else
      lcd_fill_region(end_row + ((100 - percent) * (start_row - end_row)) / 100, start_col,
		      start_row, end_col, color);
    break;
  case -LCD_VERTICAL:
    if (end_row > start_row)
      lcd_fill_region(start_row, start_col,
		      start_row + ((100 - percent) * (end_row - start_row)) / 100, end_col, color);
    else
      lcd_fill_region(end_row, start_col,
		      end_row + ((100 - percent) * (start_row - end_row)) / 100, end_col, color);
  case LCD_HORIZONTAL:
    if (end_col > start_col)
      lcd_fill_region(start_row, start_col,
		      end_row, start_col + (percent * (end_col - start_col)) / 100, color);
    else
      lcd_fill_region(start_row, end_col,
		      end_row, end_col + (percent * (start_col - end_col)) / 100, color);
    break;
  case -LCD_HORIZONTAL:
    if (end_col > start_col)
      lcd_fill_region(start_row, start_col + ((100 - percent) * (end_col - start_col)) / 100,
		      end_row, end_col, color);
    else
      lcd_fill_region(start_row, end_col +  ((100 - percent) * (start_col - end_col)) / 100,
		      end_row, start_col, color);
    break;
    
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
    
void
lcd_clear_region(
    unsigned int start_row, 
    unsigned int start_col, 
    unsigned int end_row, 
    unsigned int end_col
    )
{
    if (machine_is_jornada56x()) return;
    if (!lcd_params)
	return;
    lcd_fill_region(start_row, start_col, end_row, end_col,
            (lcd_params->id == LCDP_3100) ? 0 : 0xffff);
}

void
lcd_fill_region(
    unsigned int start_row, 
    unsigned int start_col, 
    unsigned int end_row, 
    unsigned int end_col,
    int	color
    )
{
    unsigned int row, col;
if (machine_is_jornada56x()) return;
    for (col = min(start_col, end_col); col < max(start_col, end_col); col++)
        for (row = min(start_row, end_row); row < max(start_row, end_row); row++)
    		set_pixel_value(row, col, color);
}

void
lcd_invert_region(
    unsigned short	start_row,
    unsigned short	start_col,
    unsigned short	end_row,
    unsigned short	end_col)

{
    unsigned short row,col,p;


if (machine_is_jornada56x()) return;
    for (row=start_row; row < (end_row); row++)
	for (col=start_col; col < end_col; col++){
	    p = get_pixel_value(row, col);
	    set_pixel_value(row, col, ~p);
	}
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

void
led_blink(char onOff,char totalTime,char onTime,char offTime)
{
    char ledData[] = {onOff,totalTime,onTime,offTime};

    if (machine_is_jornada56x()) return;
    auxm_send_cmd(0x08, 4, ledData);
}

void splash_zfile(char *filename, char *partition_name);

#if defined(CONFIG_LCD) && !defined (NO_SPLASH)
#include "splashz.h"

void
splash()
{
    int		    rc;
    unsigned long   uncomplen;
    char*	    fb = lcd_get_image_buffer();

    putstr("splash()\r\n");
    if (machine_is_jornada56x()) return;
#ifdef CONFIG_LOAD_KERNEL
    if (param_splash_filename.value) {
	splash_zfile((char *) param_splash_filename.value, (char *) param_splash_partition.value);
	return;
    }
#endif
    //putLabeledWord("splash: fb = 0x",fb);
    //putLabeledWord("splash: sizeof(img) = 0x",sizeof(splash_zimg));
    
    /*  lcd_default_init(); */
    rc = uncompress(fb, &uncomplen, splash_zimg, sizeof(splash_zimg));
    if (rc != Z_OK) {
	putLabeledWord("splash: uncompress failed, rc: 0x", rc);
	//putLabeledWord("splash: uncompress failed, -5: 0x", -5);
	putLabeledWord("splash: uncompress failed, uncomplen: 0x", uncomplen);
	return;
    }
    //putLabeledWord("splash: uncompress succeeded, uncomplen: 0x", uncomplen);
    lcd_display_bitmap(fb, uncomplen, lcd_type, NULL);
}
#else
void
splash(
    void)
{
    putstr("splash code not compiled in.\n\r");
}
#endif


#if defined(CONFIG_LCD)

void
command_lcd_test(
    int	    argc,
    const char*   argv[])
{
    int		    params_id;
    lcd_params_t*   params = NULL;
    
    if (argc > 1) {
	params_id = strtoul(argv[1], NULL, 0);
	if (params_id == LCDP_CUSTOM && argc > 2) {
	    params = (lcd_params_t*)strtoul(argv[2], NULL, 0);
	}
    }
    else {
	params_id = lcd_type;
	params = NULL;
    }
    
    lcd_default_init(params_id, params);

}

COMMAND(lcdon, command_lcd_on, "-- turn lcd on", BB_RUN_FROM_RAM);
void
command_lcd_on(
    int	    argc,
    const char*   argv[])
{
    lcd_on();
}

COMMAND(lcdoff, command_lcd_off, "-- turn lcd off", BB_RUN_FROM_RAM);
void
command_lcd_off(
    int	    argc,
    const char*   argv[])
{
    lcd_off(lcd_type);
}

COMMAND(lcdlight, command_lcd_light, "[level=0] -- adjust lcd backlight", BB_RUN_FROM_RAM);
void
command_lcd_light(
    int	    argc,
    const char*   argv[])
{
    if (argc == 1)
	lcd_off(lcd_type);
    else {
	int level = strtoul(argv[1], NULL, 0);
	lcd_light(1, level);
    }
}

#if 0
COMMAND(lcdpal, command_lcd_pal, "<palval> -- set lcd pal", BB_RUN_FROM_RAM);
void
command_lcd_pal(
    int	    argc,
    const char*   argv[])
{
    int	 palval;

    if (argc > 1) {
	palval = strtoul(argv[1], NULL, 0);
	setup_solid_palette(palval);
    }
    else
	putstr("Need arg: palval\n\r");
}
#endif

COMMAND(lcdfill, command_lcd_fill, "<color> [inc] -- fill lcd display", BB_RUN_FROM_RAM);
void
command_lcd_fill(
    int	    argc,
    const char*   argv[])
{
    int	color;
    int	inc;

    if (argc > 1) {
	color = strtoul(argv[1], NULL, 0);
	if (argc > 2)
	    inc = strtoul(argv[2], NULL, 0);
	else
	    inc = 0;
	lcd_fill(color, inc);
    }
    else
	putstr("Need args: color [inc]\n\r");
}

COMMAND(lcdbar, command_lcd_bar, "<color> [percent] -- fill lcd bar", BB_RUN_FROM_RAM);
void
command_lcd_bar(
    int	    argc,
    const char*   argv[])
{
    int	color;
    int	percent = 100;

    if (argc > 1) {
      color = strtoul(argv[1], NULL, 0);
      if (argc > 2)
	percent = strtoul(argv[2], NULL, 0);
      lcd_bar(color, percent);
    }
    else
	putstr("Need args: color [percent]\n\r");

}

void
command_lcd_progress_bar(
    int	    argc,
    const char*   argv[])
{
  int start_row;
  int end_row;
  int start_col;
  int end_col;
  int color;
  int percent;
  int direction;
  
  if (argc < 8) {
	putstr("Need args: start_row start_col end_row end_col percent direction color\n\r");
	putstr("percent: 1 .. 100, direction: 0 - horizontal, 1 - vertical\n\r");
	return;
  } 
  
  start_row = strtoul(argv[1], NULL, 0);
  start_col = strtoul(argv[2], NULL, 0);
  end_row = strtoul(argv[3], NULL, 0);
  end_col = strtoul(argv[4], NULL, 0);
  percent = strtoul(argv[5], NULL, 0);
  direction = strtoul(argv[6], NULL, 0);
  color = strtoul(argv[7], NULL, 0);
  lcd_progress_bar(start_row, start_col, end_row, end_col, percent, direction, color);
}

void
command_lcd_fill_region(
    int	    argc,
    const char*   argv[])
{
    int	start_row;
    int	end_row;
    int	start_col;
    int	end_col;
    int color;

    if (argc < 6)
	putstr("Need args: start_row start_col end_row end_col color\n\r");

    start_row = strtoul(argv[1], NULL, 0);
    start_col = strtoul(argv[2], NULL, 0);
    end_row = strtoul(argv[3], NULL, 0);
    end_col = strtoul(argv[4], NULL, 0);
    color = strtoul(argv[5], NULL, 0);
    lcd_fill_region(start_row,start_col,end_row,end_col,color);
}

void
command_lcd_clear_region(
    int	    argc,
    const char*   argv[])
{
    int	start_row;
    int	end_row;
    int	start_col;
    int	end_col;

    if (argc < 5)
	putstr("Need args: start_row start_col end_row end_col\n\r");

    start_row = strtoul(argv[1], NULL, 0);
    start_col = strtoul(argv[2], NULL, 0);
    end_row = strtoul(argv[3], NULL, 0);
    end_col = strtoul(argv[4], NULL, 0);
    lcd_clear_region(start_row,start_col,end_row,end_col);
}

COMMAND(lcdinvertregion, command_lcd_invert_region, "<start_row> <start_col> <end_row> <end_col>", BB_RUN_FROM_RAM);
void
command_lcd_invert_region(
    int	    argc,
    const char*   argv[])
{
    int	start_row;
    int	end_row;
    int	start_col;
    int	end_col;

    if (argc < 5)
	putstr("Need args: start_row start_col end_row end_col\n\r");

    start_row = strtoul(argv[1], NULL, 0);
    start_col = strtoul(argv[2], NULL, 0);
    end_row = strtoul(argv[3], NULL, 0);
    end_col = strtoul(argv[4], NULL, 0);
    lcd_invert_region(start_row,start_col,end_row,end_col);
    

}

#if 0
COMMAND(lcdimg, command_lcd_image, "-- display image", BB_RUN_FROM_RAM);
void
command_lcd_image(
    int	    argc,
    const char*   argv[])
{
    char*   fb = lcd_get_image_buffer();
    dword   img_size;
    
    img_size = modem_receive(fb, LCD_NUM_DISPLAY_BYTES(LCD_BPP));

    lcd_display_bitmap(fb, img_size, lcd_type, NULL);
}
#endif


char	zbuf[17465];
/* Displays the image stored in zbuf */
void lcd_display_zimg(dword img_size) {
  char*   fb = lcd_get_image_buffer();
  uLongf  uncomplen = lcd_get_image_buffer_len();
  int rc;
#if 0
  rc = uncompress(fb, &uncomplen, zbuf, img_size);
  putLabeledWord("uncompress returned, rc: 0x", rc);
  if (rc != Z_OK) {
	putLabeledWord("uncompress failed, rc: 0x", rc);
	return;
  }
#else
  {
	/*
	 * do it piecemeal to test out using zlib stream functions...
	 */
	z_stream stream;
	int	err;
	int	err2;
	char	tiny_buf[128];
	char*	fbp = fb;
	size_t tlen;
	
	stream.next_in = (Bytef*)zbuf;
	stream.avail_in = (uInt)img_size;
	
	stream.next_out = tiny_buf;
	stream.avail_out = (uInt)sizeof(tiny_buf);
	
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	
	err = inflateInit(&stream);
	if (err != Z_OK) {
	  putLabeledWord("inflateInit failed", err);
	  return;
	}
	
	while ((err = inflate(&stream, Z_SYNC_FLUSH)) == Z_OK) {
	  tlen = sizeof(tiny_buf) - stream.avail_out;
	  
	  if (tlen) {
		memcpy(fbp, tiny_buf, tlen);
		fbp += tlen;
		putLabeledWord("memcpy 0x", tlen); 
	  }
	  
	  if (stream.avail_out == 0) {
		putstr("reset obuf\n\r"); 
		stream.next_out = tiny_buf;
		stream.avail_out = (uInt)sizeof(tiny_buf);
	  }
	}
	
	
	if (err == Z_STREAM_END) {
	  tlen = sizeof(tiny_buf) - stream.avail_out;
	  if (tlen)
		memcpy(fbp, tiny_buf, tlen);
	}
	
	err2 = inflateEnd(&stream);
	
	if (err != Z_STREAM_END) {
	  putLabeledWord("inflate failed", err);
	  return;
	}
	
	if (err2 != Z_OK) {
	  putLabeledWord("inflateEnd failed", err2);
	  return;
	}
	
	uncomplen = stream.total_out;
  }
  
#endif
  
  putstr("about to display\n\r");

#if 1
#ifdef CONFIG_LCD
  lcd_display_bitmap(fb, uncomplen, lcd_type, NULL);
#endif
#else
  putstr("skipping pixel conversion");
#endif    
}

/** Warning: This uses zbuf, which is just large enough for the stock bootloader image. Make sure your images
 * are smaller, or you could corrupt memory. */
COMMAND(lcdzimg, command_lcd_zimage, "<len> -- display gzipped image", BB_RUN_FROM_RAM);
void lcd_display_zimg(dword img_size);
void
command_lcd_zimage(
    int	    argc,
    const char*   argv[])
{
    dword   img_size;
    int	    rc;
 
    if (argc < 2) {
	putstr("need args: len\n\r");
	return;
    }
    
    img_size = strtoul(argv[1], NULL, 0);
    if (img_size > sizeof(zbuf)) {
	putLabeledWord("img too big, zbuf size: ", sizeof(zbuf));
	return;
    }
	
    rc = modem_receive(zbuf, img_size);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    putLabeledWord("modem_rx returned, rc: 0x", rc);
    if (rc == 0) {
	  putstr("download failed.  Aborting\n\r");
	  return;
    }
	lcd_display_zimg(img_size);
}

#if defined(CONFIG_LCD) && defined(CONFIG_LOAD_KERNEL)
COMMAND(lcdzfile, command_splash_zfile, "[filename] [partition] -- displays a gzipped PNM", BB_RUN_FROM_RAM);
void command_splash_zfile(int argc, const char **argv) {
  if (argc > 2)
	splash_zfile(argv[1], argv[2]);
  else
	splash_zfile((char *) param_splash_filename.value, (char *) param_splash_partition.value);
}

#include "lkernel.h"
void splash_zfile(char *filename, char *partition_name) {
  unsigned long size;
  char *part_name = partition_name;
  struct FlashRegion *partition;
  struct part_info part;

  if (!filename) filename = (char *) param_splash_filename.value;
  if (!filename) filename = "/boot/splashz_linux";
  if (!part_name) part_name = (char *) param_splash_partition.value;
  if (!part_name) part_name = "root";
  
  partition = btflash_get_partition(part_name);
  part.size = partition->size;
  /* for uniformly sized flash sectors */
  part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
  part.offset = ((char*)flashword) + partition->base;

  if ((size = jffs2_1pass_load(zbuf,&part,filename)) == 0){
	putstr("bad splash load for file: ");
	putstr(filename);
	putstr("\r\n");
	return;
  }

  lcd_display_zimg(size);
}
#endif /* defined(CONFIG_LOAD_KERNEL) && defined(CONFIG_LCD) */

#endif  /* defined(CONFIG_LCD) */


#ifdef CONFIG_LCD
COMMAND(splash, command_splash, "-- display the splash screen (also see lcdzfile)", BB_RUN_FROM_RAM);
void
command_splash(
    int		argc,
    const char* argv[])
{
	splash();
}
#endif

void lcd_init(int mach_type)
{
#ifdef CONFIG_LCD
    switch (mach_type){
	case MACH_TYPE_H3100:
	    lcd_type = LCDP_3100;	
	    break;	    
	case MACH_TYPE_H3600:
	    lcd_type = LCDP_3600;
	    break;
	case MACH_TYPE_H3800:
	    *((unsigned short *) H3800_ASIC1_GPIO_MASK_ADDR) = H3800_ASIC1_GPIO_MASK_INIT;
	    *((unsigned short *) H3800_ASIC1_GPIO_OUT_ADDR) = H3800_ASIC1_GPIO_OUT_INIT;
	    *((unsigned short *) H3800_ASIC1_GPIO_DIR_ADDR) = H3800_ASIC1_GPIO_DIR_INIT;
	    *((unsigned short *) H3800_ASIC1_GPIO_OUT_ADDR) = H3800_ASIC1_GPIO_OUT_INIT;
	    lcd_type = LCDP_3800;
	    break;	    
	default:
	    break;
    }
    lcd_params = params_list[lcd_type];
    
#endif
}

void set_pixel_value(unsigned short row, unsigned short col, unsigned short val)
{
    
    
    if (lcd_params->id == LCDP_3100){
	unsigned char *p = (lcd_image_buffer+LCD_XRES*row/2+col/2);
	unsigned char pv = *p;
	
	if ((col%2) == 1)
	    *p = (pv & 0x0f)| ((val & 0xf)<<4);
	else
	    *p = (pv & 0xf0)| ((val & 0xf));
    }
    else if (lcd_params->id == LCDP_3600){
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
static unsigned short get_pixel_value(unsigned short row, unsigned short col)
{
    unsigned short ret = 0x0;
    
    if (lcd_params->id == LCDP_3100){
	unsigned char *p = (lcd_image_buffer+LCD_XRES*row/2+col/2);
	unsigned char pv = *p;
	
	if ((col%2) == 1)
	    ret = (pv & 0xf0)>>4;      
	else
	    ret = (pv & 0x0f);      
    }
    else if (lcd_params->id == LCDP_3600){
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


