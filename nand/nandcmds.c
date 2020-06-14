// nandcmds.cpp

#include "params.h"
#include "commands.h"
#include "bootldr.h"
#include "serial.h"
#include "btflash.h"
#include "partition.h"
#include "modem.h"
#include "sa1100.h"
#include <string.h>
#if defined(CONFIG_GZIP) || defined(CONFIG_GZIP_REGION)
#include <zlib.h>
#include <zUtils.h>
#endif


#if defined (CONFIG_YAFFS)
#include "yaffs/yaffs.h"
#include "yaffs/yaffs_guts.h"
#include "util.h"
#endif

//extern byte strtoul_err;
//extern volatile unsigned long *flashword;

//extern FlashDescriptor *flashDescriptor;
//extern struct bootblk_param param_initrd_filename;
//extern struct bootblk_param param_kernel_filename;
//static struct ebsaboot bootinfo;

const char *get_balloon_id(void) {
    return "development";
//    return param_id.value;
}


#if defined(CONFIG_NAND)

#include "nandif.h"

extern int force_erase;

static volatile int blart;
void udelay(int delay) {
int i;
    while (delay--)
	for (i=0; i<5; i++)
	    blart++;
}

extern int use_block_in;
#if defined(CONFIG_GZIP) || defined(CONFIG_GZIP_REGION)
extern int gunzip_region(char*   src,    char*   dst,
		  long len, const char *name);
#endif

COMMAND(ramtest, command_ramtest, "[command] -- Does Ramtest", BB_RUN_FROM_RAM);
void command_ramtest(int argc, const char *argv[]) {
unsigned long ramtest_start = DRAM_BASE0;
size_t ramtest_size = (48*1024*1024)/sizeof(unsigned long);
unsigned long *dest;
size_t error_count;
enum {error_max=100};
size_t i;
    putstr("Ramtest:\r\n");
    putLabeledWord("Ramtest: start address = 0x",ramtest_start);
    putLabeledWord("Ramtest: end address   = 0x",ramtest_start+ramtest_size*sizeof(unsigned long));
    
    putstr("Ramtest: filling zero\r\n");
    dest = (unsigned long *)ramtest_start;
    for (i=0; i<ramtest_size; i++) 
	dest[i]=0;
    putstr("Ramtest: test 1\r\n");
    error_count=0;
    for (i=0; i<ramtest_size; i++) {
	if (dest[i]!=0) {
	    putLabeledWord("Ramtest: fail address   = 0x",ramtest_start+i*sizeof(unsigned long));
	    putLabeledWord("Ramtest: fail value     = 0x",dest[i]);
	    if (++error_count==error_max)
		break;
	}
    }
    putstr("Ramtest: test 2\r\n");
    error_count=0;
    for (i=0; i<ramtest_size; i++) {
	if (dest[i]!=0) {
	    putLabeledWord("Ramtest: fail address   = 0x",ramtest_start+i*sizeof(unsigned long));
	    putLabeledWord("Ramtest: fail value     = 0x",dest[i]);
	    if (++error_count==error_max)
		break;
	}
    }

    putstr("Ramtest: filling 0xffffffff\r\n");
    for (i=0; i<ramtest_size; i++)
	dest[i]=0xffffffff;
    putstr("Ramtest: test 1\r\n");
    error_count=0;
    for (i=0; i<ramtest_size; i++) {
	if (dest[i]!=0xffffffff) {
	    putLabeledWord("Ramtest: fail address   = 0x",ramtest_start+i*sizeof(unsigned long));
	    putLabeledWord("Ramtest: fail value     = 0x",dest[i]);
	    if (++error_count==error_max)
		break;
	}
    }
    putstr("Ramtest: test 2\r\n");
    error_count=0;
    for (i=0; i<ramtest_size; i++) {
	if (dest[i]!=0xffffffff) {
	    putLabeledWord("Ramtest: fail address   = 0x",ramtest_start+i*sizeof(unsigned long));
	    putLabeledWord("Ramtest: fail value     = 0x",dest[i]);
	    if (++error_count==error_max)
		break;
	}
    }

    putstr("Ramtest: filling incrementing\r\n");
    for (i=0; i<ramtest_size; i++)
	dest[i]=i;
    putstr("Ramtest: test 1\r\n");
    error_count=0;
    for (i=0; i<ramtest_size; i++) {
	if (dest[i]!=i) {
	    putLabeledWord("Ramtest: fail address   = 0x",ramtest_start+i*sizeof(unsigned long));
	    putLabeledWord("Ramtest: expected value = 0x",i);
	    putLabeledWord("Ramtest: fail value     = 0x",dest[i]);
	    if (++error_count==error_max)
		break;
	}
    }
    putstr("Ramtest: test 2\r\n");
    error_count=0;
    for (i=0; i<ramtest_size; i++) {
	if (dest[i]!=i) {
	    putLabeledWord("Ramtest: fail address   = 0x",ramtest_start+i*sizeof(unsigned long));
	    putLabeledWord("Ramtest: expected value = 0x",i);
	    putLabeledWord("Ramtest: fail value     = 0x",dest[i]);
	    if (++error_count==error_max)
		break;
	}
    }
}

COMMAND(speed, command_speed, "[command] -- Does SPEED stuff", BB_RUN_FROM_RAM);
void command_speed(int argc, const char *argv[]) {
int speed=0;

    speed=CTL_REG_READ(PPCR_REG);
    putLabeledWord("Speed: CPU speed index = 0x",speed);
    if (argc>1) {
        speed = strtoul(argv[1], NULL, 0);
	putLabeledWord("Speed: CPU speed set to = 0x",speed);
	CTL_REG_WRITE(PPCR_REG,speed);
    }
}

int debug_bits=0;
COMMAND(set_debug, command_set_debug, "[set_debug] -- Sets an int of debug bits", BB_RUN_FROM_RAM);
void command_set_debug(int argc, const char *argv[]) {
    if (argc>1) {
        debug_bits = strtoul(argv[1], NULL, 0);
    }
    putLabeledWord("Debug bits = 0x",debug_bits);
}

