#ifndef LCD_H_INCLUDED
#define LCD_H_INCLUDED

#if CONFIG_MACH_IPAQ
#include <asm-arm/arch-sa1100/sa1100-lcd.h>
#endif
#if CONFIG_MACH_H3900
#include "lcd-pxa.h"
#endif

#define LCD_VERTICAL 1
#define LCD_HORIZONTAL 2

#define LCD_BAR_START_ROW 28
#define LCD_BAR_START_COL 0
#define LCD_BAR_END_ROW 43
#define LCD_BAR_END_COL 320
#define LCD_BAR_DIRECTION -LCD_HORIZONTAL

typedef struct lcd_params_s
{
    int		    id;    
    unsigned long   lccr0;
    unsigned long   lccr1;
    unsigned long   lccr2;
    unsigned long   lccr3;
    unsigned long   gpio;
    unsigned long   egpio;
    unsigned long   gpdr;
    unsigned long   gafr;
    unsigned short* v_palette_base;

    int		    bits_per_pixel;
}
lcd_params_t;

#ifdef CONFIG_PXA
enum lcd_type
  {
    LCDP_3900 = 0,
    LCDP_CUSTOM,
    LCDP_end_sentinel
  };
#else
enum lcd_type
{
    LCDP_3600 = 0,
    LCDP_3100,
    LCDP_3800,
    LCDP_CUSTOM,
    LCDP_end_sentinel
};
#endif

extern enum lcd_type lcd_type;

extern void
lcd_default_init(
    int		    params_id,
    lcd_params_t*   params);

extern void
lcd_display_bitmap(
    char*	    bitmap,
    size_t	    len,
    int		    params_id,
    lcd_params_t*   params);

/* requires lcd_default_init or lcd_display_bitmap to have been called already */
extern void
lcd_on(
    void);

extern void
lcd_off(
    enum lcd_type params_id);

extern void
lcd_light(
    int	on_off,
    int	level);

extern void
setup_solid_palette(
    int	palval);

extern char*
lcd_get_image_buffer(
    void);

extern size_t
lcd_get_image_buffer_len(
    void);


extern void
lcd_bar(int color, int percent);

extern void
lcd_bar_clear();

extern void
led_blink(char onOff,
	  char totalTime,
	  char onTime,
	  char offTime);

extern void
lcd_progress_bar(
    unsigned int start_row, 
    unsigned int start_col, 
    unsigned int end_row, 
    unsigned int end_col,
	unsigned int percent,
	unsigned int direction,
    int color
    );

extern void
lcd_fill_region(
    unsigned int start_row, 
    unsigned int start_col, 
    unsigned int end_row, 
    unsigned int end_col,
    int	color
    );

extern void
lcd_clear_region(
    unsigned int start_row, 
    unsigned int start_col, 
    unsigned int end_row, 
    unsigned int end_col
    );

extern void
lcd_fill_region(
    unsigned int start_row, 
    unsigned int start_col, 
    unsigned int end_row, 
    unsigned int end_col,
    int	color
    );

extern void
lcd_clear_region(
    unsigned int start_row, 
    unsigned int start_col, 
    unsigned int end_row, 
    unsigned int end_col
    );

extern void
lcd_invert_region(
    unsigned short	start_row,
    unsigned short	start_col,
    unsigned short	end_row,
    unsigned short	end_col);

extern void
set_pixel_value(unsigned short row, unsigned short col, unsigned short val);


#if defined(CONFIG_LCD)
void command_lcd_test(int argc,const char* argv[]);
void command_lcd_on(int argc,const char* argv[]);
void command_lcd_off(int argc,const char* argv[]);
void command_lcd_light(int argc,const char* argv[]);
/*  void command_lcd_pal(int argc,const char* argv[]); */
void command_lcd_fill(int argc,const char* argv[]);
void command_lcd_bar(int argc,const char* argv[]);
void command_led_blink(int argc,const char* argv[]);
void command_lcd_image(int argc, const char* argv[]);
void command_lcd_zimage(int argc, const char* argv[]);
void command_ttmode(int argc, const char* argv[]);
void command_ser_con(int argc, const char* argv[]);
void command_irda_con(int argc, const char* argv[]);
void command_splash(int argc, const char* argv[]);
void command_lcd_progress_bar(int argc,const char* argv[]);
void command_lcd_fill_region(int argc,const char* argv[]);
void command_lcd_clear_region(int argc,const char* argv[]);
void command_lcd_invert_region(int argc,const char* argv[]);


#endif /* CONFIG_LCD */

void lcd_init(int mach_type);
#endif
