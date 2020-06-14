// nandcmds.cpp

#include "params.h"
#include "commands.h"
#include "bootldr.h"
#include "serial.h"
#include "btflash.h"
#include "partition.h"
#include "modem.h"
#include "sa1100.h"
#include "heap.h"
#include <string.h>
#if defined(CONFIG_GZIP) || defined(CONFIG_GZIP_REGION)
#include <zlib.h>
#include <zUtils.h>
#endif

#if defined(CONFIG_YAFFS) && defined(CONFIG_NAND)

#include "nand/nandif.h"

#include "yaffs.h"
#include "yaffs_guts.h"
#include "yaffs_mtdif.h"

/* helper functions needed by yaffs source tree */
//char *strncpy(char *dest, char *src, size_t maxlen)
char *yaffs_strncpy(char *dest, char *src, size_t maxlen)
{
   while (maxlen-- && (( *dest++ = *src++) != 0))
	;
   return dest;
}

static char yaffs_partition[100]={0};
static struct yaffs_DeviceStruct yaffs_device = {
    isMounted : 0
};

void yaffs_fs_reset(void) {
    yaffs_partition[0]=0;
}

int yaffs_fs_init(const char *partition_name) {
struct mtd_info *chip=NULL;
struct yaffs_DeviceStruct *dev=&yaffs_device;
int smartmedia=0;
int first_block,end_block;
    
    if (partition_name == "") {
	if (yaffs_partition != NULL)
	    return 0;
	else
	    partition_name="boot";
    }
    else if (!strcmp(partition_name,yaffs_partition))
	return 0;
    else if ((!strcmp(partition_name,"sm")) 
	|| (!strcmp(partition_name,"smartmedia"))
	|| (!strcmp(partition_name,"1"))
	)
	smartmedia=1;

    if (smartmedia) {
	chip = nandif_get(1);
	if (chip) {
	    first_block=0;
	    end_block=(chip->size/chip->erasesize)-1;
	}
    }
    else {
	struct FlashRegion *partition = btflash_get_partition(partition_name);
	if (partition == NULL) {
	    putstr("Unknown partition <"); putstr(partition_name); putstr(">\r\n");
	    return -1;
	}
	if (!partition->flags & LFR_NAND_YAFFS) {
	    putstr("Partition <"); putstr(partition_name); putstr("> is not yaffs\r\n");
	    return -1;
	}
	chip = nandif_get(0);
	if (chip) {
	    first_block=partition->base/chip->erasesize;
	    end_block=first_block+(partition->size/chip->erasesize)-1;
	}
    }

    if (!chip) {
	putstr("Nand device not found\r\n");
	return -1;
    }

    // unmount previous filesystem
    if (dev->isMounted)
	yaffs_Deinitialise(dev);
    dev->isMounted=0;
    dev->genericDevice = chip;
    dev->startBlock=first_block;
    dev->endBlock=end_block;
	// don't use block  0
//	if (dev->startBlock == 0) {
	    dev->startBlock++;
//	}
//    putLabeledWord("Start Block = 0x",dev->startBlock);
//    putLabeledWord("End Block   = 0x",dev->endBlock);
    dev->writeChunkToNAND = nandmtd_WriteChunkToNAND;
//	dev->writeChunkToNAND = NULL;
    dev->readChunkFromNAND = nandmtd_ReadChunkFromNAND;
    dev->eraseBlockInNAND = nandmtd_EraseBlockInNAND;
    dev->initialiseNAND = nandmtd_InitialiseNAND;

    if (yaffs_GutsInitialise(dev) != YAFFS_OK) {
	putstr("YAFFS Guts Initialise *FAIL*\r\n");
	return -1;
    }
    putstr("YAFFS Guts Initialised\r\n");
    strcpy(yaffs_partition,partition_name);
    return 0;
    
}

static yaffs_Object *yaffs_root(char *partition_name) {
    if (yaffs_fs_init(partition_name))
	return NULL;
    return yaffs_Root(&yaffs_device);
}

