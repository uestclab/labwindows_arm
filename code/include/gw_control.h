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

// 0x43c2010c,bit 7 enable constellation,bit6 enable csi , bit6 and bit7 not both enable
int switchTocsi(g_RegDev_para* g_RegDev);
int switchToconstellation(g_RegDev_para* g_RegDev);


#endif//GW_CONTROL_H
