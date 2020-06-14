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

#include "bootldr.h"
#include "params.h"
#if defined(__linux__) || defined(__QNXNTO__)
#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900)
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
#endif /* CONFIG_MACH_IPAQ */
#endif
#ifdef CONFIG_PXA
#include "pxa.h"
#endif
#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"
#include "serial.h"
#include "buttons.h"

extern int check_for_func_buttons;
extern long reboot_button_enabled;

void
button_check(
    void)
{
#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900)
    unsigned long mach_type = 0;
    static int	action_button_was_pressed = 0;
    static int	sleep_button_was_pressed = 0;
    unsigned long   gpio_bits;
    unsigned short *p = H3800_ASIC2_KPIO_ADDR;
    unsigned short kpio_bits = 0;
    
    mach_type = param_mach_type.value;

    if (machine_is_jornada56x())
        return;

#ifdef CONFIG_PXA
    gpio_bits = GPDR0;
#else
    gpio_bits = GPIO_GPLR_READ();
#endif

    if (mach_type == MACH_TYPE_H3800 || mach_type == MACH_TYPE_H3900){
	kpio_bits = *p;	
	if (action_button_was_pressed &&
	    (kpio_bits & H3800_ASIC2_ACTION_BUTTON) != 0) {
	    action_button_was_pressed = 0;
	    if (reboot_button_is_enabled()) {
		bootldr_reset();
	    }
	}
    }    
    else {
	if (action_button_was_pressed &&
	    (gpio_bits & (1<<18)) != 0) {
	    action_button_was_pressed = 0;
	    if (reboot_button_is_enabled()) {
		bootldr_reset();
	    }
	}
    }

    // sleep buttons are the same 
    if (sleep_button_was_pressed &&
	(gpio_bits & (1<<0)) != 0) {
	sleep_button_was_pressed = 0;
	putstr_sync("Snoozing...");
	bootldr_goto_sleep(NULL);
    }
    
    if (mach_type == MACH_TYPE_H3800 || mach_type == MACH_TYPE_H3900){
	if ((kpio_bits & H3800_ASIC2_ACTION_BUTTON) == 0) {
	    action_button_was_pressed = 1;	
	}
    }
    else {
	if ((gpio_bits & (1<<18)) == 0) {
	    action_button_was_pressed = 1;	
	}
    }
    
    if ((gpio_bits & (1<<0)) == 0) {
	sleep_button_was_pressed = 1;	
    }

    if (check_for_func_buttons && !machine_is_jornada56x()) {
	if (mach_type == MACH_TYPE_H3800 || mach_type == MACH_TYPE_H3900){
	    check_3800_func_buttons();	    
	}
	else {
#ifdef CONFIG_MACH_IPAQ
          if (machine_has_auxm())
	    auxm_serial_check();
#endif
	}	
    }
#endif
}



