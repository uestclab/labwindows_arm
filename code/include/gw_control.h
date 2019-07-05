#ifndef GW_CONTROL_H
#define GW_CONTROL_H

#include "zlog.h"
#include <stdint.h>
#include "regdev_common.h"

#define	REG_PHY_ADDR	0x43C20000
#define	REG_MAP_SIZE	0X10000


typedef struct g_RegDev_para{
	struct mem_map_s*  mem_dev_phy; // c2
	zlog_category_t*   log_handler;
}g_RegDev_para;

int initRegdev(g_RegDev_para** g_RegDev, zlog_category_t* handler);


#endif//GW_CONTROL_H
