
#include "osapi.h"
#include "os_type.h"
#include "ip_addr.h"
#include "user_interface.h"

#include "user_wifi_connect.h"

LOCAL os_timer_t test_timer;
 
/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
ICACHE_FLASH_ATTR void user_esp_platform_check_ip(void)
{
    struct ip_info ipconfig;
 
   //disarm timer first
    os_timer_disarm(&test_timer);
 
   //get ip info of ESP8266 station
    wifi_get_ip_info(STATION_IF, &ipconfig);
 
    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) 
	{
    	//os_printf("got ip %d !!! \r\n", wifi_station_get_rssi()); 
    }
	else
	{
		if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
			 wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
			 wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) 
		{
			wifi_station_connect();
			//os_printf("connect fail !!! \r\n");
		}
    }

	//os_printf("timestamp : %u\n\r", system_get_time());

	//re-arm timer to check ip
	os_timer_setfn(&test_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
	os_timer_arm(&test_timer, 100, 0);
}
/*
ICACHE_FLASH_ATTR void user_mdns_conf(void)
{
	struct ip_info ipconfig;

	wifi_get_ip_info(STATION_IF, &ipconfig);

	struct mdns_info *info = (struct mdns_info *)os_zalloc(sizeof(struct mdns_info));

	info->host_name = "robotleo-sweeper";
	info->ipAddr= ipconfig.ip.addr; //sation ip
	info->server_name = "robotleo-xmpp";
	info->server_port = 14001;
	info->txt_data[0] = "version = 1.0.1";

	espconn_mdns_init(info);
}*/
 
/******************************************************************************
 * FunctionName : user_set_station_config
 * Description  : set the router info which ESP8266 station will connect to 
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
ICACHE_FLASH_ATTR void user_set_station_config(uint8* SSID, uint8* PASSWORD)
{
	// Wifi configuration 
	struct station_config stationConf;

	os_memset(stationConf.ssid, 0, 32);
	os_memset(stationConf.password, 0, 64);
	//need not mac address
	stationConf.bssid_set = 0; 

	//Set ap settings 
	os_sprintf(stationConf.ssid, "%s", SSID); 
	os_sprintf(stationConf.password, "%s", PASSWORD);
	//os_printf("%s %s\n\r", stationConf.ssid, stationConf.password);
	wifi_station_set_config_current(&stationConf);
	wifi_station_disconnect();
	wifi_station_connect();

	wifi_module_set_wifi_status(wifi_connect_ap_ok);
	user_link_led_timer_rst();
	//set a timer to check whether got ip from router succeed or not.
	os_timer_disarm(&test_timer);
	os_timer_setfn(&test_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
	os_timer_arm(&test_timer, 100, 0);
}