// NB yaffs_follow_path is destructive of the source
static yaffs_Object *yaffs_follow_path(char **path) {
    yaffs_Object *dir_obj;
    char *slashptr;
    char *partition_name="boot";
    char *colon=strchr(*path,':');
    if (colon) {
	partition_name=*path;
	*path=colon+1;
	*colon=0;
    }
    dir_obj=yaffs_root(partition_name);
    if (!dir_obj)
	return NULL;
    // skip leading slash
    if (**path=='/') {
	(*path)++;
    }
    if (**path) {
	// now travel down list
	while (dir_obj && ((slashptr=strchr(*path,'/')) != NULL)) {
	    char *directory_name=*path;
	    yaffs_Object *obj;
	    *path=slashptr+1;
	    *slashptr=0;
	    if (!strcmp(directory_name,"."))
		continue;
	    else if (!strcmp(directory_name,".."))
		dir_obj = dir_obj->parent;
	    else {
		obj=yaffs_FindObjectByName(dir_obj,directory_name);
		if (obj && (obj->variantType == YAFFS_OBJECT_TYPE_DIRECTORY))
		    dir_obj=obj;
		else if (!obj)
		    return NULL;
		else
		    break;
	    }
	}
    }
    return dir_obj;
}

int yaffs_fs_ls(const char *path) {
    yaffs_Object *obj;
    char buffer[100];	
    char *bptr=buffer;
    int l;
    buffer[0]=0;
    strncat(buffer,path,99);
    l=strlen(buffer);
    if((l==0) || (buffer[l-1]!='/')) 
	strcat(buffer,"/");
    obj=yaffs_follow_path(&bptr);
    if (obj && obj->variantType == YAFFS_OBJECT_TYPE_DIRECTORY) {
	struct list_head *i;
	yaffs_Object *l;
	list_for_each(i,&obj->variant.directoryVariant.children) {
	    char name[YAFFS_MAX_NAME_LENGTH+1];
	    char isLink=0;
	    int mode=0;
	    l = list_entry(i, yaffs_Object, siblings);
	    switch (yaffs_GetObjectType(l)) {
		case DT_UNKNOWN : putc('?'); break;
		case DT_REG : putc('-'); break;
		case DT_LNK : putc('l'); isLink=1; break;
		case DT_DIR : putc('d'); break;
		case DT_FIFO : putc('='); break;
		case DT_CHR : putc('c'); break;
		case DT_BLK : putc('b'); break;
		case DT_SOCK : putc('s'); break;
		default : putstr("*** error *** : "); break;
	    }

	    mode=l->st_mode;
	    putc( (mode & S_IRUSR) ? 'r':'-');
	    putc( (mode & S_IWUSR) ? 'w':'-');
	    if (mode & S_ISUID) 
		putc( (mode & S_IXUSR) ? 's':'S');
	    else
		putc( (mode & S_IXUSR) ? 'x':'-');
	    putc( (mode & S_IRGRP) ? 'r':'-');
	    putc( (mode & S_IWGRP) ? 'w':'-');
	    if (mode & S_ISGID) 
		putc( (mode & S_IXGRP) ? 's':'S');
	    else
		putc( (mode & S_IXGRP) ? 'x':'-');
	    putc( (mode & S_IROTH) ? 'r':'-');
	    putc( (mode & S_IWOTH) ? 'w':'-');
	    putc( (mode & S_IXOTH) ? 'x':'-');

	    putDecIntWidth(l->st_uid,6);
	    putDecIntWidth(l->st_gid,5);
		
	    putDecIntWidth(yaffs_GetObjectFileLength(l),10);
	    putstr("  ");
	    yaffs_GetObjectName(l,name,YAFFS_MAX_NAME_LENGTH+1);
	    putstr(name);
	    if (isLink) {
		putstr(" -> ");
		putstr(yaffs_GetSymlinkAlias(l));
	    }
		
	    putstr("\r\n");
	}	
	putstr("Free bytes = 0x"); putDecIntWidth(yaffs_GetNumberOfFreeChunks(&yaffs_device),10); putstr("\r\n");
	return 0;
    }
    else {
	putstr("Directory not found\r\n");
	return -1;
    }
}

static yaffs_Object *yaffs_fopen(const char *path) {
    char dup_path[100];
    char *dup_path_ptr=&dup_path;
    yaffs_Object *dir_obj=NULL;
    strcpy(dup_path,path);
    dir_obj=yaffs_follow_path(&dup_path_ptr);
    if (!dir_obj)
	return NULL;
    return yaffs_FindObjectByName(dir_obj,dup_path_ptr);
}

