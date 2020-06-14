/*
 * Embedded reflash support for iPAQ bootloader
 *
 * Copyright (C) 2002, 2003 Phil Blundell
 *
 * Copying or modifying this code for any purpose is permitted,
 * provided that this copyright notice is preserved in its entirety
 * in all copies or modifications.
 */

#include "bootldr.h"
#include "buttons.h"
#include "commands.h"
#include "serial.h"
#include "font_8x8.c"
#include "lcd.h"
#include "hal.h"
#include "h3600_sleeve.h"
#include "pcmcia.h"
#include "vfat.h"
#include "ide.h"
#include "heap.h"
#include "params.h"
#include "md5.h"
#include "fs.h"
#include "aux_micro.h"
#include "zlib.h"
#include "zUtils.h"
#include "crc.h"

#include <string.h>

#ifndef CONFIG_IDE
#error IDE support required!
#endif

#ifndef CONFIG_VFAT
#error VFAT support required!
#endif

int reflash_enabled = 0;
int direction = 0;

struct reflash_entry
{
  char *pretty_name;
  char *image_name;
  char *md5sum_name;
  char *target;
};

void
draw_text (char *s, int r, int c, int color)
{
  while (*s)
    {
      unsigned const char *d = &fontdata_8x8[*s << 3];
      unsigned int x, y;
      
      for (y = 0; y < 8; y++)
	{
	  unsigned char v = d[y];
      
	  for (x = 0; x < 8; x++)
	    {
	      int val = (v & (1 << x)) ? color : 0xffff;
	      set_pixel_value (LCD_YRES - (r + y), LCD_XRES - (c + (7 - x)), val);
	    }
	}

      c += 8;
      s++;
    }
}

void
draw_rectangle (int r1, int c1, int r2, int c2, int col)
{
  int r, c;
  for (r = r2; r <= r1; r++)
    {
      set_pixel_value (r, c1, col);
      set_pixel_value (r, c2, col);
    }
  for (c = c2; c <= c1; c++)
    {
      set_pixel_value (r1, c, col);
      set_pixel_value (r2, c, col);
    }
}

int
await_key (void)
{
  int r;

#ifdef CONFIG_MACH_IPAQ
  if (machine_has_auxm ())
    auxm_serial_check ();
#endif

  set_last_buttonpress (0);
  
  while (r = get_last_buttonpress(), r == 0)
    {
      if (param_mach_type.value == MACH_TYPE_H3800 || param_mach_type.value == MACH_TYPE_H3900){
	check_3800_func_buttons();	    
      }
      else {
#ifdef CONFIG_MACH_IPAQ
	if (machine_has_auxm ())
	  auxm_serial_check ();
#endif
      }	
    }

  return r;
}

#define MAX_IMAGES 8
#define MAX_LEN 25

