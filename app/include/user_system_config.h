
#ifndef __USER_SYSTEM_CONFIG_H__
#define __USER_SYSTEM_CONFIG_H__

#include "c_types.h"

typedef struct {
	uint32 cfg_holder;
	uint8 sta_ssid[64];
	uint8 sta_pwd[64];
} USER_INFO_T;
extern USER_INFO_T UserConfigInfo;

ICACHE_FLASH_ATTR void user_config_save(void);
ICACHE_FLASH_ATTR uint8 user_config_load(void);

#endif //__USER_SYSTEM_CONFIG_H__
