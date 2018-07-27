
#ifndef __USER_APP_ANALYZE_H__
#define __USER_APP_ANALYZE_H__

ICACHE_FLASH_ATTR void navigation_electric(uint32 electric);
ICACHE_FLASH_ATTR void navigation_ctime(uint32 hour, uint32 minute);
ICACHE_FLASH_ATTR void navigation_carea(uint32 area);
ICACHE_FLASH_ATTR void navigation_pos(sint16 x, sint16 y, sint16 angle);

ICACHE_FLASH_ATTR void navigation_dormancy_timer_init(void);

ICACHE_FLASH_ATTR sint8 user_app_command_analyze(struct WIFI_PORT_DATA_t AppCommand);

#endif //__USER_APP_ANALYZE_H__