static int yaffs_fdelete(const char *path) {
    yaffs_Object *file_obj=yaffs_fopen(path);
    if (file_obj && (yaffs_GetObjectType(file_obj) == DT_REG)) {
	if (yaffs_DeleteFile(file_obj)) 
	    putstr("Cannot delete existing file (but I may be lying)\r\n");
	else {
	    putstr("Deleted <"); putstr(path); putstr(">\r\n");
	    return 0;
	}
    }
    return -1;
}

static yaffs_Object *yaffs_fcreate(const char *path, int mode) {
    yaffs_Object *dir_obj=NULL, *file_obj=NULL;
    char dup_path[100];
    char *dup_path_ptr=&dup_path;
    strcpy(dup_path,path);
    yaffs_fdelete(dup_path);
    dir_obj=yaffs_follow_path(&dup_path_ptr);
    if (dir_obj) {
//#define MODE ((6<<6)|(6<<3)|6)
//#define MODE (0100666)
//#define MODE (S_IFREG | 666)
	file_obj = yaffs_MknodFile(dir_obj,dup_path_ptr,S_IFREG | mode,0,0);
	if (!file_obj)
	    putstr("Cannot create file\r\n");
    }
    return file_obj;
}

int yaffs_fs_writefile(const char *path, char *buffer, unsigned int length) {
    yaffs_Object *file_obj=yaffs_fcreate(path,0600);
    if (!file_obj) {
	    putstr("Cannot create file\r\n");
	    return -1;
    }
    if (yaffs_WriteDataToFile(file_obj,buffer, 0, length) != length) {
	    putstr("Cannot write file\r\n");
	    return -2;
    }
    yaffs_FlushFile(file_obj,0);
    return 0;
}

void *yaffs_fs_readfile(const char *path, void *dest, size_t dest_length, size_t *read_length) {
    yaffs_Object *file_obj=yaffs_fopen(path);
    size_t len;
    if (!file_obj) 
	return NULL;
    if (read_length)
	*read_length=0;
    len=yaffs_GetObjectFileLength(file_obj);
    // not enough space?
    if (dest_length && (dest_length<len))
	return NULL;
    if (!dest)
	dest=malloc(len);
    if (!dest)
	return NULL;
    len=yaffs_ReadDataFromFile(file_obj,dest,0,len);
    if (read_length)
	*read_length=len;
    return dest;
}

int yaffs_fs_read(void *obj, char *buffer, unsigned int offset, unsigned int len) {
    int retlen=yaffs_ReadDataFromFile((yaffs_Object *)obj,buffer,offset,len);
    return retlen;
}

int yaffs_fs_write(void *obj, char *buffer, unsigned int offset, unsigned int len) {
    int retlen=yaffs_WriteDataToFile((yaffs_Object *)obj,buffer,offset,len);
    return retlen;
}

int yaffs_fs_file_length(void *obj) {
    return yaffs_GetObjectFileLength((yaffs_Object *)obj);
}

void *yaffs_fs_open(const char *path) {
    yaffs_Object *obj=yaffs_fopen(path);
    return obj;
}

