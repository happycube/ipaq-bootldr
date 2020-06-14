/* dma.c
 * DMA support for the handhelds.org bootldr to provide DMA for USB
 * handhelds.org open bootldr
 *
 * code that is unclaimed by others is
 *    Copyright (C) 2003 Joshua Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "commands.h"
#include "bootldr.h"
#include "serial.h"
#include "bootconfig.h"

#ifndef CONFIG_ACCEPT_GPL
#  error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#define DMAVERSION "0.9"

#define DMA_MAX	     0x1000

#define DMA_BASE     0xB0000000
#define DMA_BASE2    0xB0000020
#define DMA_DDAR     0x00000000
#define DMA_DCSR_SET 0x00000004
#define DMA_DCSR_CLR 0x00000008
#define DMA_DCSR_RD  0x0000000C
#define DMA_DBSA     0x00000010
#define DMA_DBTA     0x00000014
#define DMA_DBSB     0x00000018
#define DMA_DBTB     0x0000001C

#define DCSR_RUN     0x00000001
#define DCSR_IE      0x00000002
#define DCSR_ERROR   0x00000004
#define DCSR_DONEA   0x00000008
#define DCSR_STARTA  0x00000010
#define DCSR_DONEB   0x00000020
#define DCSR_STARTB  0x00000040
#define DCSR_BIU     0x00000080

#define DMA_UDCREAD  0x80000A15
#define DMA_UDCWRITE 0x80000A04

#define REG(x)       (*((volatile u32 *)(x)))

static int dma_isrunning = 0;

unsigned char dmabuf[DMA_MAX];

SUBCOMMAND(dma, init, command_dma_init, "-- starts dma", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(dma, poll, command_dma_poll, "-- polls once on DMA", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(dma, version, command_dma_version, "-- does a dma version check", BB_RUN_FROM_RAM, 1);

void command_dma_version(int argc, const char** argv)
{
	putstr("version " DMAVERSION "\r\n");
};

void dma_int()
{
	int status = REG(DMA_BASE+DMA_DCSR_RD);
	int transferred;
	
	REG(DMA_BASE+DMA_DCSR_CLR) = 0x01;
	REG(DMA_BASE+DMA_DCSR_SET) = 0x01;
	
	if (status & DCSR_DONEA)
	{
		CHECKDRAW("DMA from buffer A is done! *ding!*");
		transferred = DMA_MAX - REG(DMA_BASE + DMA_DBTA);
		command_dma_init(0,0);
	};
	
	if (status & DCSR_DONEB)
	{
		CHECKDRAW("DMA from buffer B is done, but there is no buffer B. *poof!*");
		transferred = DMA_MAX - REG(DMA_BASE + DMA_DBTB);
		command_dma_init(0,0);
	};
	
	if (status & DCSR_ERROR)
	{
		CHECKDRAW("DMA error! *wh00nk*");
	};
	
	REG(DMA_BASE+DMA_DCSR_CLR) = DCSR_DONEA | DCSR_DONEB | DCSR_ERROR;
};

void command_dma_poll(int argc, const char** argv)
{
	dma_int();
};

void command_dma_isrunning(int argc, const char** argv)
{
	if (dma_isrunning)
		CHECKDRAW("yes, it's running.");
	else
		CHECKDRAW("no, it's not running.");
};

void command_dma_init(int argc, const char** argv)
{
	int curbuf;
	unsigned int buf = dmabuf;
	
	dma_isrunning = 1;
	
	curbuf = REG(DMA_BASE+DMA_DCSR_RD) & DCSR_BIU;
	
	if ((unsigned int)buf < 0xC0000000)
		buf += FLASH_IMG_START;
	
	REG(DMA_BASE+DMA_DCSR_CLR) = 0x7F;
	
	REG(DMA_BASE+DMA_DDAR) = DMA_UDCREAD;
	
	if (!curbuf) {
		REG(DMA_BASE+DMA_DBTA) = DMA_MAX;
		REG(DMA_BASE+DMA_DBSA) = (unsigned int)buf;
	} else {
		REG(DMA_BASE+DMA_DBTB) = DMA_MAX;
		REG(DMA_BASE+DMA_DBTB) = (unsigned int)buf;
	};	
	
	REG(DMA_BASE+DMA_DCSR_SET) = DCSR_RUN | DCSR_IE | (curbuf ? DCSR_STARTB : DCSR_STARTA);
};

extern void CHECKDRAW(char*);
extern void oscrsleep(int i);

int dma_startsend(unsigned char* buf, int count)
{
	/* start a DMA send over DMA_BASE2 */
	int curbuf;
	
	if ((unsigned int)buf < 0xC0000000)
		buf += FLASH_IMG_START;
	
	curbuf = 0;
	while (REG(DMA_BASE2+DMA_DCSR_RD) & DCSR_RUN)
	{
		curbuf++;
		oscrsleep(1);
		if (curbuf == 5000)
		{
			CHECKDRAW(__FILE__ ": " __FUNCTION__ ": timed out waiting for DCSR_RUN to become 0.");
			break;
		};
	};
	
	curbuf = REG(DMA_BASE2+DMA_DCSR_RD) & DCSR_BIU;
	
	REG(DMA_BASE2+DMA_DCSR_CLR) = 0x7F;
	REG(DMA_BASE2+DMA_DDAR) = DMA_UDCWRITE;

	if (!curbuf) {
		REG(DMA_BASE2+DMA_DBTA) = count;
		REG(DMA_BASE2+DMA_DBSA) = (unsigned int)buf;
	} else {
		REG(DMA_BASE2+DMA_DBTB) = count;
		REG(DMA_BASE2+DMA_DBSB) = (unsigned int)buf;
	};
	
	// go go go!
	REG(DMA_BASE2+DMA_DCSR_SET) = DCSR_RUN | DCSR_IE | (curbuf ? DCSR_STARTB : DCSR_STARTA);
	return count;
};

int dma_issendrunning()
{	
	return REG(DMA_BASE2+DMA_DCSR_RD) & DCSR_RUN;
};

void dma_sendint()
{
	REG(DMA_BASE2+DMA_DCSR_CLR) = 0x7F;
};

int dma_grabbytes(unsigned char* loc)
{
	char wbuf[4];
	
	int curbuf = REG(DMA_BASE+DMA_DCSR_RD) & DCSR_BIU;
	int transferred;
	
	transferred = DMA_MAX - REG(DMA_BASE + (curbuf ? DMA_DBTB : DMA_DBTA));
	
	CHECKDRAW("DMA");
	binarytohex(wbuf, transferred, 1);
	wbuf[3] = '\0';
//	CHECKDRAW(wbuf);
	memcpy(loc, dmabuf, transferred);
	
//	CHECKDRAW("init");
	command_dma_init(0,0);
	
//	CHECKDRAW("out");
	
	return transferred;
};

int dma_getinitted() { return dma_isrunning; }

