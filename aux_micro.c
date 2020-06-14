#include "bootldr.h"
#include "btpci.h"
#include "btflash.h"
#include "btusb.h"
#include "heap.h"
#include "params.h"
#include "xmodem.h"
#include "lcd.h"
#include "aux_micro.h"
#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"
#include "serial.h"
#include "buttons.h"
#ifdef CONFIG_MACH_IPAQ
#include "asm-arm/arch-sa1100/h3600_gpio.h"
#endif

#include <string.h>

/* #define AUXM_DEBUG  1 */

static int last_button_pressed = 0;
static int exec_buttons_automatically = 1;

#ifndef DIM
#define	DIM(a)	(sizeof(a)/sizeof(a[0]))
#endif

typedef int (*msg_handler_t)(int msg_id, int msg_len, char* msg);
typedef void (*vfuncp_t)(void);

/*  #define BOOTLDR_TS */
#if defined(BOOTLDR_TS)

typedef struct screen_point_s
{
    int	x;
    int	y;
}
screen_point_t;

typedef struct screen_region_s
{
    screen_point_t  left_top;
    screen_point_t  right_bottom;
    vfuncp_t	    action;
}
screen_region_t;


/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * for certainty, go on like the 5-10th event in the
 * given region.
 */

void
serial_action(
    void)
{
    putstr("TS: serial action\n\r");
}

void
irda_action(
    void)
{
    putstr("TS: irda action\n\r");
}

void
boot_action(
    void)
{
    putstr("TS: boot action\n\r");
}


/*
 *  from the image map
<AREA SHAPE="RECT" COORDS="1,173,319,192" HREF="http://serial">
<AREA SHAPE="RECT" COORDS="1,193,319,213" HREF="http://irda">
<AREA SHAPE="RECT" COORDS="1,214,319,235" HREF="http://boot">
*/

screen_region_t	regions[] =
{
    {{213, 141}, {229, 318}, serial_action},
    {{175, 247}, {193, 318}, ide_action},
    {{44, 188}, {64, 318}, vfat_action},
    {{5, 213}, {25, 318}, boot_action}
};

screen_region_t*
find_region(
    int	x1,
    int	y1)
{
    int	    i;

    for (i = 0; i < DIM(regions); i++) {
	screen_region_t*    r = &regions[i];
	
	if (x1 >= r->left_top.x &&
	    y1 >= r->left_top.y &&
	    x1 <= r->right_bottom.x &&
	    y1 <= r->right_bottom.y) {
	    return (r);
	}
    }

    return (NULL);
}

typedef struct calibration_info_s
{
    int	    xscale;
    int	    xtrans;
    int	    yscale;
    int	    ytrans;
}
calibration_info_t;

calibration_info_t  def_cal_info =
{
    /* copped from h3600_ts.c */
    -93,
    346,
    -64,
    251
};

int
handle_pen(
    int	    msg_id,
    int	    msg_len,
    char*   msg)
{
    screen_region_t*	r;
    int			x, y;

    x = (msg[2] << 8) + msg[3];
    y = (msg[4] << 8) + msg[5];

    putLabeledWord("raw x: 0x", x);
    putLabeledWord("raw y: 0x", y);

    /* fmt->x=((pTsDev->cal.xscale*filtered.x)>>8)+pTsDev->cal.xtrans; */
    x = ((def_cal_info.xscale * x) >> 8) + def_cal_info.xtrans;
    y = ((def_cal_info.yscale * y) >> 8) + def_cal_info.ytrans;

    putLabeledWord("x: 0x", x);
    putLabeledWord("y: 0x", y);

    r = find_region(x, y);

    if (r)
	r->action();

    return (0);
}
#endif

/*
 * E.g.:
 * 02 21 04 25	: press
 * 02 21 84 15	: release
 */

/*
 * 1
 * 
 *
 *      3     4 
 *   2            5
 *         6
 *        8 7
 *         9
 */


#define	KIF_ENABLE_REBOOT_BUTTON    (1<<0)

