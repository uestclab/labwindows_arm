#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cJSON.h"
#include "gw_control.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

 /* ----------------------- new interface to read and write register ---------------------------- */

int initRegdev(g_RegDev_para** g_RegDev, zlog_category_t* handler)
{
	zlog_info(handler,"initPhyRegdev()");

	*g_RegDev = (g_RegDev_para*)malloc(sizeof(struct g_RegDev_para));

	(*g_RegDev)->mem_dev_phy = NULL;
	(*g_RegDev)->log_handler = handler;


	int rc = 0;

	regdev_init(&((*g_RegDev)->mem_dev_phy));
	regdev_set_para((*g_RegDev)->mem_dev_phy, REG_PHY_ADDR, REG_MAP_SIZE);
	rc = regdev_open((*g_RegDev)->mem_dev_phy);
	if(rc < 0){
		zlog_info(handler," mem_dev_phy regdev_open err !!\n");
		return -1;
	}

	return 0;
}









