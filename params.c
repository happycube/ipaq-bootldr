#define REGISTER_PARAMS
#include "params.h"
#include "btflash.h"
#include "cyclone_boot.h"
#include "commands.h"
#include "serial.h"
#include "heap.h"
#include "util.h"
#include <string.h>

extern volatile unsigned long *flashword; 
extern FlashDescriptor *flashDescriptor;
extern struct bootblk_param param_initrd_filename;

//
// FIXME: We aren't using this at all.
//
// static struct ebsaboot bootinfo;
//

void params_eval_partition(const char* prefix_in, int just_show);



struct bootblk_param *get_param(const char *name)
{
   struct bootblk_param *param = (struct bootblk_param *)&__params_begin;
   while (param < (struct bootblk_param *)&__params_end) {
      int namelen = strlen(param->name);
      if (strncmp(name, param->name, namelen) == 0
          && name[namelen] == 0) {
         return param;
      }
      param++;
   }
   putstr("get_param: could not find parameter "); putstr(name); putstr("\r\n");
   return NULL;
}

void params_eval(
    const char*   prefix_in,
    int	    just_show)
{
  unsigned char* p;
  struct BootldrFlashPartitionTable *pt = 0;
  int partition_table_size = 0;
  // unsigned long ptable_addr = 0;

 
  putstr("\r\nparams_eval: prefix_in =");
  putstr(prefix_in);
  putLabeledWord("\r\nparams_eval: just_show =",just_show);

  // XXX this presumes that the params sector follows the bootloader sector!!
  p = ((char*)flashword) + flashDescriptor->bootldr.base + flashDescriptor->bootldr.size;
  

  pt = (struct BootldrFlashPartitionTable *)p;
  if (pt->magic == BOOTLDR_PARTITION_MAGIC) {
    params_eval_partition(prefix_in,just_show);
  }else if (isWincePartition(p)) {
    /* guess that wince is still installed and do not look for params */
  } else {
      params_eval_file(just_show);
  }
  // now we have our new params, lets copy them to the right place.
  // ptable_addr = param_ptable_addr.value;
  partition_table_size = (sizeof(struct BootldrFlashPartitionTable) 
			  + partition_table->npartitions*sizeof(struct FlashRegion));

  //memcpy(ptable_addr,partition_table,partition_table_size);
  
}

void params_eval_partition(
    const char*   prefix_in,
    int	    just_show)
{
  unsigned char* p;
  unsigned char* endp;
  const char* prefix;
  char cmdbuf[1024];
  char* cmdp;
  struct BootldrFlashPartitionTable *pt = 0;
  int partition_table_size = 0;
  int i;
  
  putstr("eval param blk\r\n");
  // presumes that the params sector follows the bootloader sector and is no larger!
  p = ((char*)flashword) + flashDescriptor->bootldr.base + flashDescriptor->bootldr.size;
  endp = p + flashDescriptor->bootldr.size;

  pt = (struct BootldrFlashPartitionTable *)p;
  if (pt->magic == BOOTLDR_PARTITION_MAGIC) {
    putstr("found partition table in params sector\r\n");
    putLabeledWord("pt->npartitions=", pt->npartitions);
    partition_table_size = 
      (sizeof(struct BootldrFlashPartitionTable) 
       + pt->npartitions * sizeof(struct FlashRegion));
    putLabeledWord("partition_table_size=", partition_table_size);
    partition_table->npartitions = 0;
    for (i = 0; i < pt->npartitions; i++) {
      struct FlashRegion *partition = &pt->partition[i];
      btflash_define_partition(partition->name, partition->base, partition->size, partition->flags);
    }
  }
  p += partition_table_size;

  /* stops at end of sector or first unwritten byte */
  while (p < endp && *p != 0xff && *p != 0) {
#ifdef DEBUG_COMMAND_PARAMS_EVAL
    putLabeledWord("top of loop, p=", (dword)p);
#endif
    prefix = prefix_in;
    while (p < endp && *prefix && *p == *prefix) {
      p++;
      prefix++;
    }
#ifdef DEBUG_COMMAND_PARAMS_EVAL
    putLabeledWord("past prefix check, p=", (dword)p);
#endif

    if (*prefix != '\0') {
#ifdef DEBUG_COMMAND_PARAMS_EVAL
      putstr("no prefix match\r\n");
#endif

      skip_to_eol: 
      /* skip to end of line */
      while (p < endp && *p != '\n')
	p++;

#ifdef DEBUG_COMMAND_PARAMS_EVAL
      putLabeledWord("skipped line, p=", (dword)p);
      putLabeledWord("char at p=", (dword)(*p) & 0xff);
#endif
      p++;
      continue;
    }

    /* copy line to buffer */
    /* terminate with eof, or eol */
    cmdp = cmdbuf;
    while (p < endp && *p != 0xff) {
      if (*p == '\r' || *p == '\n') {
	p++;
	if (*p == '\r' || *p == '\n') 
	  p++;
	break;
      }
      if ((cmdp - cmdbuf) >= sizeof(cmdbuf) - 1)
	  goto skip_to_eol;
      *cmdp++ = *p++;
    }
    *cmdp = '\0';
    cmdp = cmdbuf;

    putstr("+");
    putstr(cmdbuf);
    putstr("\r\n");

    if (just_show)
	continue;

    exec_string(cmdbuf);
    
  }
}