typedef struct key_info_s
{
    char*	name;
    unsigned	flags;
    unsigned long h3800_bits;
    unsigned char h3800_pressed_state;
    // for graphics
    unsigned short r1;
    unsigned short c1;
    unsigned short r2;
    unsigned short c2;
    
}
key_info_t;

key_info_t  key_info_tab[] =
{
    {"placeholder", 0,0xffff,0,0,0,0,0},
    {"recb", 0,H3800_ASIC2_REC_BUTTON,0,0,0,0,0},
    {"calb", KIF_ENABLE_REBOOT_BUTTON,H3800_ASIC2_CAL_BUTTON,0,213,141,229,318},
    {"conb", KIF_ENABLE_REBOOT_BUTTON,H3800_ASIC2_CON_BUTTON,0,174,241,207,318},
    {"qb", KIF_ENABLE_REBOOT_BUTTON,H3800_ASIC2_Q_BUTTON,0,44,188,64,318},
    {"startb", KIF_ENABLE_REBOOT_BUTTON,H3800_ASIC2_START_BUTTON,0,5,213,25,318},
    {"upb",0,H3800_ASIC2_UP_BUTTON,0,0,0,0,0},
    {"rightb",0,H3800_ASIC2_RIGHT_BUTTON,0,0,0,0,0},
    {"leftb",0,H3800_ASIC2_LEFT_BUTTON,0,0,0,0,0},
// penguin hilite
    {"downb",0,H3800_ASIC2_DOWN_BUTTON,0,0,0,0,0},
    {"action", 0, H3800_ASIC2_ACTION_BUTTON, 0, 0, 0, 0, 0}
};

#define	KI_REBOOT_BUTTON(kip)  ((kip)->flags & KIF_ENABLE_REBOOT_BUTTON)

int
null_handler(
    int	    msg_id,
    int	    msg_len,
    char*   msg)
{
    return (0);
}

int
handle_key(
    int	    msg_id,
    int	    msg_len,
    char*   msg)
{
    int		key_down = !(msg[2] & 0x80);
    int		key_code = msg[2] & 0x7f;
    key_info_t*	key_info;
    

    
#if 0
    /* debug code */
    putstr("key ");
    if (key_down)
	putstr("pressed");
    else
	putstr("released");
	    
    putLabeledWord(", code: 0x", key_code);
#endif

    /* convert key code to name and look up its command val */
    key_info = &key_info_tab[key_code];
    
    if (key_down) {
	/*
	 * as a safety measure, we need to have one of the
	 * function buttons pressed before we'll allow the
	 * reboot button to function.
	 */
	// give graphic feedback
	if (exec_buttons_automatically)
	    hilite_button(key_code);
	

	if (KI_REBOOT_BUTTON(key_info))
	    enable_reboot_button();
    }
    else {
	last_button_pressed = key_code;
	// give graphic feedback (i.e. reset it)
	if (exec_buttons_automatically){	    
	    hilite_button(key_code);
	    exec_button(key_code);
	}
    }
    
    return (0);
}


/*
 * XXX
 * Disable the touchscreen for now since there are
 * calibration issues that it may not be worth the time
 * to work at this time.
 */

msg_handler_t	msg_handlers[16] =
{
    NULL,			/* 0 */
    NULL,			/* 1 */
    handle_key,			/* 2 */
#if defined(BOOTLDR_TS)
    handle_pen,			/* 3 */
#else
    NULL,			/* 3  */
#endif    
    NULL,			/* 4 */
    NULL,			/* 5 */
    NULL,			/* 6 */
    NULL,			/* 7 */
    NULL,			/* 8 */
    NULL,			/* 9 */
    NULL,			/* a */
    NULL,			/* b */
    NULL,			/* c */
    null_handler,		/* d, backlight status */
    NULL,			/* e */
    NULL			/* f */
};


enum
{
    MSG_STATE_INIT,
    MSG_STATE_STX,
    MSG_STATE_CMD,
    MSG_STATE_MSG,
    MSG_STATE_SUM
};

