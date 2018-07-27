
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"

#include "user_xmpp_analyzer.h"
#include "user_linkled.h"
#include "user_system_config.h"
#include "user_wifi_connect.h"

#include "user_device_find.h"

const char *device_find_request = "robot searching";
/*---------------------------------------------------------------------------*/
LOCAL esp_udp ptrespconn_udp;
LOCAL struct espconn ptrespconn;

/******************************************************************************
 * FunctionName : user_devicefind_recv
 * Description  : Processing the received data from the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 *                pusrdata -- The received data (or NULL when the connection has been closed!)
 *                length -- The length of received data
 * Returns      : none
*******************************************************************************/
LOCAL ICACHE_FLASH_ATTR void user_devicefind_recv(void *arg, char *pusrdata, unsigned short length)
{
	sint8 res = 0;
	char ssid[32];
	char password[64];
	char xmpp[64];
	char DeviceBuffer[256];
	char hwaddr[6];
	remot_info *premot = NULL;
	struct ip_info ipconfig;

	if (wifi_get_opmode() != STATION_MODE)
	{
		wifi_get_ip_info(SOFTAP_IF, &ipconfig);
		wifi_get_macaddr(SOFTAP_IF, hwaddr);

		if (!ip_addr_netcmp((struct ip_addr *)ptrespconn.proto.udp->remote_ip, &ipconfig.ip, &ipconfig.netmask))
		{
			wifi_get_ip_info(STATION_IF, &ipconfig);
			wifi_get_macaddr(STATION_IF, hwaddr);
		}
	} 
	else
	{
		wifi_get_ip_info(STATION_IF, &ipconfig);
		wifi_get_macaddr(STATION_IF, hwaddr);
	}

	if (pusrdata == NULL)
		return;
#if !defined(SMARTCONFIG_ENABLE)
	user_link_led_timer_init();
#endif

//	os_printf("%s, %s\n\r", device_find_request, pusrdata);
#if 1
	if (length == os_strlen(device_find_request) && \
	    os_strncmp(pusrdata, device_find_request, os_strlen(device_find_request)) == 0)
	{
		os_sprintf(DeviceBuffer, "robot 14000 robotleo-sweeper_%02X%02X%02X%01X", \
			hwaddr[3], hwaddr[4], hwaddr[5], hwaddr[0] / 0x10);
		//os_printf("%s\n\r", DeviceBuffer);
#else
	if (length > (os_strlen(device_find_request) + 1) && \
	    os_strncmp(pusrdata, device_find_request, os_strlen(device_find_request)) == 0)
	{
		res = UserInfo_Search(pusrdata + os_strlen(device_find_request) + 1, ssid, password, xmpp);
		os_memset(DeviceBuffer, 0, sizeof(DeviceBuffer));
		if(0 == res)
		{
			os_sprintf(DeviceBuffer, "robot 14000 %02X%02X%02X%01X registered", \
				hwaddr[3], hwaddr[4], hwaddr[5], hwaddr[0] / 0x10);
			//user_connect_router(ssid, password, xmpp);
		}
		else if(-1 == res)
			os_sprintf(DeviceBuffer, "robot 14000 %02X%02X%02X%01X not-registered", \
				hwaddr[3], hwaddr[4], hwaddr[5], hwaddr[0] / 0x10);
		else
			os_sprintf(DeviceBuffer, "robot 14000 %02X%02X%02X%01X un-registered", \
				hwaddr[3], hwaddr[4], hwaddr[5], hwaddr[0] / 0x10);
#endif
		length = os_strlen(DeviceBuffer);

//		if(WifiModule.DeviceFindFlag)
//			return ;

		if (espconn_get_connection_info(&ptrespconn, &premot, 0) != ESPCONN_OK)
			return;

		//os_printf(IPSTR "\n\r", IP2STR(premot->remote_ip));
		os_memcpy(ptrespconn.proto.udp->remote_ip, premot->remote_ip, 4);
		ptrespconn.proto.udp->remote_port = 14001;//premot->remote_port;
		espconn_sent(&ptrespconn, DeviceBuffer, length);
	}
}

LOCAL ICACHE_FLASH_ATTR void user_devicefind_sent(void *arg)
{
#if 1
	//os_printf("Searching-Times %u\n\r", times);
	//user_set_station_config("RobotLeo_SZ_A", "RBT20171020");
#else
	user_set_station_config("ROBOTLEO_003", "12345678");
#endif
}

LOCAL ICACHE_FLASH_ATTR void user_devicefind_discb(void *arg)
{
	user_link_led_timer_done();
}

#if !defined(SMARTCONFIG_ENABLE)
LOCAL ICACHE_FLASH_ATTR void user_set_wifi_ssid(void)
{
	struct softap_config config;
	char hwaddr[6];

	wifi_softap_get_config(&config);
	os_memset(config.ssid, 0, 32); 
	wifi_get_macaddr(STATION_IF, hwaddr);

	os_sprintf(config.ssid, "robotleo-sweeper_%02X%02X%02X%01X", \
		       hwaddr[3], hwaddr[4], hwaddr[5], hwaddr[0] / 0x10);
	config.ssid_len = 0;
	config.authmode = AUTH_OPEN;
	config.max_connection = 1;
	wifi_softap_set_config_current(&config);
}

LOCAL ICACHE_FLASH_ATTR void user_set_dhcps_lease(void)
{
	struct ip_info info;
	struct dhcps_lease dhcps_ip;

	IP4_ADDR(&dhcps_ip.start_ip, 192, 168, 1, 1);
	IP4_ADDR(&dhcps_ip.end_ip,   192, 168, 1, 2);

	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);

	wifi_softap_dhcps_stop();
	wifi_set_ip_info(SOFTAP_IF, &info);
	wifi_softap_set_dhcps_lease(&dhcps_ip);
	wifi_softap_dhcps_start();
}
#endif

/******************************************************************************
 * FunctionName : user_devicefind_init
 * Description  : the espconn struct parame init
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR user_devicefind_init(uint32 port)
{
#if !defined(SMARTCONFIG_ENABLE)
	user_set_wifi_ssid();
	user_set_dhcps_lease();
#endif

	WifiModule.WifiStatus = wifi_searching;
	wifi_module_set_wifi_status(wifi_searching);

    ptrespconn.type = ESPCONN_UDP;
    ptrespconn.proto.udp = (esp_udp *)&ptrespconn_udp;
    ptrespconn.proto.udp->local_port = port;
    espconn_regist_recvcb(&ptrespconn, user_devicefind_recv);
	espconn_regist_sentcb(&ptrespconn, user_devicefind_sent);
	espconn_regist_disconcb(&ptrespconn, user_devicefind_discb);
    espconn_create(&ptrespconn);
}