COMMAND(build, command_build, "[build] -- build production system from smart media source", BB_RUN_FROM_RAM);
void command_build(int argc, const char *argv[]) {
#if defined (CONFIG_YAFFS)
    char tmp[100]={0};
    void * yaffs_obj=NULL;
    void *img=(void *)((unsigned long)(VKERNEL_BASE) + 1024); /* just a temporary holding area */
    size_t img_len=0;
    if (argc>1) {
	if (!strcmp(argv[1],"base")) {
	    strcpy(tmp,"nand yaffs boot sm:/boot.yaffs.gz");
	    exec_string(tmp);
	    strcpy(tmp,"nand yaffs root sm:/root-balloon-base.yaffs.gz");
	    exec_string(tmp);
	}
	else if (!strcmp(argv[1],"std")) {
	    strcpy(tmp,"nand yaffs boot sm:/boot.yaffs.gz");
	    exec_string(tmp);
	    strcpy(tmp,"nand yaffs root sm:/root-balloon-std.yaffs.gz");
	    exec_string(tmp);
	}
	else if ((!strcmp(argv[1],"bootldr")) || (!strcmp(argv[1],"bootldr.fast") || (!strcmp(argv[1],"bootldr.slow")))) {
	    char temporary[40];
	    char filename[40];
	    if (!strcmp(argv[1],"bootldr"))
#if defined(CONFIG_BALLOON_SLOW_CPU)
		strcpy(filename,"sm:/bootldr.slow.gz");
#else
		strcpy(filename,"sm:/bootldr.fast.gz");
#endif
	    else 
		strcpy(filename,(!strcmp(argv[1],"bootldr.fast")) ? "sm:/bootldr.fast.gz":"sm:/bootldr.slow.gz");
	    yaffs_obj=yaffs_fs_open(filename);
	    if (!yaffs_obj) {
		strcpy(filename,(!strcmp(argv[1],"bootldr.fast")) ? "sm:/bootldr.fast.gz":"sm:/bootldr.slow.gz");
		putstr("File <"); putstr(filename); putstr("> not found\r\n");
		return;
	    }
	    img_len=yaffs_fs_file_length(yaffs_obj);
	    if (!yaffs_fs_read(yaffs_obj,img,0,img_len)==img_len) {
		putstr("File read error\r\n");
		return;
	    }
	    strcpy(tmp,"program bootldr 0x");
	    binarytohex(temporary,img,sizeof(img));
	    strcat(tmp,temporary);
	    strcat(tmp," 0x");
	    binarytohex(temporary,img_len,sizeof(img_len));
	    strcat(tmp,temporary);
	    putstr("exec-ing "); putstr(tmp); putstr("\r\n");
	    exec_string(tmp);
	}
	else if (!strcmp(argv[1],"boot")) {
	    strcpy(tmp,"nand yaffs boot sm:/boot.yaffs.gz");
	    exec_string(tmp);
	}
	else if (!strcmp(argv[1],"root")) {
	    strcpy(tmp,"nand yaffs root sm:/");
	    if (argc>2)
		strcat(tmp,argv[2]);
	    else
		strcat(tmp,"root-balloon-std.yaffs.gz");
	    exec_string(tmp);
	}
	else if (!strcmp(argv[1],"save")) {
	    strcpy(tmp,"nand yaffs save root ");
	    if (argc>2) {
		if (!strchr(argv[2],':'))
		    strcat(tmp,"sm:/");
		strcat(tmp,argv[2]);
	    }
	    else
		strcat(tmp,"sm:/root.save");
	    exec_string(tmp);
	}
	else if (!strcmp(argv[1],"restore")) {
	    strcpy(tmp,"nand yaffs root ");
	    if (argc>2) {
		if (!strchr(argv[2],':'))
		    strcat(tmp,"sm:/");
		strcat(tmp,argv[2]);
	    }
	    else
		strcat(tmp,"sm:/root.save");
	    exec_string(tmp);
	}
	else {
	    strcpy(tmp,"yaffs cp ");
	    strcat(tmp,"sm:/cramfs/");
	    strcat(tmp,argv[1]);
	    strcat(tmp,".cramfs root:/usr/usr.local.cramfs");
	    putstr("exec-ing "); putstr(tmp); putstr("\r\n");
	    exec_string(tmp);
	}
    }
    else {
	putstr("usage: build [ base | std | bootldr | bootldr.fast | bootldr.slow | boot | root [filename] | <filename> (for usr.local.cramfs)]\r\n"); 
	putstr("also: build save | resore [ filename ] - saves/restores root filesystem to file\r\n"); 
	putstr(" Generally production should do <build std>\r\n");
	putstr(" and then <build swedish_infovox_formant> or some similar filename\r\n");
    }
#else
    putstr("build: needs YAFFS support\r\n");
#endif
}

