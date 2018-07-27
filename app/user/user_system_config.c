
#include "osapi.h"
#include "spi_flash.h"
#include "user_interface.h"
#include "user_system_config.h"

typedef struct {
	uint8 flag;
	uint8 pad[3];
}SAVE_FLAG;

SAVE_FLAG   SaveFlag;
USER_INFO_T UserConfigInfo;

#define _USER_INFO_CFG_FLAG    (0x00005AA5)
#define USER_INFO_LOCATION     (0x79)

ICACHE_FLASH_ATTR void user_config_save(void)
{
	spi_flash_read((USER_INFO_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
		           (uint32 *)&SaveFlag, sizeof(SAVE_FLAG));

	if(0 == SaveFlag.flag)
	{
		spi_flash_erase_sector(USER_INFO_LOCATION + 1);
		spi_flash_write((USER_INFO_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
			            (uint32 *)&UserConfigInfo, sizeof(USER_INFO_T));

		SaveFlag.flag = 1;
		spi_flash_erase_sector(USER_INFO_LOCATION + 3);
		spi_flash_write((USER_INFO_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
			            (uint32 *)&SaveFlag, sizeof(SAVE_FLAG));
	}
	else
	{
		spi_flash_erase_sector(USER_INFO_LOCATION + 0);
		spi_flash_write((USER_INFO_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
			            (uint32 *)&UserConfigInfo, sizeof(USER_INFO_T));

		SaveFlag.flag = 0;
		spi_flash_erase_sector(USER_INFO_LOCATION + 3);
		spi_flash_write((USER_INFO_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
			            (uint32 *)&SaveFlag, sizeof(SAVE_FLAG));
	}
}

ICACHE_FLASH_ATTR uint8 user_config_load(void)
{
	spi_flash_read((USER_INFO_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
		           (uint32 *)&SaveFlag, sizeof(SAVE_FLAG));

	if(0 == SaveFlag.flag)
	{
		spi_flash_read((USER_INFO_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
			            (uint32 *)&UserConfigInfo, sizeof(USER_INFO_T));
	}
	else
	{
		spi_flash_read((USER_INFO_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
			            (uint32 *)&UserConfigInfo, sizeof(USER_INFO_T));
	}

	if(_USER_INFO_CFG_FLAG == UserConfigInfo.cfg_holder)
		return 1;
	else
		return 0;
}

ICACHE_FLASH_ATTR void user_config_set(struct station_config* config)
{
	os_memset((uint32 *)&UserConfigInfo, 0, sizeof(USER_INFO_T));
	os_memcpy(UserConfigInfo.sta_ssid, config->ssid, os_strlen(config->ssid));
	os_memcpy(UserConfigInfo.sta_pwd,  config->password, os_strlen(config->password));

	UserConfigInfo.cfg_holder = _USER_INFO_CFG_FLAG;

	user_config_save();
}

ICACHE_FLASH_ATTR void user_config_clean_all(void)
{
	spi_flash_erase_sector(USER_INFO_LOCATION + 0);
	spi_flash_erase_sector(USER_INFO_LOCATION + 1);
	spi_flash_erase_sector(USER_INFO_LOCATION + 3);
}