void
auxm_process_char(
    int	ch)
{
    static int	state = MSG_STATE_INIT;
    static int	msg_id;
    static int	msg_len;
    static int	csum;
    static int	msg_idx;
    static char	msg_buf[32];
    
    switch (state) {
	case MSG_STATE_INIT:
	    if (ch == 0x02) {	/* stx */
		msg_idx = 0;
		msg_buf[msg_idx++] = ch; /* save entire msg for debug */
		state = MSG_STATE_CMD;
	    }
	    break;

	case MSG_STATE_CMD:
	    /* got stx, get cmd */
	    msg_id = (ch >> 4) & 0x0f;
	    msg_len = ch & 0x0f;
	    csum = ch;
	    msg_buf[msg_idx++] = ch; /* save entire msg for debug */
	    state = (msg_len > 0) ? MSG_STATE_MSG : MSG_STATE_SUM;
	    break;

	case MSG_STATE_MSG:
	    csum += ch;
	    msg_buf[msg_idx++] = ch;
	    if (msg_idx == msg_len+2)
		state = MSG_STATE_SUM;
	    break;

	case MSG_STATE_SUM:
	    msg_buf[msg_idx++] = ch;
	    if ((csum & 0xff) != ch) {
		int i;
		putLabeledWord("Auxm checksum error, csum: 0x", csum&0xff);
		for (i = 0; i < msg_idx; i++)
		    putLabeledWord("msg byte: 0x", msg_buf[i]);
	    }
	    else {
#if 0
		putLabeledWord("msg_id: 0x", msg_id);
#endif
		if (msg_handlers[msg_id]) {
#if 0
		    {	/* XXX - tmp debug code: remove me! */
			putLabeledWord("calling handler: 0x",
				       (unsigned long)msg_handlers[msg_id]);
		    }	/* XXX - tmp debug code: remove me! */
#endif
		    msg_handlers[msg_id](msg_id, msg_len, msg_buf);
		}
#if defined(AUXM_DEBUG)		
		else {
		    int i;
		    putstr("Auxm: unhandled message:\n\r");
		    for (i = 0; i < msg_idx; i++)
			putLabeledWord("msg byte: 0x", msg_buf[i]);
		}
#endif		
		
	    }
	    state = MSG_STATE_INIT;
	    break;

	default:
	    putLabeledWord("Bad state in auxm_process_char, 0x", state);
	    state = MSG_STATE_INIT;
	    break;
    }
}
    
#if !defined(CONFIG_PXA)
int
auxm_init_serial()
{
    unsigned int status;

        /* TODO registers read in 32bit chunks - need to do this for the sa1110? */

        /* Clean up CR3 for now */
        /*  ( (volatile unsigned long *)port)[UTCR3]= 0; */
	CTL_REG_WRITE(UART1_UTCR3, 0);

        /* 8 bits no parity 1 stop bit */
        /*  ( (volatile unsigned long *)port)[UTCR0] = 0x08; */
	CTL_REG_WRITE(UART1_UTCR0, 0x08);

        /* Set baud rate MS nibble to zero. */
        /*  ( (volatile unsigned long *)port)[UTCR1] = 0; */
	CTL_REG_WRITE(UART1_UTCR1, 0);

        /* Set baud rate LS nibble to 115K bits/sec */
        /*  ( (volatile unsigned long *)port)[UTCR2]=0x1; */
	CTL_REG_WRITE(UART1_UTCR2, 0x01);

        /*
	  SR0 has sticky bits, must write ones to them to clear.
          the only way you can clear a status reg is to write a '1' to clear.
        */
        /*  ( (volatile unsigned long *)port)[UTSR0]=0xff; */
	CTL_REG_WRITE(UART1_UTSR0, 0xff);

        /*
          DISable rx fifo interrupt
          If RIE=0 then sr0.RFS and sr0.RID are ignored ELSE
          if RIE=1 then whenever sro.RFS OR sr0.RID are high an
          interrupt will be generated.
          Also enable the transmitter and receiver.
        */
        status = UTCR3_TXE | UTCR3_RXE;
        /*  ( (volatile unsigned long *)port)[UTCR3]=status; */
	CTL_REG_WRITE(UART1_UTCR3, status);

        return 0;	/* TODO how do we denote an error */

} // End of function initSerial
    