COMMAND(nand, command_nand, "[command] -- Does NAND stuff", BB_RUN_FROM_RAM);
void command_nand(int argc, const char *argv[]) {
unsigned int address=0;
int i=0;
struct mtd_info *chip=NULL;
unsigned char page[512+16];
unsigned char spare[16];
extern int nand_wait;

    // let us be safe
    force_erase = 0;

    if ((argc>1) && !strcmp(argv[1],"0")) {
	chip = nandif_get(0);
	argc--;
	argv++;
    }
    else if ((argc>1) && !strcmp(argv[1],"1")) {
	chip = nandif_get(1);
	argc--;
	argv++;
    }
    else
	chip = nandif_get(0);
    

    if ((argc>1)) {
	if (!strcmp(argv[1],"nocache")) {
	    asmEnableCaches(0,0);
	    return;
	}
	if (!strcmp(argv[1],"icache")) {
	    asmEnableCaches(0,i);
	    return;
	}
	if (!strcmp(argv[1],"dcache")) {
	    asmEnableCaches(1,0);
	    return;
	}
	if (!strcmp(argv[1],"bothcache")) {
	    asmEnableCaches(1,1);
	    return;
	}
	if (!strcmp(argv[1],"init")) {
	    nandif_init();
	    return;
	}
	if (!strcmp(argv[1],"waitinit")) {
	    nand_wait=0;
	    return;
	}
	if (!strcmp(argv[1],"wait")) {
	    putLabeledWord("nand: nand_wait = 0x",nand_wait);
	    nand_wait=0;
	    return;
	}
	if (!strcmp(argv[1],"force_erase")) {
	    force_erase = 1;
	    argc--;
	    argv++;
	    putstr("**WARNING** Forcing NAND erase is a *BAD* practice!\r\n");
	}
    }

    if (!chip) {
	putstr("nand: cannot get NAND device chip");
	return;
    }

    if (argc>1) {
	if (!strcmp(argv[1],"size")) {
	    putLabeledWord("nand chip: size = 0x",chip->size);
	}
	else if (!strcmp(argv[1],"erasesize")) {
	    putLabeledWord("nand chip: erasesize = 0x",chip->erasesize);
	}
	else if (!strcmp(argv[1],"scanblocks")) {
	    int i, pass=0, fail=0;
	    putstr("nand scanblocks: Scanning all blocks for bad block data\r\n");
	    for (i=0; i<chip->size; i+=chip->erasesize) {
		size_t count;
		chip->read_oob(chip, i, 16, &count, spare);
		if (spare[5]!=0xff) {
		    putLabeledWord("Bad block. sector 0x",i/512);
		    fail++;
		}
		else
		    pass++;
	    }
	    putLabeledWord("scanblocks passed 0x",pass);
	    putLabeledWord("scanblocks failed 0x",fail);
	}
	else if (!strcmp(argv[1],"markbadblock")) {
	    int block=-1;
	    int count;
	    if (argc<3)
		putstr("nand markbadblock: needs block number\r\n");
	    block=strtoul(argv[2], NULL, 0);	    
	    
	    chip->read_oob(chip, block*chip->erasesize, 16, &count, spare);
	    if (spare[5]!=0xff) 
		putstr("Block already marked as bad\r\n");
	    else {
		putLabeledWord("nand markblock: Marking bad block 0x",block);
		spare[5]=0xa5;
		chip->write_oob(chip, block*chip->erasesize, 16, &count, spare);
		chip->read_oob(chip, block*chip->erasesize, 16, &count, spare);
		if (spare[5]==0xa5)
		    putstr("Block successfully marked bad\r\n");
		else
		    putLabeledWord("Bad block mark is 0x",spare[5]);
	    }
	}
	else if (!strcmp(argv[1],"markbadsector")) {
	    int block=-1;
	    int count;
	    if (argc<3)
		putstr("nand markbadblock: needs block number\r\n");
	    block=strtoul(argv[2], NULL, 0);	    
	    
	    chip->read_oob(chip, block*512, 16, &count, spare);
	    if (spare[5]!=0xff) 
		putstr("Block already marked as bad\r\n");
	    else {
		putLabeledWord("nand markblock: Marking bad block 0x",block);
		spare[5]=0xa5;
		chip->write_oob(chip, block*512, 16, &count, spare);
		chip->read_oob(chip, block*512, 16, &count, spare);
		if (spare[5]==0xa5)
		    putstr("Sector successfully marked bad\r\n");
		else
		    putLabeledWord("Bad block mark is 0x",spare[5]);
	    }
	}
	else if (!strcmp(argv[1],"checkblocks")) {
	    int block,sector;
	    int firstblock=0;
	    int lastblock=(chip->size/chip->erasesize)-1;
	    putstr("nand checkblocks: Checking all blocks for bad block data\r\n");
	    if (argc>2)
		firstblock=strtoul(argv[2], NULL, 0);	    
	    if (argc>3) 
		lastblock=strtoul(argv[3], NULL, 0);
	    if (firstblock<0) {
		putstr("Firstblock must be positive\r\n");
		return;
	    }	    
	    if (lastblock>=(chip->size/chip->erasesize)) {
		putstr("Lastblock is past the end of the device\r\n");
		return;
	    }	    
	    if (lastblock<firstblock) {
		putstr("Lastblock is less than firstblock\r\n");
		return;
	    }	    
	    putstr("Checking blocks from ");
	    putDecInt(firstblock);
	    putstr(" to ");
	    putDecInt(lastblock);
	    putstr(" inclusive\r\n");
	    for (block=firstblock; block<=lastblock; block++) {
		size_t count;
		int val;
		chip->read_oob(chip, block*chip->erasesize, 16, &count, spare);
		// skip block if already bad
		if (spare[5]!=0xff) {
		    putLabeledWord("Skipping bad block 0x",block);
		    continue;
		}

		// display block index
		putstr("Checking block ");
		putDecInt(block);
		putstr(" ");
		putDecIntWidth(((block-firstblock)*100)/(lastblock-firstblock+1),4);
		putstr("% ");
		
		
		// now erase/program/verify the block with marching pattern
		for (val=0; val<8; val++) {
		
		    putc('e');
		    // erase the block
		    if (chip->erase(chip, block*chip->erasesize, chip->erasesize)) {
			putLabeledWord("Erase bad block failed block 0x",block);
			continue;	    
		    }

		    // initialise page data		
		    memset(page,1<<val,512);
		    memset(page+512,0xff,16);
		    
		    putc('w');
		    // now write the page to all sectors
		    for (sector=0; sector<chip->erasesize/512; sector++) {
			if (chip->write_ecc(chip,(block*chip->erasesize)+(sector*512),512,&count,page,page+512,NAND_YAFFS_OOB)) {
			    putLabeledWord("Write failed at sector 0x",+sector);
			    break;
			}
		    }
		    
		    putc('r');
		    // now read each page and check that it verifies
		    for (sector=0; sector<chip->erasesize/512; sector++) {
			if (chip->read_ecc(chip,(block*chip->erasesize)+(sector*512),512,&count,page,page+512,NAND_YAFFS_OOB)) {
//			if (chip->read_ecc((block*chip->erasesize)+(sector*512),page,&count) {
			    putLabeledWord("Page read failed at sector 0x",+sector);
			    break;
			}
			for (i=0; i<512; i++) {
			    if (page[i]!=1<<val) {
				putLabeledWord("aarrgh!! Verify error at address 0x",i);
				break;
			    }
			}
		    }
		}
		putstr("\r\n");
	    }
	    putstr("checkblocks completed\r\n");
	}
	else if (!strcmp(argv[1],"scansectors")) {
	    int i, pass=0, fail=0;
	    putstr("nand scansectors: Scanning all sectors for bad sector data\r\n");
	    for (i=0; i<chip->size; i+=512) {
		size_t count;
		chip->read_oob(chip, i, 16, &count, spare);
		if (spare[5]!=0xff) {
		    putLabeledWord("Bad block. sector 0x",i/512);
		    fail++;
		}
		else
		    pass++;
	    }
	    putLabeledWord("scansectors passed 0x",pass);
	    putLabeledWord("scansectors failed 0x",fail);
	}
	else if (!strcmp(argv[1],"erasechip")) {
	    int i, errors;
	    putstr("nand erasechip: Erasing entire chip\r\n");
	    errors=0;
	    for (i=0; i<chip->size; i+=chip->erasesize) {
		if (chip->erase(chip, i, chip->erasesize))
		    errors++;
	    }
	    if (errors) {
		putstr("nand erasechip: erase failed\r\n");
		return;
	    }
	    if ((argc>2) && !strcmp(argv[2],"verify")) {
		int count;
		putstr("Verifying erase of all sectors\r\n");
		errors=0;
		for (i=0; i<chip->size; i+=512) {
		    int sector_bad=0;
		    chip->read(chip,i,512,&count,page);
		    chip->read(chip,i,16,&count,spare);
		    // skip bad blocks
		    if ((i%chip->erasesize)==0) {
			if (spare[5]!=0xff) {
			    putLabeledWord("Skipping bad block at sector 0x",i/512);
			    i+=(chip->erasesize-512);
			    continue;
			}
		    }
		    // erk - sector marked bad
		    if (spare[5]!=0xff) {
			putLabeledWord("Unexpected bad block mark error in oob of sector 0x",i/512);
			sector_bad=1;
			errors++;
		    }
		    // check oob
		    if (!sector_bad) {
			for (count=0; count<16; count++) {
			    if (spare[count]!=0xff) {
				putLabeledWord("Error in oob of sector 0x",i/512);
				sector_bad=1;
				errors++;
				break;
			    }
			}
		    }
		    // check data
		    if (!sector_bad) {
			for (count=0; count<512; count++) {
			    if (page[count]!=0xff) {
				putLabeledWord("Error in data of sector 0x",i/512);
				errors++;
				sector_bad=1;
				break;
			    }
			}
		    }
		}
		if (errors) {
		    putLabeledWord("nand erasechip: sector verify failures 0x0",errors);
		    return;
		}
	    }
	    putstr("nand erasechip: passed\r\n");
	}
	else if (!strcmp(argv[1],"eraseblocks")) {
	    int block;
	    int firstblock=0;
	    int lastblock=(chip->size/chip->erasesize)-1;
	    putstr("nand eraseblocks: Erasing blocks\r\n");
	    if (argc>2)
		firstblock=strtoul(argv[2], NULL, 0);	    
	    if (argc>3) 
		lastblock=strtoul(argv[3], NULL, 0);	    
	    for (block=firstblock; block<=lastblock; block++) {
		if (chip->erase(chip, block*chip->erasesize, chip->erasesize)) 
		    putLabeledWord("Bad erase of block 0x",block);
	    }
	}
#if 0
	else if (!strcmp(argv[1],"blocktest")) {
	    int old_use_block_in=use_block_in;
	    unsigned char sector_buffer1[512], sector_buffer2[512];
	    int sector,i,error_count;
	    int retlen;
	    memset(sector_buffer1,0,512);
	    memset(sector_buffer2,0,512);

	    sector=1;
	    if (argc>2)
                sector = strtoul(argv[2], NULL, 0);

	    putstr("Reading sector slowly\r\n");
	    use_block_in=0;
	    chip->read(chip, sector*512 ,512 , &retlen, sector_buffer1);
	    putstr("Reading sector fast\r\n");
	    use_block_in=1;
	    chip->read(chip, sector*512 ,512 , &retlen, sector_buffer2);
	    use_block_in=old_use_block_in;

	    putstr("Comparing sectors\r\n");
	    error_count=0;
	    for (i=0; i<512; i++) {
		if (sector_buffer1[i] != sector_buffer2[i]) {
		    if (error_count<20) {
			putLabeledWord("Address ",i);
			putLabeledWord("Slow value = ",sector_buffer1[i]);
			putLabeledWord("Fast value = ",sector_buffer2[i]);
		    }
		    error_count++;
		}
	    }
	    putLabeledWord("Error count ",error_count);
	    if (error_count>20) {
		for (i=0; i<32; i++)
		    putLabeledWord("Slow ",sector_buffer1[i]);
		putstr("\r\n");
		for (i=0; i<32; i++)
		    putLabeledWord("Fast ",sector_buffer2[i]);
	    }
	}
#endif
#if 1
	else if (!strcmp(argv[1],"test")) {
	    unsigned char page_to_write[512];
	    unsigned char read_page[512];
	    int index;
	    int sector;
	    int retlen;
	    int numsecs=1; // 32*1024 possible
	    int eraseblock_pages=chip->erasesize/512;
	    int erase_pages;
	    if (argc>2)
                numsecs = strtoul(argv[2], NULL, 0);
	    if (numsecs == 0) {
		putstr("No sectors specified!\r\n");
		return;
	    }
	    erase_pages=((numsecs+eraseblock_pages-1)/eraseblock_pages)*eraseblock_pages;
	    putLabeledWord("nand test: Erasing 0x", numsecs);
	    putLabeledWord("nand test: Actually Erasing 0x", erase_pages);
	    if (chip->erase(chip, 0, erase_pages*512)) {
		putstr("nand: erase failed\r\n");
		return;
	    }
	    memset(page_to_write,0,sizeof(page_to_write));
	    strcpy((char *)page_to_write,"Hello World.");
	    strcpy((char *)page_to_write+256,"This is some news.");
//	    memcpy(page_to_write,flashword,512);

	    putLabeledWord("nand test: writing sectors ... 0x", numsecs);
	    for (sector=0; sector<numsecs; sector++) {
		if (chip->write(chip, sector*512 ,512 , &retlen, page_to_write)) {
		    putLabeledWord("nand test: writing failed at sector 0x", sector);
//		    return;
		}
		if (!(sector%1024))
		    putLabeledWord("nand test: written 0x", sector);
	    }

	    putLabeledWord("nand test: reading sectors ... 0x", i);
	    for (sector=0; sector<numsecs; sector++) {
		if (chip->read(chip, sector*512 ,512 , &retlen, read_page)) {
		    putLabeledWord("nand test: read failed at sector 0x", sector);
		    return;
		}
		for (index=0; index<512; index++) {
		    if (page_to_write[index]!=read_page[index]) {
			putLabeledWord("nand test: error index 0x", index);
			putLabeledWord("nand test: written 0x", page_to_write[index]);
			putLabeledWord("nand test: read 0x", read_page[index]);
			return;
		    }
		}
		if (!(sector%1024))
		    putLabeledWord("nand test: read 0x", sector);
	    }
	    putstr("nand test: passed\r\n");
	    
	}
#endif
#if 1
	else if (!strcmp(argv[1],"read")) {
	    size_t retlen;
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);

	    putLabeledWord("nand: Reading 0x", address);
	    if (chip->read(chip, address*512, 512, &retlen, page)) 
		putstr("failed!\r\n");
	    else {
		putstr("Ok\r\n");
		for (i=0; i<16; i++)
		    putLabeledWord("nand : data 0x", ((unsigned long *)page)[i]);
	    }
	}
	else if (!strcmp(argv[1],"write")) {
	    size_t retlen;
	    char verify_page[512];
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);

	    putLabeledWord("nand: Programming data from address 0x", address);
	    memset(page, 0, sizeof(page));
	    for (i=0; i<512; i++)
		page[i]=i&0xff;
	    if (chip->write(chip, address*512, 512, &retlen, page))
		putstr("nand: write failed - verifying anyway.\r\n");
		
	    if (chip->read(chip, address*512, 512, &retlen, verify_page)) {
		putstr("band: write verify read failed\r\n");
		return;
	    }
	    
	    for (i=0; i<512; i++) {
		if (page[i] != verify_page[i]) {
		    putLabeledWord("nand: verify failed at", i);
		    putLabeledWord("nand: written data ", page[i]);
		    putLabeledWord("nand: read data ", verify_page[i]);
		    return;
		}
	    }
	    putstr("nand: write verified\r\n");
	}
