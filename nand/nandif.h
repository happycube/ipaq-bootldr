/* nandif.h - nand flash interface */

#include "bootconfig.h"

#ifdef	CONFIG_NAND
#include "nand.h"

int nandif_init(void);
int nandif_enum(void);
struct mtd_info *nandif_get(int index);

void nandif_jffs2_format_region(struct mtd_info *chip, unsigned long base, unsigned long size, 
                                 unsigned long marker0, unsigned long marker1, unsigned long marker2);

#if 0
#ifdef	CONFIG_YAFFS
int nandif_WriteChunkToNAND(struct mtd_info *chip, int sector, unsigned char *data, unsigned char *spare);
int nandif_ReadChunkFromNAND(struct mtd_info *chip, int sector, unsigned char *data, unsigned char *spare);
#endif
#endif

#endif