COMMAND(yaffs, command_yaffs, "[command] -- reset ls cat rm and write", BB_RUN_FROM_RAM);
void command_yaffs(int argc, const char *argv[]) {
struct yaffs_DeviceStruct *dev=&yaffs_device;
    yaffs_Object *obj;

    if (argc<2) {
	putstr("yaffs ls | cat | rm | write path\r\n");
	return;
    }
	

    if (!strcmp(argv[1],"reset")) {
	yaffs_fs_reset();
    }
    else if (!strcmp(argv[1],"ls")) {
	yaffs_fs_ls((argc>2) ? argv[2]:"");
    }
    else if (!strcmp(argv[1],"cat")) {
	if (argc>2) {
	    char buffer[100];	
	    char *bptr=buffer;
	    int flength=0;
	    int foffset=0;
	    strcpy(buffer,argv[2]);
	    obj=yaffs_fopen(bptr);
	    if (!obj) {
		putstr("Invalid filename "); putstr(bptr); putstr("\r\n");
		return;
	    }
	    if (!(yaffs_GetObjectType(obj) == DT_REG)) {
		putstr("Not a normal file\r\n");
		return;
	    }
	    
	    flength=yaffs_GetObjectFileLength(obj);
	    do {
		char ch;
		if (yaffs_ReadDataFromFile(obj,&ch,foffset,1) != 1) {
		    putstr("Error reading file\r\n");
		    return;
		}
		foffset++;
		if (ch=='\r')
		    ;
		else if (ch == '\n')
		    putstr("\r\n");
		else if (((ch<32) || (ch>127)) && (ch != '\t'))
		    putc('?');
		else 
		    putc(ch);
	    } while(foffset<flength);
	    putstr("\r\n");
	}
	else
	    putstr("Must supply filename\r\n");
		
    }
    else if (!strcmp(argv[1],"read")) {
	if (argc>2) {
	    unsigned long img_dest = VKERNEL_BASE + 1024; /* just a temporary holding area */
	    unsigned long regionSize=0x1000000;	/* 16MB should be ok for most things */
	    int img_size = 0;
	    int ymodem = 0;
//#ifdef CONFIG_YMODEM
//	    ymodem = param_ymodem.value;
//#endif
	    if (!yaffs_fs_readfile(argv[2],(char *)img_dest,regionSize,&img_size)) {
		putstr("File read error\r\n");
		return;
	    }
	    else
		putstr("File read ok\r\n");

	    if (!img_size) {
    		putstr("File size is zero, nothing to send.\r\n");
    		return;
	    }
	    putLabeledWord("Sending file size = 0x",img_size);
//	    if (ymodem) {
//#ifdef CONFIG_YMODEM
//		putstr("using ymodem\r\n");
//    		ymodem_send((char*)img_dest, img_size);
//#endif
//	    } else {
    		putstr("using xmodem\r\n");
    		modem_send((char*)img_dest, img_size);
//	    }
	}
	else
	    putstr("Filename not supplied\r\n");
    }
    else if (!strcmp(argv[1],"write")) {
	if (argc>2) {
	    unsigned long img_dest = VKERNEL_BASE + 1024; /* just a temporary holding area */
	    unsigned long regionSize=0x1000000;	/* 16MB should be ok for most things */
	    int img_size = 0;
	    int ymodem = 0;
	    char path[100];	
	    strcpy(path,argv[2]);
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
	    {
	    if (yaffs_fs_writefile(path,(char *)img_dest,img_size))
		putstr("File write error\r\n");
	    else
		putstr("File created ok\r\n");
	    }
	}
	else
	    putstr("Filename not supplied\r\n");
    }
    else if (!strcmp(argv[1],"cp")) {
	if (argc>3) {
	    unsigned long img_dest = VKERNEL_BASE + 1024; /* just a temporary holding area */
	    unsigned long regionSize=0x1000000;	/* 16MB should be ok for most things */
	    int img_size = 0;
	    int ymodem = 0;
	    char source_path[100];	
	    char source_path_copy[100];	
	    char dest_path[100];	
	    void *yaffs_file;
	    strcpy(source_path,argv[2]);
	    strcpy(source_path_copy,source_path);
	    strcpy(dest_path,argv[3]);
	    yaffs_file=yaffs_fs_open(source_path);
	    if (yaffs_file) {
		img_size=yaffs_fs_file_length(yaffs_file);
		putstr("Reading file <"); putstr(source_path_copy); putLabeledWord("> length 0x",img_size);
		if (yaffs_fs_read(yaffs_file,(char *)img_dest,0,img_size)!=img_size) {
		    putstr("Error reading file\r\n");
		}
		putstr("Writing file <"); putstr(dest_path); putstr(">\r\n");
		if (yaffs_fs_writefile(dest_path,(char *)img_dest,img_size))
		    putstr("Error writing file\r\n");
		else
		    putstr("Ok\r\n");
	    }
	    else {
		putstr("Cannot open <"); putstr(source_path_copy); putstr(">\r\n");
		return;
	    }
	}
	else
	    putstr("Filenames not supplied\r\n");
    }
    else if (!strcmp(argv[1],"rm")) {
	if (argc>2) {
	    if (yaffs_fdelete(argv[2]))
		putstr("Delete failed\r\n");
	    else
		putstr("File deleted\r\n");
	}
	else
	    putstr("Filename not supplied\r\n");
    }
    else {
	putstr("Command <");
	putstr(argv[1]);
	putstr("> not known \r\n");
    }
}

#endif