#endif
#if 1
	else if (!strcmp(argv[1],"read_all_ecc")) {
	    size_t retlen;
	    unsigned char tmpSpare[16 + (2*sizeof(int))];
    	    struct FlashRegion *partition;
	    int regionSize=0;
	    int regionBase=0;
	    int address=regionBase;
	    int errors=0;
    	    partition = btflash_get_partition( (argc>2) ?argv[2]:"boot");
	    if (partition) {
		regionSize=partition->size;
		regionBase=partition->base;
		if (!(partition->flags & LFR_NAND)) {
		    putstr("Not a NAND partition\n");
		    return;
		}
	    }
	    argc--;
	    argv++;
	    
	    putLabeledWord("nand: Reading sectors count=0x", regionSize/512);
	    while (regionSize) {
		memset(tmpSpare,0x55,sizeof(tmpSpare));
		if (chip->read_ecc(chip, address/512, 512, &retlen, page, tmpSpare, NAND_YAFFS_OOB)) {
		    putstr("nand: Read ecc failed\r\n");
		    putLabeledWord("sector = 0x",address/512);
		}
		
		if (((int *)(tmpSpare+16))[0]) {
		    putLabeledWord("Sector read ecc1 failed, sector = 0x",address/512);
		    putLabeledWord(" ecc1 = 0x",((int *)(tmpSpare+16))[0]);
		    errors++;
		}
		if (((int *)(tmpSpare+16))[1]) {
		    putLabeledWord("Sector read ecc2 failed, sector = 0x",address/512);
		    putLabeledWord(" ecc2 = 0x",((int *)(tmpSpare+16))[1]);
		    errors++;
		}
		regionSize-=512;
		address+=512;
	    }
	    putLabeledWord("Errors = 0x",errors);
	}
	else if (!strcmp(argv[1],"read_ecc")) {
	    size_t retlen;
	    unsigned char tmpSpare[16 + (2*sizeof(int))];
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);

	    putLabeledWord("nand: Reading data with ecc from address 0x", address);
	    memset(tmpSpare,0x55,sizeof(tmpSpare));
	    if (chip->read_ecc(chip, address*512, 512, &retlen, page, tmpSpare, NAND_YAFFS_OOB)) {
		putstr("nand: Read ecc failed\r\n");
		return;
	    }
	    for (i=0; i<16; i++)
		putLabeledWord("nand : spare 0x", ((unsigned char *)tmpSpare)[i]);
	    putLabeledWord("nand: eccres1 = 0x",((int *)(tmpSpare+16))[0]);
	    putLabeledWord("nand: eccres2 = 0x",((int *)(tmpSpare+16))[1]);
	}
	else if (!strcmp(argv[1],"read_oob")) {
	    size_t retlen;
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);

	    putLabeledWord("nand: Reading spare from address 0x", address);
	    memset(spare,0x55,sizeof(spare));
	    if (chip->read_oob(chip, address*512, 16, &retlen, spare)) {
		putstr("nand: Read spare failed\r\n");
		return;
	    }
	    for (i=0; i<16; i++)
		putLabeledWord("nand : spare 0x", ((unsigned char *)spare)[i]);
	}
	else if (!strcmp(argv[1],"write_oob")) {
	    size_t retlen;
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);

	    memset(spare,0xff,sizeof(spare));
	    putLabeledWord("nand: Writing oob 0x", address);
	    for (i=3; i<3+16; i++) {
		if (i<argc)
		    ((unsigned char *)spare)[i-3]=(unsigned char)strtoul(argv[i], NULL, 0);
	    }
	    if (chip->write_oob(chip,address*512,16,&retlen,spare))
		putstr("failed!\r\n");
	    else {
		putstr("Ok\r\n");
	    }
	}
	else if (!strcmp(argv[1],"write_ecc")) {
	    size_t retlen;
	    if (argc>2) {
                address = strtoul(argv[2], NULL, 0);
		argc--;
		argv++;
	    }
	    memset(page,0xff,sizeof(page));
	    memset(spare,0xff,sizeof(spare));
	    for (i=2; i<2+512; i++) {
		if (i<argc)
		    ((unsigned char *)page)[i-2]=(unsigned char)strtoul(argv[i], NULL, 0);
	    }
	    putLabeledWord("nand: Writing ecc 0x", address);
	    if (chip->write_ecc(chip,address*512,512,&retlen,page,spare,NAND_YAFFS_OOB))
		putstr("failed!\r\n");
	    else {
		putstr("Ok\r\n");
	    }
	}
