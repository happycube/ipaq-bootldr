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
#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"

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

void
lcd_clear_region(
    unsigned int start_row, 
    unsigned int start_col, 
    unsigned int end_row, 
    unsigned int end_col
    )
{
    if (machine_is_jornada56x()) return;
    lcd_fill_region(start_row, start_col, end_row, end_col,
		    0xffff);
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

void splash_zfile(char *filename, char *partition_name);

#if defined(CONFIG_LCD) && !defined (NO_SPLASH)
#include "splashz.h"

void
splash()
{
    int		    rc;
    unsigned long   uncomplen;
    char*	    fb = lcd_get_image_buffer();

    if (machine_is_jornada56x()) return;
#ifdef CONFIG_LOAD_KERNEL
    if (param_splash_filename.value) {
	splash_zfile((char *) param_splash_filename.value, (char *) param_splash_partition.value);
	return;
    }
#endif
    //putLabeledWord("splash: fb = 0x",fb);
    //putLabeledWord("splash: sizeof(img) = 0x",sizeof(splash_zimg));
    
    uncomplen = lcd_get_image_buffer_len ();

    putLabeledWord("splash: uncomplen: 0x", uncomplen);

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
