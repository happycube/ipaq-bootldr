/*
 * nandif.c - Low level nand interface routines
 *
 */

#include "btflash.h"
#include "sa1100.h"
#include "params.h"
#include "serial.h"

#ifdef	CONFIG_NAND

#include "nand.h"
#include "nand_ecc.h"

unsigned char oob_buf[16];
unsigned char data_buf[512+16];
unsigned char data_cache0[512+16];
unsigned char data_cache1[512+16];
struct mtd_info nand0_mtd, nand1_mtd;
struct nand_chip nand0, nand1;

enum {
    CLE_BIT = 0,
    ALE_BIT = 1,
    NCE0_BIT = 2,
    NCE1_BIT = 3,
    NCE2_BIT = 4,
    NCE3_BIT = 5,
    NSE_BIT = 6,
    NWP_BIT = 7,
};

#define	NAND_CTRL_ADDR	0x40800000
#define	NAND_IO_ADDR	0x40000000

/* standby values */
static u_char balloon_nand_ctrl_image = (1 << NWP_BIT) | (1 << NCE0_BIT) | (1 << NCE1_BIT) | (1 << NCE2_BIT) | (1 << NCE3_BIT);

static void balloon_hwcontrol0(int cmd)
{
	switch(cmd){

		case NAND_CTL_SETCLE: balloon_nand_ctrl_image |= (1 << CLE_BIT); break;
		case NAND_CTL_CLRCLE: balloon_nand_ctrl_image &= ~(1 << CLE_BIT); break;

		case NAND_CTL_SETALE: balloon_nand_ctrl_image |= (1 << ALE_BIT); break;
		case NAND_CTL_CLRALE: balloon_nand_ctrl_image &= ~(1 << ALE_BIT); break;

		case NAND_CTL_SETNCE: balloon_nand_ctrl_image &= ~(1 << NCE0_BIT); break;
		case NAND_CTL_CLRNCE: balloon_nand_ctrl_image |= (1 << NCE0_BIT); break;
	}
//	putLabeledWord("NAND0: 0x",balloon_nand_ctrl_image);
	*(volatile unsigned char *)(NAND_CTRL_ADDR)=balloon_nand_ctrl_image;
}


static void balloon_hwcontrol1(int cmd)
{
	switch(cmd){

		case NAND_CTL_SETCLE: balloon_nand_ctrl_image |= (1 << CLE_BIT); break;
		case NAND_CTL_CLRCLE: balloon_nand_ctrl_image &= ~(1 << CLE_BIT); break;

		case NAND_CTL_SETALE: balloon_nand_ctrl_image |= (1 << ALE_BIT); break;
		case NAND_CTL_CLRALE: balloon_nand_ctrl_image &= ~(1 << ALE_BIT); break;

		case NAND_CTL_SETNCE: balloon_nand_ctrl_image &= ~(1 << NCE3_BIT); break;
		case NAND_CTL_CLRNCE: balloon_nand_ctrl_image |= (1 << NCE3_BIT); break;
	}
//	putLabeledWord("NAND1: 0x",balloon_nand_ctrl_image);
	*(volatile unsigned char *)(NAND_CTRL_ADDR)=balloon_nand_ctrl_image;
}

#if 0
/*
*	read device ready pin
*/
static int balloon_device_ready(void)
{
#define busy() (((*(volatile char *)0x8400000)&1) == 0)
    return (((*(volatile char *)balloon_nand_busy_addr)&1) == 1);
}
#endif


int nandif_init(void) {

    memset(&nand0_mtd,0,sizeof(struct mtd_info));
    memset(&nand1_mtd,0,sizeof(struct mtd_info));

    memset(&nand0,0,sizeof(nand0));
    memset(&nand1,0,sizeof(nand1));

    nand0_mtd.priv=&nand0;
    nand0.data_buf=data_buf;
    nand0.data_cache=data_cache0;
    nand0.cache_page=-1;
    nand0.eccmode=NAND_ECC_SOFT;

    /* Set address of NAND IO lines */
    nand0.IO_ADDR_R = NAND_IO_ADDR;
    nand0.IO_ADDR_W = NAND_IO_ADDR;
    nand0.hwcontrol = balloon_hwcontrol0;
//	nand0.dev_ready = balloon_device_ready;
    nand0.dev_ready = NULL;

    /* 20 us command delay time */
    nand0.chip_delay = 20;		

    nand1_mtd.priv=&nand1;
    nand1.data_buf=data_buf;
    nand1.data_cache=data_cache1;
    nand1.cache_page=-1;
    nand1.eccmode=NAND_ECC_SOFT;

    /* Set address of NAND IO lines */
    nand1.IO_ADDR_R = NAND_IO_ADDR;
    nand1.IO_ADDR_W = NAND_IO_ADDR;
    nand1.hwcontrol = balloon_hwcontrol1;
//	nand1.dev_ready = balloon_device_ready;
    nand1.dev_ready = NULL;

    /* 20 us command delay time */
    nand1.chip_delay = 20;		

    return 0;
}

int nandif_enum(void) {
    return 2;
}

struct mtd_info *nandif_get(int index) {
struct mtd_info *chip=index ? &nand1_mtd:&nand0_mtd;
    putstr("Scanning for nand chip ...\r\n");
    if (nand_scan(chip))
	return NULL;
    return chip;
}

void nand_jffs2_format_region(struct mtd_info *chip, unsigned long base, unsigned long size, 
                                 unsigned long marker0, unsigned long marker1, unsigned long marker2)
{
  unsigned long jffs_blank_sector[3];
  jffs_blank_sector[0]=marker0;
  jffs_blank_sector[1]=marker1;
  jffs_blank_sector[2]=marker2;
  putLabeledWord("Formatting eraseblocks from address 0x",base);
  putLabeledWord("Chip size 0x",chip->size);
  putLabeledWord("Chip erase size 0x",chip->erasesize);
  putLabeledWord("Size to be erased 0x",size);
  putLabeledWord("Format block size  0x",sizeof(jffs_blank_sector));
  while (size) {
    size_t retlen;
    if (chip->write(chip, base, sizeof(jffs_blank_sector), &retlen, (u_char *)jffs_blank_sector)) {
	putstr("nand_jffs2_format: error\r\n");
	putLabeledWord("Write address = 0x",base);
	break;;
    }
    base+=chip->erasesize;
    if (size>=chip->erasesize)
	size-=chip->erasesize;
    else {
	putLabeledWord("Residual size was 0x",size);
	putLabeledWord("at address 0x",base);
	size=0;
    }
  }
  putLabeledWord("First unerased block at 0x",base);
}

#endif