#endif
#if 1
	else if (!strcmp(argv[1],"read_ecc")) {
	    unsigned char oob[16];
	    size_t retlen;
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);

	    putLabeledWord("nand: Reading 0x", address);
	    if (chip->read_ecc(chip, address*512, 512, &retlen, page, oob, NAND_YAFFS_OOB)) 
		putstr("failed!\r\n");
	    else {
		putstr("Ok\r\n");
		for (i=0; i<16; i++)
		    putLabeledWord("nand : data 0x", ((unsigned long *)page)[i]);
		for (i=0; i<16; i++)
		    putLabeledWord("nand : oob 0x", oob[i]);
	    }
	}
	else if (!strcmp(argv[1],"write_ecc")) {
	    char verify_page[512+16];
	    unsigned char verify_oob[16];
	    size_t retlen;
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);

	    putLabeledWord("nand: Programming data from address 0x", address);
	    memset(page, 0, sizeof(page));
	    memset(spare, 0xff, sizeof(spare));
	    for (i=0; i<512; i++)
		page[i]=i&0xff;
	    spare[0]=16;
	    spare[1]=17;
	    spare[3]=18;
	    if (chip->write_ecc(chip, address*512, 512, &retlen, page, spare, NAND_YAFFS_OOB)) 
		putstr("nand: write failed - verifying\r\n");
	    if (chip->read_ecc(chip, address*512, 512, &retlen, verify_page, verify_oob, NAND_YAFFS_OOB)) {
		putstr("nand: verify read failed\r\n");
		return;
	    }
	    for (i=0; i<512; i++) {
		if (page[i] != verify_page[i]) {
		    putLabeledWord("nand: verify failed at", i);
		    putLabeledWord("nand: written data ", page[i]);
		    putLabeledWord("nand: read data ", verify_page[i]);
		    return;
		}
	    }
	    for (i=0; i<16; i++) {
		if (spare[i] != verify_oob[i]) {
		    putLabeledWord("nand: oob verify failed at", i);
		    putLabeledWord("nand: oob written data ", spare[i]);
		    putLabeledWord("nand: read oob data ", verify_oob[i]);
		}
	    }	    
	}
