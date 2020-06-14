/****************************************************************************/
/* Copyright 2001 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * button support
 *
 */

extern int check_for_func_buttons;
extern long reboot_button_enabled;

void button_check(void);


#define H3800_ASIC2_ACTION_BUTTON  ( 1 << 5 )
#define H3800_ASIC2_CAL_BUTTON  ( 1 << 6 )
#define H3800_ASIC2_CON_BUTTON  ( 1 << 7 )
#define H3800_ASIC2_Q_BUTTON  ( 1 << 8 )
#define H3800_ASIC2_START_BUTTON  ( 1 << 9 )
#define H3800_ASIC2_UP_BUTTON  ( 0x010 )
#define H3800_ASIC2_DOWN_BUTTON  ( 0x02c )
#define H3800_ASIC2_LEFT_BUTTON  ( 0x400 )
#define H3800_ASIC2_RIGHT_BUTTON  ( 0x800 )
#define H3800_ASIC2_REC_BUTTON  ( 0x001 )
