#ifndef	__REGDEV_COMMON_H__
#define	__REGDEV_COMMON_H__
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <paths.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>


#include <linux/types.h>

struct mem_map_s{
	int fd;
	off_t phy_addr;
	void *map_base;
	//void *virt_addr;
	int memmap_size;
};


#define	DEV_MEM_PATH	"/dev/mem"

//void regdev_init(void **dev);
void regdev_init(struct mem_map_s** g_regdev);
int regdev_set_para(void *dev, int phy_addr, int map_size);
int regdev_open(void *dev);
void regdev_close(void *dev);
int regdev_read(void *dev, int reg, int *val);
int regdev_write(void *dev, int reg, int val);
int regdev_phy(void *dev);


#endif