SUBCOMMAND(params, eval, command_params_eval, "[-n] -- shows the parameters", BB_RUN_FROM_RAM, 1);
void command_params_eval(int argc, const char **argv)
{
  int	just_show = 0;
  const char*	prefix = PARAM_PREFIX;
    
  if (argc > 1) {
      just_show = (strncmp(argv[1], "-n", 2) == 0);
      if (just_show) {
	  if (argc > 2)
	      prefix = argv[2];
      }
      else
	  prefix = argv[1];
  }
  
  params_eval(prefix, just_show);
}

extern void command_params_show(int argc, const char **argv); 
COMMAND(params, command_params, "-- displays the parameters", BB_RUN_FROM_RAM);
void command_params(int argc, const char **argv)  
{
  command_params_show(argc, argv);
}

SUBCOMMAND(params, reset, command_params_reset, "-- sets the parameters to default values", BB_RUN_FROM_RAM, 0);
void command_params_reset(int argc, const char **argv)
{
  putstr("setting params to default values\r\n");
  putLabeledWord("flashword = ",(long)flashword);
  putLabeledWord("bootldr_params = ",(long)&__params_begin);
  putLabeledWord("FLASH_BASE = ",FLASH_BASE);
  putLabeledWord("sizeof(bootldr_params) = ", &__params_end - &__params_begin);
  memcpy((char*)&__params_begin, (const char*)(((char*)flashword) + (((dword)&__params_begin) - FLASH_BASE)), &__params_end - &__params_begin);
  return;
}


int set_param(struct bootblk_param *param, char *pValue)
{
  if (!param) return 0;
  {
    const char *paramValueString;
    long paramoldvalue = (long)pValue;
    long value;
    int paramValueLen;
    paramValueString = pValue;
    if (paramValueString == NULL)
      paramValueString = "";
    paramValueLen = strlen(paramValueString);
    putstr("  setting param <");
    putstr(param->name);
    putstr("> to value <");
    putstr(paramValueString);
    putstr(">\r\n");
    switch (param->paramFormat) {
    case PF_STRING: {
      if (paramValueLen == 0) {
	value = 0;
      } else {
	/* should we free the memory? */
	/* if (param->value) mfree(param->value); */
	char *newstr = (char *) mmalloc(paramValueLen+1);
	memcpy(newstr, paramValueString, paramValueLen+1);
	value = (long)newstr;
      }
      break;
    }
    case PF_LOGICAL:
      paramoldvalue = param->value;
      value = strtoul(paramValueString, NULL, 0);
    case PF_DECIMAL:
    case PF_HEX:
      
    default:
      paramoldvalue = param->value;
      value = strtoul(paramValueString, NULL, 0);
      
    }
    param->value = value;
    if (param->update != NULL) {
      (param->update)(param, paramoldvalue);
    }
    return 1;
  }
}

int set_param_by_name(const char *pName, char *pValue)
{
  return set_param(get_param(pName), pValue);
}

#define DEBUG_COMMAND_PARAMS_EVAL 1 


COMMAND(set, command_set, "<param>=<value>", BB_RUN_FROM_RAM);
void command_set(int argc, const char **argv)
{
   char pName[256] = "";
   char pValue[256] = "";

   parseParamName(argc,argv,pName,pValue);
   
   if ((argc == 1) && (pName[0] = '\0')) {
       /* no argses */
       int my_argc = 1;
       char *my_argv[128];
       my_argv[0] = "show";
       my_argv[1] = NULL;
       do_command(my_argc, (const char **) my_argv);
       return;
   }

   set_param_by_name(pName, pValue);
}

   