void
command_reflash (int argc, const char **argv)
{
  unsigned int detect = 0;
  char *buf = (char *)VKERNEL_BASE + 1024; /* just a temporary holding area */
  int nbytes, r;
  size_t compressed_size;
  unsigned int nr, i;
  struct reflash_entry flash[MAX_IMAGES], *fp;
  int sel;
  unsigned int calc_md5[MD5_SUM_WORDS], stored_md5[MD5_SUM_WORDS];
  char *error = NULL;
  static char image_filename_store[256];
  static char target_store[64];
  static char error_message_store[256];
  int do_md5, is_gzip;
  struct bz_stream z;
  unsigned int verify_crc = 0;
  int err;
  const char *current_model = NULL;

#ifdef CONFIG_MACH_IPAQ
  if (machine_is_h3100 ())
    current_model = "h3100";
  else if (machine_is_h3600 () || machine_is_h3800 ())
    current_model = "h3600";
#endif
#ifdef CONFIG_MACH_H3900
  if (machine_is_h3900 ())
    current_model = "h3900";
#endif

  if (!reflash_enabled)
    {
      putstr ("reflashing not enabled.  Reboot with action button depressed.\r\n");
      return;
    }

  lcd_clear_region (LCD_YRES - 112, LCD_XRES - 108, LCD_YRES - 240, LCD_XRES - 320);

  lcd_clear_region (208,0,235,318);
  lcd_clear_region (174,241,207,318);
  lcd_clear_region (44,188,64,318);
  lcd_clear_region (5,213,25,318);

  draw_text ("Embedded Reflash Utility", 116, 118, 0);

  draw_text ("Scanning for images...", 116 + 16, 110, 0x001f);

#ifdef CONFIG_H3600_SLEEVE
  h3600_sleeve_insert();
#endif
  delay_seconds(1);
  pcmcia_detect( (u8*)&detect);

  set_exec_buttons_automatically (0);

  if (!detect) 
    {
      draw_text ("No PCMCIA card found", 116 + 16, 110, 0xf800);
      goto loop;
    }

  pcmcia_insert ();
  ide_read_ptable (NULL);

  if (r = vfat_mount_partition (0), r != 0)
    {
      putLabeledWord("vfat_mount_partition returned ", r);
      draw_text ("VFAT mount failed", 116 + 16, 110, 0xf800);
      goto loop;
    }

  nbytes = vfat_read_file (buf, "reflash.ctl", 0);
  if (nbytes < 0)
    {
      draw_text ("Can't read control file", 116 + 16, 110, 0xf800);
      goto loop;
    }
  
  lcd_clear_region (LCD_YRES - (116 + 16), LCD_XRES - 110, 0, 0);

  draw_text ("Select image file to load:", 116 + 16, 110, 0);
  draw_rectangle (LCD_YRES - (116 + 28), LCD_XRES - 106, 12, 4, 0);

  {
    char *p = buf;
    int last = 0;

    nr = 0;

    while (!last)
      {
	char *pretty_name, *image_filename, *md5_filename, *target;
	char *model = NULL;

	pretty_name = p;

	while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '\0')
	  p++;
	if (*p == '\n' || *p == '\r')
	  {
	    p++;
	    continue;
	  }
	else if (*p == '\0')
	  break;
	*p++ = 0;
	while (*p == ' ')
	  p++;

	image_filename = p;
	
	while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '\0')
	  p++;
	if (*p == '\n')
	  {
	    p++;
	    continue;
	  }
	else if (*p == '\0')
	  break;
	*p++ = 0;
	while (*p == ' ' || *p == '\t')
	  p++;

	md5_filename = p;

	while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '\0')
	  p++;
	if (*p == '\n')
	  {
	    p++;
	    continue;
	  }
	else if (*p == '\0')
	  break;

	*p++ = 0;
	while (*p == ' ' || *p == '\t')
	  p++;

	target = p;

	while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '\0')
	  p++;

	if (*p == ' ' || *p == '\t')
	  {
	    *p++ = 0;
	    while (*p == ' ' || *p == '\t')
	      p++;
	    
	    if (*p != '\0' && *p != '\r' && *p != '\n')
	      {
		model = p;

		while (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' && *p != '\0')
		  p++;
	      }
	  }

	if (*p == 0)
	  last = 1;

	*p++ = 0;

	if (current_model && model && strcasecmp (current_model, model))
	  continue;

	if (nr < MAX_IMAGES)
	  {
	    flash[nr].pretty_name = pretty_name;
	    flash[nr].image_name = image_filename;
	    flash[nr].md5sum_name = md5_filename;
	    flash[nr].target = target;
	    putLabeledWord ("image ", nr);
	    putstr ("name ");
	    putstr (flash[nr].pretty_name);
	    putstr ("; image ");
	    putstr (flash[nr].image_name);
	    putstr ("; md5sum ");
	    putstr (flash[nr].md5sum_name);
	    putstr ("; target ");
	    putstr (flash[nr].target);
	    putstr ("\r\n");
	    nr++;
	  }
      }
  }

  putLabeledWord ("total images ", nr);
    
  for (i = 0; i < nr; i++)
    {
      char buf[MAX_LEN];
      strncpy (buf, flash[i].pretty_name, MAX_LEN - 2);
      buf[MAX_LEN - 1] = 0;
      draw_text (buf, 116 + 32 + (i * 8), 110, 0);
    }
  
  sel = 0;
  for (;;)
    {
      int k;

      lcd_invert_region (LCD_YRES - (116 + 32 + (sel * 8) + 7), LCD_XRES - (110 + 8 * MAX_LEN),
			 LCD_YRES - (116 + 31 + (sel * 8)), LCD_XRES - 110);      
      k = await_key();
      if (k == 10)
	break;
      
      lcd_invert_region (LCD_YRES - (116 + 32 + (sel * 8) + 7), LCD_XRES - (110 + 8 * MAX_LEN),
			 LCD_YRES - (116 + 31 + (sel * 8)), LCD_XRES - 110);      
      putLabeledWord("button k=", k);
      if (k == 8 && sel > 0)
	sel--;
      else if (k == 7 && (sel + 1) < nr)
	sel++;
      else if (k == 2 /* calendar button */) {
	if (direction == 0) {
	  direction++;
	  draw_text ("Select image file to save:", 116 + 16, 110, 0);
	} else {
	  draw_text ("Select image file to load:", 116 + 16, 110, 0);
	  direction=0;
	} 
      }
      msleep(100);
    }

  lcd_clear_region (LCD_YRES - (116 + 15), LCD_XRES - 105, 0, 0);

  fp = &flash[sel];

  putstr ("selected image is ");
  putstr (fp->pretty_name);
  putstr ("\n");

  strcpy (image_filename_store, fp->image_name);
  strcpy (target_store, fp->target);

  if (direction != 0) {
    int copyargc = 3;
    char copyargv[3][MAX_LEN];
    draw_text ("Saving file...", 116 + 16, 110, 0x001f); 
    
    putstr ("selected image is ");
    putstr (fp->pretty_name);
    putstr ("\n");
    
    strcpy (image_filename_store, fp->image_name);
    strcpy (copyargv[0],"copy");
    strcpy (copyargv[1], "root");
    strcpy (copyargv[2], fp->target);
    copyargv[3][0]=0;

    do_command(copyargc,copyargv);
    draw_text ("No feedback yet.", 116 + 16, 110, 0xf800);
    goto loop;
  }

  draw_text ("Loading file...", 116 + 16, 110, 0x001f);
  do_md5 = strcmp (fp->md5sum_name, "-") ? 1 : 0;

  if (do_md5) 
    {
      char *lp = buf;

      /* suck in the md5sum */
      nbytes = vfat_read_file (buf, fp->md5sum_name, 0);
      if (nbytes < 0)
	{
	  draw_text ("Cannot read md5sum from", 116 + 16, 110, 0xf800);
	  draw_text (fp->md5sum_name, 116 + 24, 110, 0xf800);
	  goto loop;
	}
      
      buf[nbytes] = 0;
      
      /* convert md5sum to binary form */
      for (;;)
	{
	  int nibble = 0, bit = 0, offset;
	  char *p;
	
	  for (offset = 0; offset < MD5_SUM_WORDS; offset++)
	    stored_md5[offset] = 0;
	
	  for (offset = 0; offset < 32; offset++)
	    {
	      char c = lp[offset];
	      int n;
	      if (c >= '0' && c <= '9')
		n = c - '0';
	      else if (c >= 'a' && c <= 'f')
		n = c - 'a' + 10;
	      else if (c >= 'A' && c <= 'F')
		n = c - 'A' + 10;
	      else
		{
		  error = "md5sum file is corrupt";
		  goto print_error;
		}
	      
	      stored_md5[bit / 32] |= (n << ((bit % 32) + (nibble ? 0 : 4)));
	      
	      if (nibble)
		bit += 8;
	      
	      nibble ^= 1;
	    }
	  
	  lp += offset;
	  
	  if (lp[0] != ' '
	      || lp[1] != ' ')
	    {
	      error = "md5sum file is corrupt";
	      goto print_error;
	    }
	  
	  lp += 2;

	  p = lp;
	  while (*p != '\r' && *p != '\n' && *p)
	    p++;
	  *(p++) = 0;

	  putstr ("have md5sum for ");
	  putstr (lp);
	  putstr ("\r\n");

	  if (strcmp (lp, image_filename_store) == 0)
	    break;

	  lp = p;
	  while (isspace (*lp))
	    lp++;
	  if (*lp == 0)
	    {
	      error = "md5sum not found in file";
	      goto print_error;
	    }
	}
    }

  /* suck in the image */
  nbytes = vfat_read_file (buf, image_filename_store, 0);
  if (nbytes < 0)
    {
      draw_text ("Cannot read image from", 116 + 16, 110, 0xf800);
      draw_text (image_filename_store, 116 + 24, 110, 0xf800);
      goto loop;
    }

  if (do_md5)
    {
      draw_text ("Checking md5sum...", 116 + 16, 110, 0x001f);

      md5_sum(buf, nbytes, calc_md5);
      
      for (i = 0; i < MD5_SUM_WORDS; i++)
	if (stored_md5[i] != calc_md5[i])
	  {
	    error = "Image file is corrupt";
	    goto print_error;
	  }
    }
  
  is_gzip = isGZipRegion ((unsigned long)buf);
  if (strcmp(target_store, "bootldr") == 0) {
       draw_text ("Checking bootldr validity", 116 + 16, 110, 0x001f);
       if (is_gzip) {
	    size_t sz = nbytes;
	    if (!gUnZip((unsigned long)buf, &sz, (unsigned long)buf + SZ_256K)){
		 error = "Failed to gunzip bootldr image";
		 goto print_error;
	    }
	    /* now point to the unzipped image */
	    buf += SZ_256K;
	    nbytes = sz;
	    is_gzip = 0;
       }
       if (!isValidBootloader((unsigned long)buf, nbytes)){
	    error = "Invalid bootldr into flash";
	    goto print_error;
       }
  }

  if (is_gzip)
    {
      size_t sz = nbytes;

      draw_text ("Checking data integrity", 116 + 16, 110, 0x001f);
      
      if (! verifyGZipImage ((unsigned long)buf, &sz))
	{
	  error = "Error in compressed data";
	  goto print_error;
	}
      
      // sz now has the uncompressed size in it.
      gzInitStream ((unsigned long)buf, nbytes, &z);

      compressed_size = nbytes;
      nbytes = sz;
    }

  lcd_clear_region (LCD_YRES - (116 + 15), LCD_XRES - 105, 0, 0);

  draw_text (image_filename_store, 116 + 16, 110, 0x001f);
  draw_text ("appears to be intact.", 116 + 24, 110, 0x001f);

  draw_text ("Press RECORD to flash", 116 + 48, 110, 0x001f);
  draw_text ("or any other key to reset", 116 + 56, 110, 0x001f);

  r = await_key();
  if (r != 1)
    bootldr_reset ();

  lcd_clear_region (LCD_YRES - (116 + 15), LCD_XRES - 105, 0, 0);

  {
    struct iohandle *ioh = open_iohandle (target_store);
    if (!ioh)
      {
	strcpy(error_message_store, "Could not open ");
	strcat(error_message_store, target_store);
	error = error_message_store;
	goto print_error;
      }

    if (ioh->ops->prewrite)
      {
	size_t size = ioh->ops->len (ioh);
	unsigned int i = 0;

	draw_text ("Erasing...", 116 + 16, 110, 0x001f);
	draw_rectangle (LCD_YRES - (116 + 33),
			LCD_XRES - 110, 
			LCD_YRES - (116 + 49),
			LCD_XRES - 311,
			0);

	while (i < size)
	  {
	    int l = size - i;

	    if (l > 256 * 1024)
	      l = 256 * 1024;

	    if (ioh->ops->prewrite (ioh, i, l) < 0)
	      {
		error = "Erase failed";
		goto print_error;
	      }

	    i += l;

	    lcd_progress_bar (LCD_YRES - (116 + 48),
			      LCD_XRES - 310,
			      LCD_YRES - (116 + 32),
			      LCD_XRES - 110,
			      (i * 100) / size,
			      -LCD_HORIZONTAL,
			      0xf800);
	  }
      }

    lcd_clear_region (LCD_YRES - (116 + 15), LCD_XRES - 105, 0, 0);
    draw_text ("Programming...", 116 + 16, 110, 0x001f);
    draw_rectangle (LCD_YRES - (116 + 32),
		    LCD_XRES - 110, 
		    LCD_YRES - (116 + 49),
		    LCD_XRES - 311,
		    0);
    
    i = 0;
    while (i < nbytes)
      {
	size_t s;

	if (is_gzip)
	  {
	    unsigned char p[256];
	    s = gzRead (&z, p, 256);
	    if (z.err != Z_OK && z.err != Z_STREAM_END)
	      {
		putLabeledWord ("zerror ", z.err);
		error = "Decompression error";
		goto print_error;
	      }
	    err = ioh->ops->write (ioh, p, i, s);
	  }
	else
	  {
	    s = nbytes - i;
	    if (s > 32768)
	      s = 32768;
	    err = ioh->ops->write (ioh, buf + i, i, s);
	  }
	if (err < 0) {
	  int len;
	  memset(error_message_store, 0, sizeof(error_message_store));
	  strcpy(error_message_store, "Write failure, err=");
	  len = strlen(error_message_store);
	  binarytohex(error_message_store+len, err, 64);
	  error = error_message_store;
	  goto print_error;
	}

	i += s;

	lcd_progress_bar (LCD_YRES - (116 + 48),
			  LCD_XRES - 310,
			  LCD_YRES - (116 + 32),
			  LCD_XRES - 110,
			  (i * 100) / nbytes,
			  -LCD_HORIZONTAL,
			  0x07e0);
      }

    lcd_clear_region (LCD_YRES - (116 + 15), LCD_XRES - 105, 0, 0);

    draw_text ("Verifying...", 116 + 16, 110, 0x001f);
    draw_rectangle (LCD_YRES - (116 + 32),
		    LCD_XRES - 110, 
		    LCD_YRES - (116 + 49),
		    LCD_XRES - 311,
		    0);
    i = 0;
    while (i < nbytes)
      {
	size_t s = nbytes - i;
	static char verify_buf[1024];
	int j;

	if (s > 1024)
	  s = 1024;

	ioh->ops->read (ioh, verify_buf, i, s);

	if (is_gzip)
	  {
	    verify_crc = crc32 (verify_crc, verify_buf, s);
	  }
	else
	  {
	    for (j = 0; j < s; j += 4)
	      {
		unsigned long *p1 = (unsigned long *)(buf + i + j);
		unsigned long *p2 = (unsigned long *)(verify_buf + j);
		
		if (*p1 != *p2)
		  {
		    putLabeledWord("p1 = ", (int)p1);
		    putLabeledWord("p2 = ", (int)p2);
		    
		    putLabeledWord("*p1 = ", *p1);
		    putLabeledWord("*p2 = ", *p2);
		    
		    error = "Verify failure";
		    goto print_error;
		  }
	      }
	  }
	
	i += s;
	lcd_progress_bar (LCD_YRES - (116 + 48),
			  LCD_XRES - 310,
			  LCD_YRES - (116 + 32),
			  LCD_XRES - 110,
			  (i * 100) / nbytes,
			  -LCD_HORIZONTAL,
			  0x07e0);
      }

    putstr ("Done\r\n");

    ioh->ops->close(ioh);

    if (is_gzip)
      {
	putLabeledWord ("nbytes ", nbytes);
	putLabeledWord ("verify_crc ", verify_crc);
	putLabeledWord ("orig_crc ", z.read_crc32);

	if (verify_crc != z.read_crc32)
	  {
	    error = "Verify failure";
	    goto print_error;
	  }
      }
  }

  lcd_clear_region (LCD_YRES - (116 + 15), LCD_XRES - 105, 0, 0);

  draw_text ("Programmmed successfully", 116 + 24, 110, 0x001f);
  goto loop;
  
 print_error:
  lcd_clear_region (LCD_YRES - (116 + 15), LCD_XRES - 105, 0, 0);
  draw_text (error, 116 + 16, 110, 0xf800);
  goto loop;

 loop:
  draw_text ("Press action button to", 116 + 48, 110, 0x001f);
  draw_text ("restart the system", 116 + 56, 110, 0x001f);

  do {
    r = await_key ();
    putLabeledWord ("r = %d\n", r);
  } while (r != 10);

  if (error && (strcmp(target_store, "bootldr") == 0)) {
    /* if we got an error while programming the bootldr, it would be a bad idea to reset */
    return;
  } else {
    bootldr_reset();
  }
}

COMMAND(reflash, command_reflash, "invoke reflash utility", BB_RUN_FROM_RAM);
