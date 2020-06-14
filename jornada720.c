/****************************************************************************/
/* Copyright 2000 Hewlett-Packard Company.                                  */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  HEWLETT-PACKARD COMPANY                 */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/

#include "architecture.h"
#include "bootldr.h"
#include "cpu.h"
#include "heap.h"
#include "lcd.h"
#include "pbm.h"

void display_pbm(unsigned char *src,unsigned long size)
{
}
int machine_has_auxm(void)
{
  return 0;
}
void auxm_send_cmd(int cmd, int len, char* data)
{
}
int auxm_serial_dump(void)
{
	return 0;
}
int auxm_init()
{
	return 0;
}
void auxm_serial_check(void)
{
}
void auxm_fini_serial()
{
}
void hilite_button(int which)
{
}
void exec_button(int which)
{
}
void set_last_buttonpress(char which)
{
}
int get_last_buttonpress(void)
{
	return 0;
}
void led_blink(char onOff,char totalTime,char onTime,char offTime)
{
}
void set_exec_buttons_automatically(char c)
{
}
void lcd_bar(int color, int percent)
{
}
void lcd_bar_clear(int percent, int offset)
{
}
