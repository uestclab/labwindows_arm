#include "regdev_common.h"

//struct mem_map_s g_regdev;

/*
void regdev_init(void **dev)
{
	*dev = (void*)&g_regdev;
	memset((void*)&g_regdev, 0, sizeof(struct mem_map_s));
	g_regdev.fd = -1;
	g_regdev.map_base = MAP_FAILED;

}
*/

void regdev_init(struct mem_map_s** g_regdev)
{
	*g_regdev = (struct mem_map_s*)malloc(sizeof(struct mem_map_s));

	memset((void*)(*g_regdev), 0, sizeof(struct mem_map_s));
	(*g_regdev)->fd = -1;
	(*g_regdev)->map_base = MAP_FAILED;

}

int regdev_set_para(void *dev, int phy_addr, int map_size)
{
	int ret = 0;
	struct mem_map_s *mp = (struct mem_map_s*)dev;

	mp->phy_addr = phy_addr;
	mp->memmap_size = map_size;

	return ret;
}

int regdev_open(void *dev)
{
	int ret = 0;
	struct mem_map_s *mp = (struct mem_map_s*)dev;
	
	unsigned page_size, mapped_size, offset_in_page;
	unsigned width = 8 * sizeof(int);

	if(NULL == mp){
		ret = -EINVAL;
		goto exit;
	}
	
	mp->fd = open(DEV_MEM_PATH,O_RDWR | O_SYNC, 0666);
	if (mp->fd < 0) {
		ret = mp->fd;
		goto exit;
	}

	mapped_size = mp->memmap_size;
	page_size = getpagesize();
	offset_in_page = (unsigned)mp->phy_addr & (page_size - 1);
	
	if (offset_in_page + width > page_size) {
		/* This access spans pages.
		 * Must map two pages to make it possible: */
		mapped_size += page_size;
	}
	
	mp->map_base = mmap(NULL,
				mapped_size,
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				mp->fd,
				mp->phy_addr & ~(off_t)(page_size - 1));

	if (mp->map_base == MAP_FAILED){
		ret = -EINVAL;
		goto exit;
	}

	mp->memmap_size = mapped_size;
	//mp->virt_addr = (char*)(mp->map_base) + offset_in_page;
exit:
	return ret;

}

void regdev_close(void *dev)
{
	struct mem_map_s *mp = (struct mem_map_s*)dev;
	
	if(dev && (MAP_FAILED != mp->map_base)){
		munmap(mp->map_base, mp->memmap_size);
	}
	
	if(dev && (mp->fd >= 0)){
		close(mp->fd);
	}	
}

int regdev_read(void *dev, int reg, int *val)
{
	int ret = 0;
	struct mem_map_s *mp = (struct mem_map_s*)dev;
	void *virt_addr;

	if(reg > 0xFFFB){ //exceed 64KB
		ret = -EIO;
		goto exit;
	}
	
	virt_addr =  (char*)(mp->map_base) + reg;
	
	*val = *(volatile uint32_t*)(virt_addr);
exit:
	return ret;
}

int regdev_write(void *dev, int reg, int val)
{
	int ret = 0;
	
	struct mem_map_s *mp = (struct mem_map_s*)dev;
	void *virt_addr;

	if(reg > 0xFFFB){ //exceed 64KB
		ret = -EIO;
		goto exit;
	}

	virt_addr =  (char*)(mp->map_base) + reg;
	
	*(volatile uint32_t*)(virt_addr) = val;	

exit:
	return ret;
}

int regdev_phy(void *dev)
{
	struct mem_map_s *mp = (struct mem_map_s*)dev;
	return mp->phy_addr;
}