#endif
#if 1
	else if (!strcmp(argv[1],"erase")) {
	    unsigned long erase_size=1;
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);
	    if (argc>3)
                erase_size = strtoul(argv[3], NULL, 0);

	    putLabeledWord("nand: Erasing from address 0x", address);
	    if (chip->erase(chip, address*512, chip->erasesize)) {
		putstr("nand: erase failed\r\n");
		return;
	    }
	}
#endif
#if 0
	else if (!strcmp(argv[1],"udelay")) {
	    if (argc>2)
                address = strtoul(argv[2], NULL, 0);
	    putLabeledWord("udelay of 0x",address);
	    putstr("udelay ...");
	    udelay(address);
	    putstr(" done.\r\n");
	}
#endif
#if	defined(CONFIG_YAFFS)
	else if (!strcmp(argv[1],"yaffs")) {
	    unsigned long img_size = 0; /* size of data */
	    unsigned long img_dest = VKERNEL_BASE + 1024; /* just a temporary holding area */
	    int tryGZip = 0;
#if defined(CONFIG_GZIP)
	    int isGZip = 0;
	    struct bz_stream z;
	    unsigned long compr_img_size = 0;
#endif
	    long ymodem = 0;
	    size_t regionBase = 0;
	    size_t regionSize = chip->size;
	    int sector = 0;
	    int pagesPerBlock = chip->erasesize/512;
	    unsigned char *src=(unsigned char *)img_dest;
	    int save=0;
	    
	    // are we saving a partition?
	    if ((argc>2) && !strcmp("save",argv[2])) {
		save=1;
		argc--;
		argv++;
		
		// are we prohibiting gzipping the file?
		if ((argc>2) && !strcmp("gzip",argv[2])) {
		    tryGZip=1;
		    argc--;
		    argv++;
		}
	    }

	    // region name?
	    if (argc>2) {
    		struct FlashRegion *partition = btflash_get_partition(argv[2]);
		if (partition) {
		    regionSize=partition->size;
		    regionBase=partition->base;
		    if (!(partition->flags & LFR_NAND)) {
			putstr("Not a NAND partition\n");
			return;
		    }
		}
		else {
    		    putstr("Cannot find partition <");
		    putstr(argv[2]);
		    putstr(">\n");
		    return;
		}    
	    }

	    // save a yaffs image
	    if (save) {
		yaffs_Spare oob;
		yaffs_TagsUnion tags;
		int numread;
#if defined(CONFIG_GZIP_WRITE)
		if (tryGZip) {
		    gzInitWriteStream((unsigned long)src,SZ_16M,&z);
		    isGZip=1;
		    putstr("Compressing using gzip\r\n");
		}
#endif
		// skip first block
		regionSize-=chip->erasesize;
		regionBase+=chip->erasesize;
		// read in yaffs file image skipping unused pages
		while (regionSize>0) {
		    // read oob
		    if (chip->read_oob(chip,regionBase,16,&numread,(unsigned char *)&oob)) {
			putstr("nand read error\r\n");
			return;
		    }
		    // make tags
		    tags.asBytes[0]=oob.tagByte0;
		    tags.asBytes[1]=oob.tagByte1;
		    tags.asBytes[2]=oob.tagByte2;
		    tags.asBytes[3]=oob.tagByte3;
		    tags.asBytes[4]=oob.tagByte4;
		    tags.asBytes[5]=oob.tagByte5;
		    tags.asBytes[6]=oob.tagByte6;
		    tags.asBytes[7]=oob.tagByte7;
		    // are we starting a block?
		    if ((regionBase%chip->erasesize)==0) {
			// oob[5] must be 0xff (ish) or the block is bad
			// if we are a bad block or not allocated
			if ((oob.blockStatus!=0xff) || (tags.asTags.objectId == YAFFS_UNUSED_OBJECT_ID)) {
			    // skip the whole block
			    if (oob.blockStatus!=0xff) {
//				putLabeledWord("Skipping bad block at chip offset 0x",regionBase);
			    }
			    else {
//				putLabeledWord("Skipping unallocated block at chip offset 0x",regionBase);
			    }
			    regionBase+=chip->erasesize;
			    regionSize-=chip->erasesize;
			    continue;
			}
		    }
		    // pageStatus is zero if deleted
		    // unused objectIds are skipped
		    if (oob.pageStatus == 0)  {
//			putLabeledWord("Skipping deleted page at chip offset 0x",regionBase);
		    }
		    else if (tags.asTags.objectId == YAFFS_UNUSED_OBJECT_ID) {
//			putLabeledWord("Skipping unused object at chip offset 0x",regionBase);
		    }
		    else {
			// page is not deleted and not invalid
//			putLabeledWord("Reading page at chip offset 0x",regionBase);
#if defined(CONFIG_GZIP_WRITE)
			if (isGZip) {
			char buffer[512+16+8];
			    chip->read_ecc(chip,regionBase,512,&numread,buffer,buffer+512,NAND_YAFFS_OOB);
			    if (gzWrite(&z,&buffer,512+16)) {
				putstr("gzWriteError\r\n");
				return;
			    }
			}
			else 
#endif
			{
			    chip->read_ecc(chip,regionBase,512,&numread,img_dest,img_dest+512,NAND_YAFFS_OOB);
			    img_dest+=512+16;
			    img_size+=512+16;
			}
		    }
		    regionBase+=512;
		    regionSize-=512;
		}
#if defined(CONFIG_GZIP_WRITE)
		if (isGZip) {
		    gzWriteFlush(&z);
		    putLabeledWord("Uncompressed image size = 0x",z.stream.total_in);
		    putLabeledWord("Gzipped image size = 0x",z.stream.total_out);
		    img_size=(unsigned long)z.stream.next_out-(unsigned long)src;
		}
#endif
		// what did we get?
		putLabeledWord("Image size = 0x",img_size);
		
/*
		// don't save GZip files yet
		if (isGZip) {
		    putstr("Not saving gzip files yet\r\n");
		    return;
		}
*/
		
		if (img_size) {
		    if (argc>3) {
			int wresult= yaffs_fs_writefile(argv[3],src,img_size);
			if (wresult) {
			    putstr("Write failed to file <"); putstr(argv[3]); putLabeledWord("> result = 0x",wresult);
			}
			else
			    putstr("Written ok\r\n");
		    }
		    else {
			putstr("Sending yaffs fs image \r\n");
			putstr("Start your xmodem upload now ... \r\n");
			if (!modem_send(src, img_size))
    			    putstr("download error.\r\n");
		    }
		}
		else
		    putstr("File size is zero. Nothing to save\r\n");
		return;
	    }

	    if (amRunningFromRam() && regionBase >= flashDescriptor->bootldr.size) {
    		putstr("Can't load yaffs image while running from ram.  Operation canceled\r\n");
    		return;
	    }

	    putstr("loading yaffs image\r\n"); 

#ifdef CONFIG_YAFFS
	    if (argc>3) {
		void *yaffs_file=NULL;
		putstr("Opening <"); putstr(argv[3]); putstr(">\r\n");
		yaffs_file=yaffs_fs_open(argv[3]);
		if (yaffs_file) {
		    img_size=yaffs_fs_file_length(yaffs_file);
		    putLabeledWord("Reading file length 0x",img_size);
		    if (yaffs_fs_read(yaffs_file,img_dest,0,img_size)!=img_size) {
			putstr("Yaffs read failure\r\n");
			return;
		    }
		}
		else {
		    putstr("Cannot open file\r\n");
		    return;
		}
	    }
	    else 
#endif
	    {
#ifdef CONFIG_YMODEM
	    ymodem = param_ymodem.value;
#endif

	    if (ymodem) {
#ifdef CONFIG_YMODEM
    		putstr("using ymodem\r\n");
    		img_size = ymodem_receive((char*)img_dest, regionSize);
#endif
	    } else {
    		putstr("using xmodem\r\n");
    		img_size = modem_receive((char*)img_dest, regionSize);
	    }
	    if (!img_size) {
    		putstr("download error. aborting.\r\n");
    		return;
	    }
	    }

#if defined(CONFIG_GZIP)
	    // are we gzipped image
	    if (isGZipRegion(img_dest)) {
    		isGZip = 1;
 
    		putstr("Looks like a gzipped image, verifying ...\r\n");
		// save the uncompressed image size
		compr_img_size = img_size;
    		if (!verifyGZipImage(img_dest,&img_size)) {	   
		    putstr("Invalid gzip image.\r\n");
		    return;
		}
		else
		    putstr("Gzip image detected and verified\r\n");
    		// img_size now has the uncomopressed size in it.

    		gzInitStream(img_dest,compr_img_size,&z);
	    }
#endif

	    if (img_size%(512+16)) {
		putstr("Filesize must be multiple of 512+16 for yaffs partition\r\n");
		return;
	    }

#ifdef CONFIG_YAFFS
	    yaffs_fs_reset();
#endif

	    // erase "region"	    
    	    putstr("erasing ...\r\n");
    	    putstr("Erasing NAND region\r\n");
	    putLabeledWord("Erasing from 0x",regionBase);
	    putLabeledWord("Erasing bytes 0x",regionSize);
	    if (chip->erase(chip, regionBase, regionSize)) {
		putstr("nand: erase failed\r\n");
		return;
	    }
	    else
        	putstr("nand: erase ok\r\n");
		
	    // get start sector
	    sector = regionBase/512;
#if 1	// first mtd block is *always* skipped by yaffs
		sector += pagesPerBlock;
#else	    // but skip first block
	    if (sector<pagesPerBlock)
		sector = pagesPerBlock;
#endif
		
	    putLabeledWord("nand_yaffsimage: Programming/Verifying 0x",img_size);
	    while (img_size>=(512+16)) {
		#define EXTRA (sizeof(int)*2)
		unsigned char vbuff[512+16+EXTRA];
		unsigned char *nand_data=vbuff;
		unsigned char gz_data[512+16];
		unsigned char *yaffs_src=gz_data;
		int i;
		int count;
#if defined(CONFIG_GZIP)
		if (isGZip) {
	    	    if (gzRead(&z,gz_data,512+16)!=512+16) {
			putstr("Gzip read error\n");
			return;
		    }
		}
		else
#endif
		    yaffs_src=src;
		    
		// pageStatus and blockStatus bytes
//		yaffs_src[512+4]=0xff;
		if (yaffs_src[512+4]!=0xff) 
		    putLabeledWord("pageStatus =",yaffs_src[512+4]);
//		yaffs_src[512+5]=0xff;
		if (yaffs_src[512+5]!=0xff) {
		    putLabeledWord("Correcting bad blockStatus =",yaffs_src[512+5]);
		    yaffs_src[512+5]=0xff;
		}
		
		// skip bad blocks - detect at boundary
		if ((sector%pagesPerBlock)==0) {
		    unsigned char oob[16];
		    do {
			int numread;
			if (chip->read_oob(chip,sector*512,16,&numread,oob)) {
			    putstr("oob_read failed, aborting\r\n");
			    return;
			}
			if (oob[5]==0xff) {
			    putLabeledWord("Block ok at page 0x",sector);			
			    break;
			}
			// warn block bad
			putLabeledWord("Block bad (skipped) at page 0x",sector);			
			// try next block
			sector+=pagesPerBlock;
		    } while(1);
		}
		
		chip->write_ecc(chip,sector*512,512,&count, yaffs_src,yaffs_src+512,NAND_YAFFS_OOB);
//		nandif_WriteChunkToNAND(chip, sector, yaffs_src, yaffs_src+512);

		// verify - ecc ignored for now.
		chip->read_ecc(chip,sector*512,512,&count,nand_data,nand_data+512,NAND_YAFFS_OOB);
//		nandif_ReadChunkFromNAND(chip,sector,nand_data,nand_data+512);
//		for (i=0; i<512+16; i++) {
		for (i=0; i<512; i++) {
		    if (nand_data[i]!=yaffs_src[i]) {
			int j;
    			putstr("NAND write Verify failure\r\n");
			putLabeledWord("Sector is 0x",sector);
			putLabeledWord("Offset is 0x",i);
			for (j=i; j<i+5; j++) {
			    putLabeledWord("Index      0x",j);
			    putLabeledWord("Downloaded 0x",yaffs_src[j]);
			    putLabeledWord("Read back  0x",nand_data[j]);
			}
			return;
		    }
		}
		
#if defined(CONFIG_GZIP)
		if (!isGZip) 
		    src+=(512+16);
#endif
		img_size-=(512+16);
		sector++;
	    }
	    putstr("YAFFS partition programming Success\r\n");
	}
