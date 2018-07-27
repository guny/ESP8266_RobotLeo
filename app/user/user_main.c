
#include "ets_sys.h"
#include "osapi.h"
#include "smartconfig.h"
#include "driver/uart.h"
#include "user_interface.h"

#include "user_config.h"
#include "user_linkled.h"
#include "user_device_find.h"
#include "user_navigation_analyze.h"
#include "user_system_config.h"

WIFI_MODULE_T WifiModule;

ICACHE_FLASH_ATTR void WifiModuleInit(void)
{
	WifiModule.DeviceFindFlag = 0;
	WifiModule.NavWorkControl.bodymode = none_sweep;
	WifiModule.NavWorkControl.mode     = auto_sweep;
}
/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}



void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_init(void)
{
	sint8 res = 0;

	system_update_cpu_freq(SYS_CPU_160MHZ);
	user_link_led_timer_done();// Wi-Fi Link LED : CLOSE

    uart_init(115200, 115200);

//	uint8 report_buff[3072];
//    os_memset(report_buff,0,sizeof(report_buff));
//    os_memset(report_buff+os_strlen(report_buff),'1',(64*32)+23);
//    os_printf("\r\nthe length is %u\r\n",os_strlen(report_buff));

	user_devicefind_init(USER_LOCATION_PORT);
	user_webserver_init(USER_LOCATION_PORT);

	WifiModuleInit();

	wifi_module_set_wifi_status(wifi_searching);

#ifdef SMARTCONFIG_ENABLE
	wifi_set_opmode(STATION_MODE);

	if(0 == user_config_load())
		user_smartconfig();
	else
		user_config_to_connect();

	user_ser_hostname();
#else
	wifi_set_opmode(STATIONAP_MODE);
#endif

	//navigation_charge_point_upload_loop();
	//wifi_navigation_handclasp_handler_enable();
}

