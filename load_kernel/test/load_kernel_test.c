typedef unsigned long	u32;
typedef unsigned short	u16;
typedef	unsigned char	u8;
#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<unistd.h>
#include<libgen.h>
#include "load_kernel.h"

int main(int argc, char *argv[])
{
	char *out;
	FILE *fp;
	struct stat st;
	struct part_info part;
	int i;
	unsigned long size = 0;
	
	part.erasesize = 0x20000;
	fodder_ram_base = malloc(0x1000);
	out = malloc(0xe00000);

	if (argc < 2) {
		printf("not enough arguments usage:\n");
		printf("%s: <fs_image> <outfile>\n", basename(argv[0]));
		return 0;
	}
	
	if (stat(argv[1], &st)) {
		perror("could not stat fs_image");
		return 0;
	}
	
	if (!(fp = fopen(argv[1], "r"))) {
		perror("could not open fs_image");
		return 0;
	}
	
	part.size = st.st_size + part.erasesize - ((st.st_size - 1) % part.erasesize) - 1;
	part.offset = malloc(part.size);
	fread(part.offset, 1, st.st_size, fp);
	memset(part.offset + st.st_size, 0xff, part.size - st.st_size);
	fclose(fp);

	printf("flash emulation region is 0x%lx bytes\n", part.size);

	for (i = 0; loader[i] && !loader[i]->check_magic(&part); i++);
	if (!loader[i]) {
		printf("unable to find magic\n");
	} else {
		printf("loading kernel from ");
		printf(loader[i]->name);
		if ((size = loader[i]->load_kernel((u32 *) out, &part)) == 0) {
			printf(" error loading kernel!\n");
			return 0;
		}
	}
	printf("loaded 0x%08lx bytes\n", size);
	
	if (!(fp = fopen(argv[2], "w"))) {
		perror("could not open outfile");
		return 0;
	}
	
	fwrite(out, 1, size, fp);
	fclose(fp);
	
	return 0;
}
