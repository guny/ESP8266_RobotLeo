
#ifndef __USER_WEBSERVER_H__
#define __USER_WEBSERVER_H__

struct WIFI_PORT_DATA_t{
	uint32 length;
	uint8  framer[2048];
	uint8* info;
};

typedef enum {
	WIFI_RECEIVER_LENGTH = 0,
	WIFI_RECEIVER_STRING = ~WIFI_RECEIVER_LENGTH,
}WIFI_PORT_STATUS;

ICACHE_FLASH_ATTR void user_webserver_init(uint32 port);

#endif //__USER_WEBSERVER_H__