void
auxm_fini_serial()
{
    /* disable tx & rx (and everything else */
    CTL_REG_WRITE(UART1_UTCR3, 0);
}



#define	SERIAL_READ_STATUS()	((*(volatile byte *)UART1_UTSR1) & UTSR1_ERROR_MASK)

int
auxm_serial_read(
    char*	    buf,
    int*	    max,
    unsigned long   timeout)
{
    int	num = 0;
    int	do_timeout = timeout != 0;
    unsigned status;

    if (!machine_has_auxm())
      goto out;

    while ((num < *max) &&
	   (CTL_REG_READ(UART1_UTSR1) & UTSR1_RNE) &&
	   (!do_timeout || timeout--)) { 
	*buf++ = CTL_REG_READ_BYTE(UART1_UTDR);
	status = SERIAL_READ_STATUS();
	if (status) {
	    putLabeledWord("auxm rx status: 0x", status);
	}
	num++;
    }

 out:
    if (max)
      *max = num;

    return (0);
}

void
auxm_serial_check(
    void)
{
    char    buf[24];
    int	    len = 16;
    int	    i;

    if (!machine_has_auxm())
      return;

    /* hack for action button */
    if (!(GPIO_GPLR_READ() & GPIO_H3600_ACTION_BUTTON))
      {
	last_button_pressed = 10;
	return 0;
      }
    
    auxm_serial_read(buf, &len, 0);

    for (i = 0; i < len; i++) {
	auxm_process_char(buf[i] & 0xff);
    }
}


void
auxm_send_char(
    unsigned char   ch)
{
    unsigned long   timeout = TIMEOUT/2;
    
    if (!machine_has_auxm())
      return;
    
    while (timeout-- && !(CTL_REG_READ(UART3_UTSR1) & UTSR1_TNF))
	;

    if (CTL_REG_READ(UART3_UTSR1) & UTSR1_TNF)    
	CTL_REG_WRITE_BYTE(UART1_UTDR, ch);
    else
	putLabeledWord("Timeout on write, stat reg:",
		       CTL_REG_READ(UART3_UTSR1));
}


#endif // !defined CONFIG_PXA

int machine_has_auxm()
{
    if (machine_is_h3600() || machine_is_h3100()) 
      return 1;
    else
      return 0;
}

int
auxm_init()
{
    int rc = 0;
    
#if !defined(CONFIG_PXA)
    if (machine_has_auxm()) {
      putstr("Initializing aux micro\r\n");
      rc = auxm_init_serial();
    }
#endif
    return (rc);
}

#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900)

// the 3800 doesn't use the aux microcontroller but this is
// where all the other button handlers live so we'll join in
// eventually we need to reorg
// this one doesn;t support the reboot button (too scary as a feature anyway)
void
check_3800_func_buttons(
    void)
{
    unsigned short *p = (unsigned short *)H3800_ASIC2_KPIO_ADDR;
    unsigned short kpio_bits;
    int i;
    key_info_t*	key_info;



    kpio_bits = *p;


    for (i=0; i < DIM(key_info_tab); i++){
	key_info = &key_info_tab[i];
	if ((key_info->h3800_bits & (kpio_bits & key_info->h3800_bits)) == 0){	    
#if 0
	    putLabeledWord("3800 kpio is 0x",kpio_bits);
	    putLabeledWord("3800 keypress for entry i:",i);
#endif
	    last_button_pressed = i;	    
	    // give graphic feedback
	    if (!key_info->h3800_pressed_state){
		if (exec_buttons_automatically)
		    hilite_button(last_button_pressed);
		key_info->h3800_pressed_state = 1;
	    }
	}	
	else if (key_info->h3800_pressed_state &&
		 ((kpio_bits & key_info->h3800_bits) != 0)){	    
	    key_info->h3800_pressed_state = 0;
	    if (exec_buttons_automatically){
		hilite_button(last_button_pressed);
		exec_button(last_button_pressed);
	    }
	}
    }
}
#endif /* defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900) */

	    
#if !defined(CONFIG_PXA)


