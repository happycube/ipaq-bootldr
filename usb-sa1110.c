/* usb-sa1110.c
 * USB support
 * handhelds.org open bootldr
 *
 * code that is unclaimed by others is
 *    Copyright (C) 2003 Joshua Wise <joshua at joshuawise dot com>
 *
 * release notes:
 *  doesn't work from internal flash at the moment. :(
 *  has to do slow serial debugging. looking into fixing.
 *  doesn't work properly when connected to a windows system (that's fine, we need drivers anyway :)
 * 
 * acknowledgements:
 *  thanks to Philip Blundell for spending so much time with me on IRC trying to debug this, and writing major portions of code.
 *  thanks to everyone who contributed to the usb support in the Linux kernel.
 *  no thanks to Intel for having a broken chip.
 *  I hope I didn't miss anyone, if I did, email me.
 *  see you all at devweekend!
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "commands.h"
#include "bootldr.h"
#include "serial.h"
#include "lcd.h"
#include "usb-sa1110.h"

/* Much code from the Linux kernel's arch/arm/mach-sa1100/usb* files.
 * Copyright (C) Compaq Computer Corporation, 1998, 1999
 * Copyright (C) Extenex Corporation, 2001
 * Copyright (c) 2001 by Nicolas Pitre
 */

#ifndef CONFIG_ACCEPT_GPL
#  error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#define USBVERSION "1.0rc1"
#define MAXBYTES 128

extern void msleep(int);
extern void oscrsleep(int);

int oldimp;

/* serial prototypes */
	int usb_haschars(struct serial_device *sd);
	unsigned char usb_readchar(struct serial_device *sd);
	unsigned int usb_read_status(struct serial_device *sd) { return 0; } /* no error */;
	int usb_setbaud(struct serial_device* sd, int speed) { return 230400; /*fast enough */ };
	void* usb_serialbase(struct serial_device *sd) { return NULL; };
	void usb_writechar(struct serial_device *sd, unsigned char c);
	void usb_drainuart(struct serial_device *sd);
	
	static struct serial_device usb_serial = {
	  char_ready: usb_haschars,
	  read_char: usb_readchar,
	  read_status: usb_read_status,
	  write_char: usb_writechar,
	  set_baudrate: usb_setbaud,
	  serbase: usb_serialbase,
	  drain_uart: usb_drainuart,
	  enabled: 1,
	  _serbase: 0xDEADC0DE
	};

	static struct serial_device *oldser = 0;
	extern struct serial_device *serial;
/* end serial prototypes */

/* xmit and recv fifos */
	unsigned char biggerfifo[1024];
	volatile static unsigned char* fifop = biggerfifo;
	volatile static int fifobytes = 0;
	volatile static int firstread = 1;


#define FIFOSIZE 512	
	unsigned char xmitfifo[FIFOSIZE+1];
	volatile static int xmitbytes=0;
	
	volatile static int xmitfailed = 0;
	volatile static int xmitlen = 0;
	unsigned char* xmitbuf;
	
	static int ignoresendint = 0;
	volatile static int sendbusy = 0;
/* end xmit and recv fifos */

/* generic USB prototypes */
	static void initialize_descriptors(void);
	static void sh_setup_begin( void );
	static inline void enable_suspend_mask_resume(void);
	static inline void enable_resume_mask_suspend(void);
/* end generic USB prototypes */

/* generic state code */
	static int usb_initted = 0;
	static int fifomode = 0;
	static int sm_state = kStateZombie;
	static int usbstate = USB_STATE_NOTATTACHED;
	static int address = 0;
	static void (*current_handler)(void) = sh_setup_begin;
	desc_t desc;
	static int noints;
/* end generic state code */

/* our specific USB code */
	static void write_byte( char b );
/* end specific USB code */

/* screen output code */
	static int y = 16;
#if NOSERIAL
	void CHECKDRAW(char* t) { if (y > 240) {lcd_clear_region(0,0,240-16,320);y = 16;}; draw_text(t, y, 0, 0); y+=8; }
#else
	void CHECKDRAW(char* t) { 
		if (oldser) 	{while (*t)
				{
					oldser->write_char(oldser, *t);
					t++;
				}; oldser->write_char(oldser, '\r'); oldser->write_char(oldser, '\n');}
		else 		{putstr(t);putstr("\r\n");};
	};
#endif
/* end screen output code */