#endif
#if 0
	else if (!strcmp(argv[1],"check")) {
	    int sector;
	    unsigned char sector_buffer[512];
	    for (sector=0; sector<(32*1024); sector++) {
		if (nand_read_sectors(chip, sector, 1, sector_buffer, 1)) {
		    putstr("nand check: Read sector failed\r\n");
		    putLabeledWord("nand check: Problem in sector 0x", sector);
		    putstr("Dump of first part of sector ...\r\n");
		    for (i=0; i<16; i++) {
			putLabeledWord(" 0x",((unsigned long *)sector_buffer)[i]);
		    }
		    return;
		}
	    }
	    putstr("nand check: chip read passed\r\n");
	}
#endif
	else if (!strcmp(argv[1],"flash")) {

extern int flashDescriptor_MBM29LV400TC_1x16;
extern int flashDescriptor_MBM29LV650UE_1x16;
extern int flashDescriptors;
extern int flashDescriptors_1x16;
	    putLabeledWord("MBM29LV400TC_1x16 is at 0x",&flashDescriptor_MBM29LV400TC_1x16);
	    putLabeledWord("MBM29LV650UE_1x16 is at 0x",&flashDescriptor_MBM29LV650UE_1x16);
	    putLabeledWord("flashDescriptors is at 0x",&flashDescriptors);
	    putLabeledWord("flashDescriptors_1x16 is at 0x",&flashDescriptors_1x16);
	}
	else {
	    putstr("nand command <");
	    putstr(argv[1]);
	    putstr("> unknown\r\n");
	}
    }
}

#endif	