int
auxm_serial_dump(
    void)
{
    int	    rc;
    char    buf[64];
    int	    num;
    int	    i;
    
    if (!machine_has_auxm()) {
      putstr("this machine has no auxm\n");
      return;
    }

    rc = auxm_init();
    if (rc)
	return (rc);

    while (1) {
	if (CTL_REG_READ(UART3_UTSR1) & UTSR1_RNE) {
	    putstr("console char read, exiting\n\r");
	    return (0);
	}

	num = sizeof(buf);
#if 0
	putLabeledWord("before, num=0x", num);
	putLabeledWord("UART1_UTSR0=0x", UART1_UTSR0);
	putLabeledWord("UART1_UTSR1=0x", UART1_UTSR1);
#endif
	
	auxm_serial_read(buf, &num, 0);
#if 0
	putLabeledWord("after, num=0x", num);
#endif
	if (num) {
	    for (i = 0; i < num; i++) {
		putHexInt8(buf[i]);
		putstr(" ");
	    }
	    putstr("\r\n");
	}
    }
}

#endif // !defined(config_pxa)

void
auxm_send_cmd(
    int	    cmd,
    int	    len,
    char*   data)
{
    unsigned char   cmd_byte = (cmd << 4) | len;
    unsigned char   csum = cmd_byte;
    int		    i;

    if (!machine_has_auxm())
      return;

#if !defined(CONFIG_PXA)
    auxm_send_char('\02');
    auxm_send_char(cmd_byte);

    for (i = 0; i < len; i++) {
	auxm_send_char(data[i]);
	csum += data[i];
    }

    auxm_send_char(csum);
#endif // config_pxa
}
// these fxns sphagettiize the code to allow us to
// deal with buttons better during hte jffs2 scan
int get_last_buttonpress()
{
  return last_button_pressed;
}
void set_last_buttonpress(char which)
{
    last_button_pressed = which;
}

void set_exec_buttons_automatically(char c)
{
    exec_buttons_automatically = c;
}
char get_exec_buttons_automatically(void)
{
    return exec_buttons_automatically;
}


void hilite_button(int which)
{
    key_info_t*	key_info;

    key_info = &key_info_tab[which];
    // give graphic feedback
#if CONFIG_LCD
#ifndef NO_HILITE_BUTTON
    lcd_invert_region(key_info->r1,
		      key_info->c1,
		      key_info->r2,
		      key_info->c2);
#endif
#endif

}
void exec_button(int which)
{
    key_info_t*	key_info;
    char	buf[64];
    struct bootblk_param *cmd_param;    
    char*	cmd_str = NULL;

    if (machine_is_jornada56x()) return;
    key_info = &key_info_tab[which];
    strcpy(buf, key_info->name);
    strcat(buf, "_cmd");

    cmd_param = get_param(buf);
    if (cmd_param) {
       cmd_str = (char *)cmd_param->value;
    }

    /* see if there is a command parameter */
    if (cmd_str) {
#if 0	
       putstr("cmd>");
       putstr(cmd_str);
       putstr("<");
#endif
       /*
        * if this button is pressed whilst the initial
        * awaitkey() is running, then it will be
        * lost. So a backspace seemed least
        * injurious.
        */
	    
       push_cmd_chars("\010", 1); /* backspace */
       push_cmd_chars(cmd_str, strlen(cmd_str));
       push_cmd_chars("\r", 1);
       if (KI_REBOOT_BUTTON(key_info))
          disable_reboot_button();
    }	
}