SUBCOMMAND(usb, init, command_usb_init, "-- starts usb", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, status, command_usb_status, "-- does a status check on USB", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, sercon, command_usb_sercon, "-- switches back to serial console", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, deinit, command_usb_deinit, "-- sets usb_initted to 0", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, loop, command_usb_loop, "-- does a 10sec poll loop", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, version, command_usb_version, "-- prints USB codebase version", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, spit4, command_usb_spit4, "-- spits 4 letters out", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, spit8, command_usb_spit8, "-- spits 8 letters out", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, spit12, command_usb_spit12, "-- spits 12 letters out", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, spit16, command_usb_spit16, "-- spits 16 letters out", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, spit20, command_usb_spit20, "-- spits 20 letters out", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, console, command_usb_serial, "-- switches to console on USB.", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(usb, autoinit, command_usb_autoinit, "-- auto-inits USB", BB_RUN_FROM_RAM, 1);

int usb_getinitted() { return usb_initted; };
extern int int_getinitted();
extern int command_int_init(int, char**);
extern int command_dma_init(int, char**);

void command_usb_version(int argc, const char** argv)
{
	putstr("version " USBVERSION "\r\n");
};

void command_usb_deinit(int argc, const char** argv)
{
	usb_initted=0;
};


void command_usb_spit4(int argc, const char** argv){do_send("aaaa", 4);};
void command_usb_spit8(int argc, const char** argv){do_send("aaaaaaaa", 8);};
void command_usb_spit12(int argc, const char** argv){do_send("aaaaaaaaaaaa", 12);};
void command_usb_spit16(int argc, const char** argv){do_send("aaaaaaaaaaaaaaaa", 16);};
void command_usb_spit20(int argc, const char** argv){do_send("aaaaaaaaaaaaaaaaaaaa", 20);};

void command_usb_autoinit(int argc, const char** argv)
{
	command_usb_init(0,0);
	
	putstr("Please disconnect serial and connect USB to the device.\r\n");
	putstr("After doing so, launch a console on /dev/usb/tts/0\r\n and");
	putstr("input a character.\r\n");
	while (fifobytes == 0)
	{
		putstr(".");
		msleep(250);
	};
	putstr("Now switching to console on USB...\r\n");
	command_usb_serial(0,0);
	putstr("Now switching to console on USB... success!!\r\n");
};

void command_usb_sercon(int argc, const char** argv)
{
	fifomode = 0;
	putstr("Console is now over serial.\r\n");
	if (oldser)
	{
		serial = oldser;
		putstr("Console is now over serial.\r\n");
	};
};

void command_usb_init(int argc, const char** argv)
{
	int loopvar,i;
	
	if (usb_initted == 2)
		putstr("WARNING: previous failure initting USB. things may fail again this time due to a weird state.\r\n");
	if (usb_initted)
	{
		putstr("eeps! usb already initted! things might be in a bad state, so we won't do it again, kk?\r\n");
		return;
	};
	
	
	if (!int_getinitted())
	{
		putstr("interrupts must be enabled.\r\n");
		command_int_init(0,0);
		if (!int_getinitted())
		{
			putstr("failed to initialize interrupts\r\n");
			return;
		};
	};
	
	sendbusy = 0;
	
	
	oldser=0;
	
	fifop=biggerfifo;
	fifobytes=0;
	fifomode = 0;
	firstread = 1;
	
	initialize_descriptors();
sm_state = kStateZombie;
usbstate = USB_STATE_NOTATTACHED;
address = 0;
xmitlen = 0;
y=16;
noints=0;
current_handler=sh_setup_begin;	
	usb_initted = 2;
	
	loopvar=10000;
	do {
		Ser0UDCCR |= UDCCR_UDD;
		if (loopvar-- <= 0) {
			putstr("timeout disabling UDC\r\n");
			return;
		};
		msleep(2);
	} while(!(Ser0UDCCR & UDCCR_UDD));
	
	loopvar=10000;
	do {
		Ser0UDCCR &= ~UDCCR_UDD;
		if (loopvar-- <= 0) {
			putstr("timeout enabling UDC\r\n");
			return;
		};
		msleep(1);
	} while(Ser0UDCCR & UDCCR_UDD);
	
	loopvar=10000;
	do {
		Ser0UDCCS1 &= ~(UDCCS1_FST | UDCCS1_RPE | UDCCS1_RPC);
		if (loopvar-- <= 0) {
			putstr("timeout clearing 1\r\n");
			return;
		};
		msleep(1);
	} while(Ser0UDCCS1 & (UDCCS1_FST | UDCCS1_RPE | UDCCS1_RPC));
	
	loopvar=10000;
	do {
		Ser0UDCCS2 &= ~(UDCCS2_FST | UDCCS2_TPE | UDCCS2_TPC);
		if (loopvar-- <= 0) {
			putstr("timeout clearing 2\r\n");
			return;
		};
		msleep(1);
	} while(Ser0UDCCS2 & (UDCCS2_FST | UDCCS2_TPE | UDCCS2_TPC));
	
	while ((Ser0UDCCR & UDCCR_ER29) != UDCCR_ER29)
		Ser0UDCCR |= UDCCR_ER29;
	
	Ser0UDCCR = 0xFC;
	
	Ser0UDCSR = UDCSR_RSTIR | UDCSR_RESIR | UDCSR_EIR | UDCSR_RIR  | UDCSR_TIR  | UDCSR_SUSIR;
	
	Ser0UDCCR  |= UDCCR_RESIM;
	
	loopvar=10000;
	do {
		Ser0UDCCR = UDCCR_SUSIM;
		if (loopvar-- <= 0) {
			putstr("timeout unmasking interrupts\r\n");
			return;
		};
		msleep(1);
	} while(Ser0UDCCR != UDCCR_SUSIM);
	
	i=10000;
	while(Ser0UDCOMP != 0xFF){
		Ser0UDCOMP=0xFF;
		i--;
		if (i == 0)
		{
			CHECKDRAW("Couldn't set Ser0UDCOMP");
			return;
		};
		msleep(1);
	};
	
	putstr("usb successfully enabled\r\n");
	usb_initted = 1;
};

void command_usb_status(int argc, const char** argv)
{
	u32 status = Ser0UDCSR;
	

	msleep(1);
	msleep(1);
	msleep(1);
	msleep(1);
	msleep(1);
	msleep(1);
	msleep(1);
	msleep(1);
	
	if (!usb_initted)
	{
		putstr("USB not initialized! run usbinit first!\r\n");
		return;
	}
	if (usb_initted == 2)
	{
		putstr("previous failure initting USB. run usbinit again to try again.\r\n");
		return;
	};
	
	putstr("doing a single poll.\r\n");
	putLabeledWord("Ser0UDCSR: ", status);
	
	if (status & UDCSR_RSTIR)
		putstr("reset interrupt request\r\n");
	if (status & UDCSR_RESIR)
		putstr("resume interrupt request\r\n");
	if (status & UDCSR_SUSIR)
		putstr("suspend interrupt request\r\n");
	if (status & UDCSR_TIR)
		putstr("transmit interrupt request\r\n");
	if (status & UDCSR_RIR)
		putstr("recv interrupt request\r\n");
	if (status & UDCSR_EIR)
		putstr("ep0 interrupt request\r\n");
	
};

void stradd(char** dest, char* src)
{
	strcpy(*dest, src);
	*dest += strlen(src);
};

int do_send(unsigned char* foo, int count)
{
	int loopvar;
	int rcount;
	int massive_attack;
	int oxmitlen = count;
	int bytes;
	
	bytes=0;
retry:

	if (!foo)
		return 0;
//	CHECKDRAW("do_send");
	if (sendbusy)
	{
		char wbuf[4];
		CHECKDRAW("still sending something else!");
		loopvar = 10000;
		while (sendbusy)
		{
			if (loopvar == 0)
				break;
			oscrsleep(1);
			loopvar--;
		};
		if (loopvar == 0)
		{
			CHECKDRAW("timed out waiting for send complete");
			CHECKDRAW("proceeding anyway");
		};
	};
	
	sendbusy = 1;
	
	if (count == 0)
	{
		CHECKDRAW("count == 0??");
		sendbusy = 0;
		return 0;
	};
	count = (count > 256) ? 256 : count;
	xmitlen = count;
	xmitbuf = foo;
	
	oldimp = Ser0UDCIMP;
	
	Ser0UDCIMP = count-1;
	massive_attack=20;
	while (Ser0UDCIMP != count-1 && massive_attack--)
	{
		oscrsleep(50*(massive_attack-19));
		Ser0UDCIMP = count-1;
	};
	if (massive_attack != 20)
	{
		if (massive_attack == 0)
			CHECKDRAW("MASSIVE ATTACK failed :(");
		else
			CHECKDRAW("MASSIVE ATTACK worked :)");
		
		oscrsleep(250);
	};
	
	
	/* start DMA */
	dma_startsend(foo, count);
	
	loopvar=10000;
	while (loopvar--)
	{
		if (!sendbusy)
			break;
		oscrsleep(10);
	};
	
	if (xmitfailed)
	{
		xmitfailed=0;
		oscrsleep(800); /* try, try again */
		goto retry;
	}
theend:
	return 0;
};

void send_int()
{
	int loopvar;
	int count;
	int rcount;

	if (ignoresendint)
	{
		loopvar = 10000;
		do {
			Ser0UDCSR = UDCSR_TIR;
			if (loopvar-- <= 0) {
				CHECKDRAW("timeout flipping tx status");
				return;
			};
			msleep(1);
		} while(Ser0UDCSR & UDCSR_TIR);
		CHECKDRAW("ignored interrupt");
		return;
	};
	
	if (!sendbusy)
	{
		loopvar = 10000;
		do {
			Ser0UDCSR = UDCSR_TIR;
			if (loopvar-- <= 0) {
				CHECKDRAW("timeout flipping tx status");
				return;
			};
			msleep(1);
		} while(Ser0UDCSR & UDCSR_TIR);
		CHECKDRAW("hmm, TIR, but not sendbusy >.<");
		return;
	};
		
	if (xmitlen == 0) goto fail;
//	CHECKDRAW("send_int");
	if (Ser0UDCCS2 & UDCCS2_TFS)		/* FIFO needs more bytes */
	{
//		CHECKDRAW("TFS");
	};
	
	if (Ser0UDCCS2 & UDCCS2_TUR)
	{
		CHECKDRAW("Too late, underrun");
		goto fail;
	};
	
	if (Ser0UDCCS2 & UDCCS2_TPE)
	{
		CHECKDRAW("TPE");
		goto fail;
	};
	
	if (Ser0UDCCS2 & UDCCS2_TPC)
	{
		loopvar=10000;
		do {
			Ser0UDCCS2 |= UDCCS2_TPC;
			if (loopvar-- <= 0)
			{
				CHECKDRAW("TPC failure");
				sendbusy = 0;
				xmitlen = 0;
				break;
			};
		} while (Ser0UDCCS2 & UDCCS2_TPC);
		xmitlen = 0;
		sendbusy = 0;
		xmitfailed=0;
	};
	
	return;
	
fail:
	sendbusy = 0;
	loopvar=10000;
	do {
		Ser0UDCCS2 |= UDCCS2_TPC;
		if (loopvar-- <= 0)
		{
			CHECKDRAW("TPC failure");
			break;
		};
	} while (Ser0UDCCS2 & UDCCS2_TPC);
	
	
	loopvar = 10000;
	do {
		Ser0UDCSR = UDCSR_TIR;
		if (loopvar-- <= 0) {
			CHECKDRAW("timeout flipping tx status");
			return;
		};
		msleep(1);
	} while(Ser0UDCSR & UDCSR_TIR);
	xmitfailed=1;
};

void do_recv(int tofifo)
{
	unsigned char wbuf[256];
	unsigned char* pbuf = wbuf;
	unsigned char* b2hbuf[5];
	int bytes = 0;
	int loopvar;
	
	bytes=0;
	
	if (firstread)
	{
		command_dma_init(0,0);
		firstread=0;
	}
	
	if (Ser0UDCCS1 & UDCCS1_RFS)
	{
		/* ok, eat bytes */
		for (loopvar=0; loopvar<12; loopvar++)
		{
			*pbuf = Ser0UDCDR;
			pbuf++;
			bytes++;	
		};
		CHECKDRAW("RFS");
	};
		
	if (Ser0UDCCS1 & UDCCS1_RPC)
	{
		int dmabytes = 0;
		
		if (Ser0UDCCS1 & UDCCS1_RPE)
		{
			bytes = 0;
			CHECKDRAW("RPE");
			goto rir;
		};		
	
		bytes = dma_grabbytes(pbuf);
		for (dmabytes = 0; dmabytes < bytes; dmabytes++)
		{
			*fifop = pbuf[dmabytes];
			fifop++;
			fifobytes++;
		};
		
		while (Ser0UDCCS1 & UDCCS1_RNE)
		{
			*fifop = Ser0UDCDR;
			fifop++;
			fifobytes++;
		};
rir:		
		loopvar = 10000;
		do {
			Ser0UDCSR = UDCSR_RIR;
			if (loopvar-- <= 0) {
				CHECKDRAW("timeout flipping rx status");
				return;
			};
			msleep(1);
		} while(Ser0UDCSR & UDCSR_RIR);
		
		
		loopvar = 0;
		
		
		if (Ser0UDCCS1 & UDCCS1_SST)
		{
			CHECKDRAW("SST.");
			for (loopvar=0; loopvar < 10000; loopvar++)
			{
				Ser0UDCCS1 = UDCCS1_SST;
				if (!(Ser0UDCCS1 & UDCCS1_SST))
					goto blah;
			};
		};
blah:		
		if (loopvar >= 9999)
			CHECKDRAW("Code: Whisky Tango Foxtrot (could not clear SST)");
		if (Ser0UDCCS1 & UDCCS1_FST)
		{
			CHECKDRAW("FST. We didn't set this, who did?");
			for (loopvar=0; loopvar < 10000; loopvar++)
			{
				Ser0UDCCS1 = UDCCS1_FST;
				if (!(Ser0UDCCS1 & UDCCS1_FST))
					break;
			};
		};
		loopvar=10000;
		do{
			Ser0UDCCS1 = UDCCS1_RPC;
			loopvar--;
			if (loopvar <= 0)
			{
				CHECKDRAW("eek, couldn't clear rpc!");
				break;
			};
		} while (Ser0UDCCS1 & UDCCS1_RPC);
	};
};

void do_ep0()
{
	int loopvar;
	
	
	if (current_handler != sh_setup_begin)  
	{
		u32 cs_reg_in = Ser0UDCCS0;
		//CHECKDRAW("not sh_setup_begin");
		if ( cs_reg_in & UDCCS0_SE ) {
			Ser0UDCCS0 =  UDCCS0_SSE;              /* clear setup end */
//			CHECKDRAW("SE clear");
			current_handler = sh_setup_begin;
		}
		
		if ( cs_reg_in & UDCCS0_SST ) {
			Ser0UDCCS0 = UDCCS0_SST;               /* clear setup end */
//			CHECKDRAW("SST clear");
			current_handler = sh_setup_begin;
		}
		
		if ( cs_reg_in & UDCCS0_OPR ) {
//			CHECKDRAW("OPR clear");
			loopvar=10000;
			if (Ser0UDCCS0 & UDCCS0_IPR) {
				do {
					Ser0UDCCS0 &= ~UDCCS0_IPR;
					if (loopvar-- <= 0) {
						putstr("timeout blocking on IPR\r\n");
						return;
					};
					oscrsleep(1);	/* voooodoooooo */
				} while(Ser0UDCCS0 & UDCCS0_IPR);
			};
			current_handler = sh_setup_begin;
		};
	};
	
	(*current_handler)();
};

void do_suspend()
{
	usb_nextstate(kEvSuspend);
	enable_resume_mask_suspend();
};

void do_resume()
{
	u32 car = Ser0UDCAR;
	u32 imp = Ser0UDCIMP;
	u32 omp = Ser0UDCOMP;
	int loopvar;
	
	usb_nextstate(kEvResume);
	loopvar=10000;
	do {
		Ser0UDCCR |= UDCCR_UDD;
		if (loopvar-- <= 0) {
			putstr("timeout enabling UDC\r\n");
			return;
		};
		oscrsleep(2);
	} while(!(Ser0UDCCR & UDCCR_UDD));
	oscrsleep(200);
	loopvar=10000;
	do {
		Ser0UDCCR &= ~UDCCR_UDD;
		if (loopvar-- <= 0) {
			putstr("timeout enabling UDC\r\n");
			return;
		};
		oscrsleep(1);
	} while(Ser0UDCCR & UDCCR_UDD);
	Ser0UDCAR = car;
	Ser0UDCIMP = imp;
	Ser0UDCOMP = omp;
	enable_suspend_mask_resume();
};

void do_reset(int status)
{
	int loopvar;
	if ( usb_nextstate( kEvReset ) != kError )
	{
		/* ugh, endpoint resets */
		/* ep0 reset */
		
		current_handler = NULL; /* don't have that yet */
		/* wr. stuff not applicable? */
		
		/* ep1 reset */
		/* no DMA, don't need to flush it */
		/* clear FST... ugh */
		loopvar=10000;
		do {
			Ser0UDCCS1 &= ~(UDCCS1_FST);
			if (loopvar-- <= 0) {
				putstr("timeout clearing 1\r\n");
				return;
			};
			msleep(1);
		} while(Ser0UDCCS1 & (UDCCS1_FST));
		
		/* ep2 reset */
		/* no DMA, don't need to flush it */
		/* clear FST... ugh */
		loopvar=10000;
		do {
			Ser0UDCCS2 &= ~(UDCCS2_FST);
			if (loopvar-- <= 0) {
				putstr("timeout clearing 2\r\n");
				return;
			};
			msleep(1);
		} while(Ser0UDCCS2 & (UDCCS2_FST));
	} else {
		CHECKDRAW("next reset state error");
	};
	
	Ser0UDCCR |= UDCCR_REM;
	msleep(15);
	Ser0UDCCR &= ~(UDCCR_SUSIM | UDCCR_RESIM);
	msleep(15);
	loopvar=10000;
	Ser0UDCSR=status;
	do {
		Ser0UDCSR = status;
		if (loopvar-- <= 0) {
			CHECKDRAW("timeout flipping status");
			return;
		};
		msleep(1);
	} while(Ser0UDCSR & status);
//	command_dma_init(0,0);
};

void usb_poll()
{
	int loopvar;
	/* do one poll */
	u32 status = 0xFFFFFFFF; /* loop at least once */
	
//	draw_text("P", 0, 320-40, 0);
	
	while (status)
	{
		status=Ser0UDCSR;
		
		if (status & UDCSR_RSTIR)
		{
			do_reset(status);
			continue;
		};
		
		loopvar=10000;
		do {
			Ser0UDCCR &= ~UDCCR_REM;
			if (loopvar-- <= 0) {
				CHECKDRAW("timeout clearing reset");
				return;
			};
			msleep(1);
		} while(Ser0UDCCR & UDCCR_REM);
		
		if (status & UDCSR_RESIR)
			do_resume();
		if (status & UDCSR_SUSIR)
			do_suspend();
		
		loopvar=10000;
		Ser0UDCSR=status;
		do {
			Ser0UDCSR = status;
			if (loopvar-- <= 0) {
				CHECKDRAW("timeout flipping status");
				return;
			};
			msleep(1);
		} while(Ser0UDCSR & status);
		
		if (status & UDCSR_TIR)
			send_int();
		if (status & UDCSR_RIR)
			do_recv(fifomode); // do it NOW, and do an echo
		if (status & UDCSR_EIR)
			do_ep0();
			
		status = Ser0UDCSR;
	};
	
//	draw_text(" ", 0, 320-40, 0);
		
};

void command_usb_loop(int argc, const char** argv)
{
	u32 status = Ser0UDCSR;
	int loop = 10000;
	int loopvar;
	char wbuf[9];
	
	if (!usb_initted)
	{
		putstr("USB not initialized! run usbinit first!\r\n");
		return;
	}
	if (usb_initted == 2)
	{
		putstr("previous failure initting USB. run usbinit again to try again.\r\n");
		return;
	};

	draw_text("L", 0, 320-48, 0);

	draw_text ("USB: Looping", 0, 0, 0);
	
	while (loop >= 0)
	{
		loop--;
		
//		usb_poll();
		if (ICPR & 0x2000)
			draw_text("U", 0, 320-80, 0);
		else
			draw_text("_", 0, 320-80, 0);
		msleep(1); /* interrupts'll handle it */
	};
	draw_text ("USB: Done   ", 0, 0, 0);
	draw_text(" ", 0, 320-48, 0);
	do_send("\r\n\r\ndone\r\n", 10);
};

void command_usb_serial(int argc, const char** argv)
{
	u32 status = Ser0UDCSR;
	int loop = 5000;
	int loopvar;
	char wbuf[9];
	
	if (!usb_initted)
	{
		putstr("USB not initialized! run usbinit first!\r\n");
		return;
	}
	if (usb_initted == 2)
	{
		putstr("previous failure initting USB. run usbinit again to try again.\r\n");
		return;
	};
	
	fifomode = 0;

	oldser = serial;
	serial=&usb_serial;
};

static void set_cs_bits( u32 bits );


int usb_nextstate(int event)
{
	int next_state = device_state_machine[sm_state][event];
	if (next_state != kError)
	{
		int next_device_state = sm_state_to_device_state[next_state];
		sm_state = next_state;
		if (usbstate != next_device_state)
		{
			/* configured callback blah blah */
			usbstate = next_device_state;
			/* endpoint change state - empty */
		};
		
	} else
		CHECKDRAW("next state is error!");
	
	return next_state;
};

static inline void enable_suspend_mask_resume(void)
{
         int i = 1;
         while( 1 ) {
                  Ser0UDCCR |= UDCCR_RESIM; // mask future resume events
                  msleep( 1 );
                  if ( (Ser0UDCCR & UDCCR_RESIM) || (Ser0UDCSR & UDCSR_RSTIR) )
                           break;
                  if ( ++i == 50 ) {
                           CHECKDRAW("couldn't set resim");
                           break;
                  }
         }
         i = 1;
         while( 1 ) {
                  Ser0UDCCR &= ~UDCCR_SUSIM;
                  msleep( 1 );
                  if ( (!( Ser0UDCCR & UDCCR_SUSIM ))
                           ||
                           (Ser0UDCSR & UDCSR_RSTIR)
                         )
                           break;
                  if ( ++i == 50 ) {
                           CHECKDRAW("couldn't set susim");
                           break;
                  }
         }
}

static inline void enable_resume_mask_suspend( void )
{
         int i = 1;

         while( 1 ) {
                  Ser0UDCCR |= UDCCR_SUSIM; // mask future suspend events
                  msleep( i );
                  if ( (Ser0UDCCR & UDCCR_SUSIM) || (Ser0UDCSR & UDCSR_RSTIR) )
                           break;
                  if ( ++i == 50 ) {
                           CHECKDRAW("couldn't set susim");
                           break;
                  }
         }

         i = 1;
         while( 1 ) {
                  Ser0UDCCR &= ~UDCCR_RESIM;
                  msleep( i );
                  if ( ( Ser0UDCCR & UDCCR_RESIM ) == 0
                           ||
                           (Ser0UDCSR & UDCSR_RSTIR)
                         )
                           break;
                  if ( ++i == 50 ) {
                           CHECKDRAW("couldn't set resim");
                           break;
                  }
         }
}

//XXX choke;

/* USB Device Requests */
typedef struct
{
    u8 bmRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} usb_dev_request_t  __attribute__ ((packed));

/* Request Codes   */
enum { GET_STATUS=0,         CLEAR_FEATURE=1,     SET_FEATURE=3,
           SET_ADDRESS=5,        GET_DESCRIPTOR=6,        SET_DESCRIPTOR=7,
                      GET_CONFIGURATION=8,  SET_CONFIGURATION=9, GET_INTERFACE=10,
                                 SET_INTERFACE=11 };
static void get_descriptor( usb_dev_request_t * pReq );
static string_desc_t_ sd_zero;
static string_desc_t sd_vend = {
	USB_DESC_STRING,
	sizeof(sd_vend),
	{'h','a','n','d','h','e','l','d','s','.','o','r','g'}
};

static string_desc_t sd_prod = {
	USB_DESC_STRING,
	sizeof(string_desc_t),
	"handhelds.org opensource bootldr"
};

enum { kTargetDevice=0, kTargetInterface=1, kTargetEndpoint=2 };
static u32  queue_and_start_write( void * in, int req, int act );
static inline int windex_to_ep_num( u16 w ) { return (int) ( w & 0x000F); }
inline int type_code_from_request( u8 by ) { return (( by >> 4 ) & 3); }

static int read_fifo( usb_dev_request_t * request );
static void sh_write_with_empty_packet( void );
static void sh_write( void );

static struct {
		unsigned char *p;
		int bytes_left;
} wr;

static void
sh_setup_begin( void )
{
	 unsigned char status_buf[2];  /* returned in GET_STATUS */
	 usb_dev_request_t req;
	 int request_type;
	 int n;
	 u32 cs_bits;
	 u32 cs_reg_in = Ser0UDCCS0;
	int loopvar;
	
	 if (cs_reg_in & UDCCS0_SST) {
		  CHECKDRAW( "sent stall");
		  set_cs_bits( UDCCS0_SST );
	}

	 if ( cs_reg_in & UDCCS0_SE ) {
		  CHECKDRAW( "Early term of setup" );
		  set_cs_bits( UDCCS0_SSE );  		 /* clear setup end */
	 }

	 /* Be sure out packet ready, otherwise something is wrong */
	 if ( (cs_reg_in & UDCCS0_OPR) == 0 ) {
		  /* we can get here early...if so, we'll int again in a moment  */
		  CHECKDRAW( "no OUT packet avail. exiting" );
		  goto sh_sb_end;
	 }

	 /* read the setup request */
	 n = read_fifo( &req );
	 if ( n != sizeof( req ) ) {
		  CHECKDRAW( "fifo READ ERROR, not enough bytes,"
				  " Stalling out...");
		  /* force stall, serviced out */
		  set_cs_bits( UDCCS0_FST | UDCCS0_SO  );
		  goto sh_sb_end;
	 }

	 /* Is it a standard request? (not vendor or class request) */
	 request_type = type_code_from_request( req.bmRequestType );
	 if ( request_type != 0 ) {
		  CHECKDRAW( "PocketPC init sequence detected - starting DMA here");
//		  command_dma_init(0,0);
		  set_cs_bits( UDCCS0_DE | UDCCS0_SO );
		  goto sh_sb_end;
	 }

	 /* Handle it */
	 switch( req.bRequest ) {

		  /* This first bunch have no data phase */

	 case SET_ADDRESS:
		  address = (u32) (req.wValue & 0x7F);
		  /* when SO and DE sent, UDC will enter status phase and ack,
			 ..propagating new address to udc core. Next control transfer
			 ..will be on the new address. You can't see the change in a
			 ..read back of CAR until then. (about 250us later, on my box).
			 ..The original Intel driver sets S0 and DE and code to check
			 ..that address has propagated here. I tried this, but it
			 ..would only work sometimes! The rest of the time it would
			 ..never propagate and we'd spin forever. So now I just set
			 ..it and pray...
		  */
		  Ser0UDCAR = address;
		  CHECKDRAW("kEvAddress");
		  usb_nextstate( kEvAddress );
		  set_cs_bits( UDCCS0_SO | UDCCS0_DE );  /* no data phase */
		  {
		  	char buf[3];
		  	binarytohex(buf, address, 1);
		  	draw_text(buf, 0, 320-16, 0);
		  }
		  break;


	 case SET_CONFIGURATION:
	 	CHECKDRAW("SET_CONFIGURATION");
		  if ( req.wValue == 1 ) {
			   /* configured */
			   if (usb_nextstate( kEvConfig ) != kError){
					Ser0UDCOMP = 0xFF;
					Ser0UDCIMP = 0xFF;
			   } else {
			   	CHECKDRAW("bad time to config!");
			   };
		  } else if ( req.wValue == 0 ) {
			   /* de-configured */
			   if (usb_nextstate( kEvDeConfig ) != kError )
					;
			   else
			   	CHECKDRAW("kEvDeConfig error");
		  } else
		  	CHECKDRAW("setup phase: unknown setconfig data");
		  set_cs_bits( UDCCS0_SO | UDCCS0_DE );  /* no data phase */
		  break;

	 case CLEAR_FEATURE:
		  /* could check data length, direction...26Jan01ww */
		  CHECKDRAW("CLEAR_FEAT");
		  if ( req.wValue == 0 ) { /* clearing ENDPOINT_HALT/STALL */
			   int ep = windex_to_ep_num( req.wIndex );
			   if ( ep == 1 ) {
			   		CHECKDRAW("clear feature ep halt on recvr");
			   		/* ep1 reset */
					/* no DMA, don't need to flush it */
					/* clear FST... ugh */
					loopvar=10000;
					do {
						Ser0UDCCS1 &= ~(UDCCS1_FST);
						if (loopvar-- <= 0) {
							putstr("timeout clearing 1\r\n");
							return;
						};
						msleep(1);
					} while(Ser0UDCCS1 & (UDCCS1_FST));
			   }
			   else if ( ep == 2 ) {
			   		CHECKDRAW("clear feature ep halt on xmitr");
			   		/* ep2 reset */
					/* no DMA, don't need to flush it */
					/* clear FST... ugh */
					loopvar=10000;
					do {
						Ser0UDCCS2 &= ~(UDCCS2_FST);
						if (loopvar-- <= 0) {
							putstr("timeout clearing 2\r\n");
							return;
						};
						msleep(1);
					} while(Ser0UDCCS2 & (UDCCS2_FST));
			   } else {
			   		CHECKDRAW("clear feature ep halt on bad ep");
			   }
		  } else {
		  	CHECKDRAW("unsupported clearfeature");
		  }
		  set_cs_bits( UDCCS0_SO | UDCCS0_DE );  /* no data phase */
		  break;

	 case SET_FEATURE:
	 	CHECKDRAW("SET_FEAT");
		  if ( req.wValue == 0 ) { /* setting ENDPOINT_HALT/STALL */
			   int ep = windex_to_ep_num( req.wValue );
			   if ( ep == 1 ) {
			   		CHECKDRAW("set feature ep halt on recvr");
			   		loopvar=10000;
					do {
						Ser0UDCCS1 |= (UDCCS1_FST);
						if (loopvar-- <= 0) {
							putstr("timeout setting 1\r\n");
							return;
						};
						msleep(1);
					} while(!(Ser0UDCCS1 & (UDCCS1_FST)));
			   }
			   else if ( ep == 2 ) {
			   		CHECKDRAW("set feature ep halt on recvr");
			   		loopvar=10000;
					do {
						Ser0UDCCS2 |= (UDCCS2_FST);
						if (loopvar-- <= 0) {
							putstr("timeout setting 2\r\n");
							return;
						};
						msleep(1);
					} while(!(Ser0UDCCS2 & (UDCCS2_FST)));
			   } else {
			   		CHECKDRAW("set feature ep halt on bad ep");
			   }
		  }
		  else {
	   		CHECKDRAW("bad setfeature");
		  }
		  set_cs_bits( UDCCS0_SO | UDCCS0_DE );  /* no data phase */
		  break;


		  /* The rest have a data phase that writes back to the host */
	 case GET_STATUS:
	 	CHECKDRAW("GET_STATUS");
		  /* return status bit flags */
		  status_buf[0] = status_buf[1] = 0;
		  n = (int) ( req.bmRequestType & 0x0F);
		  switch( n ) {
		  case kTargetDevice:
			status_buf[0] |= 1;
			   break;
		  case kTargetInterface:
			   break;
		  case kTargetEndpoint:
			   /* return stalled bit */
			   n = (int) ( req.wIndex & 0x000F);
			   if ( n == 1 )
					status_buf[0] |= (Ser0UDCCS1 & UDCCS1_FST) >> 4;
			   else if ( n == 2 )
					status_buf[0] |= (Ser0UDCCS2 & UDCCS2_FST) >> 5;
			   else {
			   	CHECKDRAW("unknown ep in getstat");
			   }
			   break;
		  default:
			   CHECKDRAW( "unknown tgt in GETSTAT");
			   /* fall thru */
			   break;
		  }
		  cs_bits  = queue_and_start_write( status_buf,
											req.wLength,
											sizeof( status_buf ) );
		  set_cs_bits( cs_bits );
		  break;
	 case GET_DESCRIPTOR:
	 	CHECKDRAW("GET_DESCR");
		  get_descriptor( &req );
		  break;

	 case GET_CONFIGURATION:
	 	CHECKDRAW("GET_CONFIG");
		  status_buf[0] = (usbstate ==  USB_STATE_CONFIGURED)
			   ? 1
			   : 0;
		  cs_bits = queue_and_start_write( status_buf, req.wLength, 1 );
		  set_cs_bits( cs_bits );
		  break;
	 case GET_INTERFACE:
	 	  CHECKDRAW("getint not supported");
		  cs_bits = queue_and_start_write( NULL, req.wLength, 0 );
		  set_cs_bits( cs_bits );
		  break;
	 case SET_INTERFACE:
	 	  CHECKDRAW("setint not supported");
		  set_cs_bits( UDCCS0_DE | UDCCS0_SO );
		  break;
	 default :
	 	  CHECKDRAW("unknown request");
		  break;
	 } /* switch( bRequest ) */

sh_sb_end:
	 return;

}



#define ABORT_BITS ( UDCCS0_SST | UDCCS0_SE )
#define OK_TO_WRITE (!( Ser0UDCCS0 & ABORT_BITS ))
#define BOTH_BITS (UDCCS0_IPR | UDCCS0_DE)

static void set_ipr();
static void set_ipr_and_de();
static void set_de();

static void set_cs_bits( u32 bits )
{
	 if ( bits & ( UDCCS0_SO | UDCCS0_SSE | UDCCS0_FST ) )
		  Ser0UDCCS0 = bits;
	 else if ( (bits & BOTH_BITS) == BOTH_BITS )
		  set_ipr_and_de();
	 else if ( bits & UDCCS0_IPR )
		  set_ipr();
	 else if ( bits & UDCCS0_DE )
		  set_de();
}

static void set_de( void )
{
	 int i = 1;
	 while( 1 ) {
		  if ( OK_TO_WRITE ) {
				Ser0UDCCS0 |= UDCCS0_DE;
		  } else {
			   CHECKDRAW( "quitting setde b/c SST/SE set");
			   break;
		  }
		  if ( Ser0UDCCS0 & UDCCS0_DE )
			   break;
		  oscrsleep( i );
		  if ( ++i == 50  ) {
		  	   CHECKDRAW("can't set de (MEEP)");
			   break;
		  }
	 }
}

static void set_ipr( void )
{
	 int i = 1;
	 while( 1 ) {
		  if ( OK_TO_WRITE ) {
				Ser0UDCCS0 |= UDCCS0_IPR;
		  } else {
//			   CHECKDRAW( "quitting setipr b/c SST/SE set");
			   break;
		  }
		  if ( Ser0UDCCS0 & UDCCS0_IPR )
			   break;
		  oscrsleep( i );
		  if ( ++i == 50  ) {
			   CHECKDRAW( "can't set ipr (MEEP)");
			   break;
		  }
	 }
}



static void set_ipr_and_de( void )
{
	 int i = 1;
	 while( 1 ) {
		  if ( OK_TO_WRITE ) {
			   Ser0UDCCS0 |= BOTH_BITS;
		  } else {
			   CHECKDRAW( "quitting setiprde b/c SST/SE");
			   break;
		  }
		  if ( (Ser0UDCCS0 & BOTH_BITS) == BOTH_BITS)
			   break;
		  msleep( i );
		  if ( ++i == 50  ) {
			   CHECKDRAW( "can't set iprde (MEEP)");
			   break;
		  }
	 }
}

static int clear_opr( void )
{
	 int i = 10000;
	 int is_clear;
	 do {
		  Ser0UDCCS0 = UDCCS0_SO;
		  is_clear  = ! ( Ser0UDCCS0 & UDCCS0_OPR );
		  if ( i-- <= 0 ) {
			   CHECKDRAW( "clear_opr() failed\n");
			   break;
		  }
	 } while( ! is_clear );
	 return is_clear;
}


/*
 * read_fifo()
 * Read 1-8 bytes out of FIFO and put in request.
 * Called to do the initial read of setup requests
 * from the host. Return number of bytes read.
 *
 * Like write fifo above, this driver uses multiple
 * reads checked agains the count register with an
 * overall timeout.
 *
 */
static int
read_fifo( usb_dev_request_t * request )
{
	int bytes_read = 0;
	int fifo_count;
	int i;

	unsigned char * pOut = (unsigned char*) request;

	fifo_count = ( Ser0UDCWC & 0xFF );

	if (fifo_count > 8)
	{
		CHECKDRAW("ASSERTION FAIL, fifo_count > 8");
		return 0;
	};

	while( fifo_count-- ) {
		 i = 0;
		 do {
			  *pOut = (unsigned char) Ser0UDCD0;
			  msleep( 1 );
		 } while( ( Ser0UDCWC & 0xFF ) != fifo_count && i < 100 );
		 if ( i == 100 ) {
		 	CHECKDRAW("read_fifo: read failure :(");
		 }
		 pOut++;
		 bytes_read++;
	}

	return bytes_read;
}

/*
 * write_fifo()
 * Stick bytes in the 8 bytes endpoint zero FIFO.
 * This version uses a variety of tricks to make sure the bytes
 * are written correctly. 1. The count register is checked to
 * see if the byte went in, and the write is attempted again
 * if not. 2. An overall counter is used to break out so we
 * don't hang in those (rare) cases where the UDC reverses
 * direction of the FIFO underneath us without notification
 * (in response to host aborting a setup transaction early).
 *
 */
static void write_fifo( void )
{
	int bytes_this_time = ( wr.bytes_left <  8 ) ? wr.bytes_left : 8;
	int bytes_written = 0;
	int i=0;


	oscrsleep(1);	 /* voodoo */

	while( bytes_this_time-- ) {
		 i = 0;
		 do {
			  Ser0UDCD0 = *wr.p;
			  oscrsleep(1);
			  i++;
		 } while( (Ser0UDCWC == bytes_written) && (i < 1000) );
		 if ( i == 1000 ) {
			  CHECKDRAW("write_fifo: write failure!");
			  CHECKDRAW("the UDC most likely locked up. sorry.");
		 }

		 wr.p++;
		 bytes_written++;
	}
	wr.bytes_left -= bytes_written;

	/* following propagation voodo so maybe caller writing IPR in
	   ..a moment might actually get it to stick 28Feb01ww */
	oscrsleep( 1 );

}

/*
 * queue_and_start_write()
 * p == data to send
 * req == bytes host requested
 * act == bytes we actually have
 * Returns: bits to be flipped in ep0 control/status register
 *
 * Called from sh_setup_begin() to begin a data return phase. Sets up the
 * global "wr"-ite structure and load the outbound FIFO with data.
 * If can't send all the data, set appropriate handler for next interrupt.
 *
 */
static u32  queue_and_start_write( void * in, int req, int act )
{
	 u32 cs_reg_bits = UDCCS0_IPR;
	 unsigned char * p = (unsigned char*) in;

//	 PRINTKD( "Qr=%d a=%d\n",req,act );

	/* thou shalt not enter data phase until the serviced OUT is clear */
	 if ( ! clear_opr() ) {
//		  printk( "%sSO did not clear OPR\n", pszMe );
		CHECKDRAW("so didn't clear opr");
		  return ( UDCCS0_DE | UDCCS0_SO ) ;
	 }
	 wr.p = p;
	 wr.bytes_left = ( act < req ) ? act : req;

	 write_fifo();

	 if ( 0 == wr.bytes_left ) {
		  cs_reg_bits |= UDCCS0_DE;	/* out in 1 so data end */
		  wr.p = NULL;  				/* be anal */
		  CHECKDRAW("checkz0r, 0 bytes left");
	 }
	 else if ( act < req )   /* we are going to short-change host */
		  current_handler = sh_write_with_empty_packet; /* so need nul to not stall */
	 else /* we have as much or more than requested */
		  current_handler = sh_write;

	CHECKDRAW("queue_and_start_write done");

	 return cs_reg_bits; /* note: IPR was set uncondtionally at start of routine */
}

static void sh_write()
{
	 if ( Ser0UDCCS0 & UDCCS0_IPR ) {
		 CHECKDRAW( "sh_write(): IPR set, exiting");
		  return;
	 }

//	CHECKDRAW("sh_write");

	 /* If bytes left is zero, we are coming in on the
		..interrupt after the last packet went out. And
		..we know we don't have to empty packet this transfer
		..so just set DE and we are done */

	 if ( 0 == wr.bytes_left ) {
		  /* that's it, so data end  */
		  set_de();
		  wr.p = NULL;  				/* be anal */
		  current_handler = sh_setup_begin;
	 } else {
		  /* Otherwise, more data to go */
		  write_fifo();
		  set_ipr();
	 }
}
/*
 * sh_write_with_empty_packet()
 * This is the setup handler when we don't have enough data to
 * satisfy the host's request. After we send everything we've got
 * we must send an empty packet (by setting IPR and DE) so the
 * host can perform "short packet retirement" and not stall.
 *
 */
static void sh_write_with_empty_packet( void )
{
	u32 cs_reg_out = 0;
	 

	 if ( Ser0UDCCS0 & UDCCS0_IPR ) {
		 CHECKDRAW( "sh_write(): IPR set, exiting");
		  return;
	 }

	 /* If bytes left is zero, we are coming in on the
		..interrupt after the last packet went out.
		..we must do short packet suff, so set DE and IPR
	 */

	 if ( 0 == wr.bytes_left ) {
		  set_ipr_and_de();
		  wr.p = NULL;
		  current_handler = sh_setup_begin;
		 CHECKDRAW( "sh_write empty() Sent empty packet \n");
	 }  else {
	 	CHECKDRAW("write fifo");
		  write_fifo();				/* send data */
		  set_ipr();				/* flag a packet is ready */
		  CHECKDRAW("wrote fifo");
	 }
	 Ser0UDCCS0 = cs_reg_out;
//	 CHECKDRAW( "sh_write_with_empty_packet" );
}

/* setup default descriptors */

static void
initialize_descriptors(void)
{
	desc.dev.bLength               = sizeof( device_desc_t );
	desc.dev.bDescriptorType       = USB_DESC_DEVICE;
	desc.dev.bcdUSB                = 0x100; /* 1.0 */
	desc.dev.bDeviceClass          = 0xFF;	/* vendor specific */
	desc.dev.bDeviceSubClass       = 0;
	desc.dev.bDeviceProtocol       = 0;
	desc.dev.bMaxPacketSize0       = 8;	/* ep0 max fifo size */
	desc.dev.idVendor              = 0x49f;		/* vid compaq */
	desc.dev.idProduct             = 0x3; 		/* did pocketpc */
	desc.dev.bcdDevice             = 0x1337; /* vendor assigned device release num */
	desc.dev.iManufacturer         = 1;	/* index of manufacturer string */
	desc.dev.iProduct              = 2; /* index of product description string */
	desc.dev.iSerialNumber         = 0;	/* index of string holding product s/n */
	desc.dev.bNumConfigurations    = 1;

	desc.b.cfg.bLength             = sizeof( config_desc_t );
	desc.b.cfg.bDescriptorType     = USB_DESC_CONFIG;
	desc.b.cfg.wTotalLength        = sizeof(struct cdb);
	desc.b.cfg.bNumInterfaces      = 1;
	desc.b.cfg.bConfigurationValue = 1;
	desc.b.cfg.iConfiguration      = 0;
	
	desc.b.cfg.bmAttributes        = USB_CONFIG_BUSPOWERED;
	desc.b.cfg.MaxPower            = USB_POWER( 500 );

	desc.b.intf.bLength            = sizeof( intf_desc_t );
	desc.b.intf.bDescriptorType    = USB_DESC_INTERFACE;
	desc.b.intf.bInterfaceNumber   = 0; /* unique intf index*/
	desc.b.intf.bAlternateSetting  = 0;
	desc.b.intf.bNumEndpoints      = 2;
	desc.b.intf.bInterfaceClass    = 0xFF; /* vendor specific */
	desc.b.intf.bInterfaceSubClass = 0;
	desc.b.intf.bInterfaceProtocol = 0;
	desc.b.intf.iInterface         = 0;

	desc.b.ep1.bLength             = sizeof( ep_desc_t );
	desc.b.ep1.bDescriptorType     = USB_DESC_ENDPOINT;
	desc.b.ep1.bEndpointAddress    = USB_EP_ADDRESS( 1, USB_OUT );
	desc.b.ep1.bmAttributes        = USB_EP_BULK;
	desc.b.ep1.wMaxPacketSize      = 256;
	desc.b.ep1.bInterval           = 0;

	desc.b.ep2.bLength             = sizeof( ep_desc_t );
	desc.b.ep2.bDescriptorType     = USB_DESC_ENDPOINT;
	desc.b.ep2.bEndpointAddress    = USB_EP_ADDRESS( 2, USB_IN );
	desc.b.ep2.bmAttributes        = USB_EP_BULK;
	desc.b.ep2.wMaxPacketSize      = 256;
	desc.b.ep2.bInterval           = 0;

	/* set language */
	/* See: http://www.usb.org/developers/data/USB_LANGIDs.pdf */
	sd_zero.bDescriptorType = USB_DESC_STRING;
	sd_zero.bLength         = sizeof( string_desc_t_ );
	sd_zero.bString[0]      = 0x409; /* American English */
	
}
/*
 * get_descriptor()
 * Called from sh_setup_begin to handle data return
 * for a GET_DESCRIPTOR setup request.
 */
static void get_descriptor( usb_dev_request_t * pReq )
{
	 u32 cs_bits = 0;
	 string_desc_t * pString;
	 ep_desc_t * pEndpoint;

	 desc_t * pDesc = &desc;
	 int type = pReq->wValue >> 8;
	 int idx  = pReq->wValue & 0xFF;

	 switch( type ) {
	 case USB_DESC_DEVICE:
	 	CHECKDRAW("DESC_DEVICE");
		  cs_bits =
			   queue_and_start_write( &pDesc->dev,
									  pReq->wLength,
									  pDesc->dev.bLength );
		  break;

		  // return config descriptor buffer, cfg, intf, 2 ep
	 case USB_DESC_CONFIG:
	 	CHECKDRAW("DESC_CONFIG");
		  cs_bits =
			   queue_and_start_write( &pDesc->b,
									  pReq->wLength,
									  sizeof( struct cdb ) );
		  break;

		  // not quite right, since doesn't do language code checking
	 case USB_DESC_STRING:
	 	  switch (idx)
	 	  {
	 	  case 0:
	 	  	CHECKDRAW("zero");
	 	  	pString = &sd_zero;
	 	  	break;
	 	  case 1:
	 	  	CHECKDRAW("vend");
	 	  	pString = &sd_vend;
	 	  	break;
	 	  case 2:
	 	  	CHECKDRAW("prod");
	 	  	pString = &sd_prod;
	 	  	break;
	 	  default:
	 	  	CHECKDRAW("NULL");
	 	  	pString = NULL;
	 	  	break;
	 	  };
		  if ( pString ) {
			   cs_bits =
					queue_and_start_write( pString,
										   pReq->wLength,
										   pString->bLength );
		  }
		  else {
			CHECKDRAW("unknown string index");
			   cs_bits = ( UDCCS0_DE | UDCCS0_SO | UDCCS0_FST );
		  }
		  break;

	 case USB_DESC_INTERFACE:
	 	CHECKDRAW("IF");
		  if ( idx == pDesc->b.intf.bInterfaceNumber ) {
			   cs_bits =
					queue_and_start_write( &pDesc->b.intf,
										   pReq->wLength,
										   pDesc->b.intf.bLength );
		  }
		  break;

	 case USB_DESC_ENDPOINT: /* correct? 21Feb01ww */
	 	CHECKDRAW("EP");
		  if ( idx == 1 )
			   pEndpoint = &pDesc->b.ep1;
		  else if ( idx == 2 )
			   pEndpoint = &pDesc->b.ep2;
		  else
			   pEndpoint = NULL;
		  if ( pEndpoint ) {
			   cs_bits =
					queue_and_start_write( pEndpoint,
										   pReq->wLength,
										   pEndpoint->bLength );
		  } else {
			CHECKDRAW("unknown ep index, stall\n");
			   cs_bits = ( UDCCS0_DE | UDCCS0_SO | UDCCS0_FST );
		  }
		  break;


	 default :
		CHECKDRAW("unknown desctype, stall\n");
		  cs_bits = ( UDCCS0_DE | UDCCS0_SO | UDCCS0_FST );
		  break;

	 }
	 set_cs_bits( cs_bits );
}

int usb_haschars(struct serial_device *sd)
{
//	usb_poll();

	/* *grumble* */
	usb_drainuart(0);
	
	if (fifobytes)
		return 1;
	else
		return 0;
};
unsigned char usb_readchar(struct serial_device *sd)
{
	char r;
	int i;
	
//	usb_poll();
	if (!fifobytes)
	{
		CHECKDRAW("wanted bytes we didn't have");
		return 0;
	};
	
	
	r=biggerfifo[0];

	for (i = 1; i < fifobytes; i++)
		biggerfifo[i-1] = biggerfifo[i];

	fifop--;
	fifobytes--;
	return r;
};

void usb_writechar(struct serial_device *sd, unsigned char c)
{
#if 0				/* Immediate mode is SLOW SLOW SLOW */
	do_send(&c,1);
#else
	xmitfifo[xmitbytes] = c;
	xmitbytes++;
	if (xmitbytes == MAXBYTES)
		usb_drainuart(0);
#endif	
};


void usb_drainuart(struct serial_device *sd)
{
	if (!xmitbytes)
		return;
	do_send(xmitfifo, xmitbytes);

	/* Since we can't actually detect when the host gets around to
	 * sending a Bulk-IN request, we have to do some oscrsleep
	 * voodoo. I'll set it conservatively to 400 or so, but 150
	 * should work for users who have fast CPUs. Email me at
	 * joshua@handhelds.org with what setting you can use, what
	 * kernel version you have on the host, what CPU speed on the
	 * host, and I will add you in. Kthx. 
	 */
	 
	/* Let's try something a little different... Joshua, 04-06-2003 */
	oscrsleep(/* 400 */ xmitbytes*4);
	
	xmitbytes=0;
}

void usb_int()
{
	if (!usb_initted)
		return;
	
	usb_poll(); /* NOW NOW NOW */
};