COMMAND(show, command_params_show, "", BB_RUN_FROM_RAM);
SUBCOMMAND(params, show, command_params_show, "-- show parameters", BB_RUN_FROM_RAM, 1);
void command_params_show(int argc, const char **argv)
{
   struct bootblk_param *params = (struct bootblk_param *)&__params_begin;
   int i;
   unsigned char showMe;
   
   print_version_verbose(">> ");

   while (params < (struct bootblk_param *)&__params_end) {
       showMe = 0;
       if (argc > 1){
	   for (i=1;i < argc; i++)
	       if (!strcmp(params->name,argv[i]))
		   showMe = 1;
       }
       else
	   showMe = 1;
       if (showMe){
	   putstr("  ");
	   putstr(params->name);
	   switch (params->paramFormat) {
	       case PF_STRING:
		   putstr("= <"); putstr((char*)params->value); putstr(">\r\n");
		   break;
	       case PF_DECIMAL: {
		   char buf[16];
		   dwordtodecimal(buf, params->value);
		   putstr("=<"); putstr(buf); putstr(">\r\n");
		   break;
	       }
	       case PF_HEX:
	       default:
		   putstr("=<0x"); putHexInt32(params->value); putstr(">\r\n");
		   break;
	   }
       }
       params++;
   }
}

int
param_is_modified(
    const struct bootblk_param*	param,
    const struct bootblk_param* def_param)
{
    /*
     * compare params, taking into account the fact that
     * a string can have a different address and the same
     * value.
     */
    
    if (param->paramType == PT_STRING) {
	if (param->value && def_param->value) {
	    return (strcmp((char*)(param->value),
			   (char*)(def_param->value)) != 0);
	}
	else {
	    /*
	     * we know both aren't non-null.
	     * if both are then that is a match
	     */
	    return (param->value || def_param->value);
	}
    }
    else {
	return (param->value != def_param->value);
    }
    
}

extern void program_flash_region(const char *regionName, 
                                 unsigned long regionBase, size_t regionSize, 
                                 unsigned long src, size_t img_size, int flags);

SUBCOMMAND(params, save, command_params_save, "[-n] -- write params to partition, if exists", BB_RUN_FROM_RAM, 1);
void command_params_save(int argc, const char **argv)
{
   char param_buf[SZ_16K];
   char *buf = param_buf;
   int partition_table_size = 0;
   int dont_save = 0;
   struct FlashRegion *paramsPart;
   struct bootblk_param *param = (struct bootblk_param *)&__params_begin;
   /* default params points into the flash image as opposed to the copy in dram */
   const struct bootblk_param *defaultParam = 
      (const struct bootblk_param *)(((char*)flashword) + (((dword)&__params_begin) - FLASH_BASE));

   /* should pick up the old params from flash in case there are any non-bootldr lines */

   memset(param_buf, 0, sizeof(param_buf));

   partition_table_size = (sizeof(struct BootldrFlashPartitionTable) 
                           + partition_table->npartitions*sizeof(struct FlashRegion));
   memcpy(buf, partition_table, partition_table_size);

   buf += partition_table_size;

   if (argc > 1 && strcmp(argv[1], "-n") == 0) {
       PLW(flashword);
       PLW(&__params_begin);
       PLW(FLASH_BASE);
       PLW(defaultParam);
       dont_save = 1;
   }

   /* construct the parameter setting code */
   while (param < (struct bootblk_param *)&__params_end) {
      int modifiedFromDefault = param_is_modified(param,
						  defaultParam); 
      if (modifiedFromDefault && !(param->paramType&PT_READONLY)) {
         strcat(buf, "bootldr: set ");
         strcat(buf, param->name);
         strcat(buf, " ");
         switch (param->paramFormat) {
         case PF_STRING:
	    strcat(buf, "\"");
            if(param->value)
	      strcat(buf, (char *)param->value);
	    strcat(buf, "\"");
            break;
         case PF_DECIMAL: {
           char num[32];
           dwordtodecimal(num, param->value);
           strcat(buf, num);
         } break;
         case PF_HEX:
         default: {
            char num[16];
            strcat(buf, "0x");
            binarytohex(num, param->value,4);
            strcat(buf, num);
         }         
         }
         strcat(buf, "\r\n");
      }
      param++;
      defaultParam++;
   }
   putstr(buf);

   if (dont_save)
       putstr("Not erasing and writing params sector.\n\r");
   else {
       /* now erase and program the params sector */
       paramsPart = btflash_get_partition("params");
       if (paramsPart)
	   program_flash_region("params", paramsPart->base,
				paramsPart->size,
				(dword)param_buf,
				partition_table_size + strlen(buf)+1,
				paramsPart->flags);
   
       else {
	   putstr("No params partition found.  I cannot save params into Wince or JFFS2 files!!!\r\n");
   }
   }
}

