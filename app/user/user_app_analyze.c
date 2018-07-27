
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_webserver.h"
#include "user_app_analyze.h"

typedef void (*operation_func)(struct WIFI_PORT_DATA_t );
typedef struct {
	char* name;
	operation_func operation;
} APP_ANALYZE_T;

LOCAL ICACHE_FLASH_ATTR void hello_callback(struct WIFI_PORT_DATA_t AppCommand)
{
//	os_printf("hello esp8266 wi-fi module\n\r");
#if 1
	controller_clrean();
	tcp_send_data("ok", 2);
#else
	WifiModule.WifiStatus = wifi_connect_ap_ok;
	wifi_module_set_wifi_status(wifi_connect_server_ok);
	tcp_send_data("songxia", 7);
	navigation_pos(WifiModule.PositionInfo.CurrentPoint.x, \
		           WifiModule.PositionInfo.CurrentPoint.y, \
		           WifiModule.PositionInfo.CurrentAngle / 10);
	navigation_electric(WifiModule.NaviBaseInfo.electric);
	navigation_ctime(WifiModule.CleanInfo.hour, WifiModule.CleanInfo.minute);
	navigation_carea(WifiModule.CleanInfo.area);
#endif
}

LOCAL ICACHE_FLASH_ATTR void navigation_go(struct WIFI_PORT_DATA_t AppCommand)
{
}

LOCAL ICACHE_FLASH_ATTR void navigation_back(struct WIFI_PORT_DATA_t AppCommand)
{
}

LOCAL ICACHE_FLASH_ATTR void navigation_left(struct WIFI_PORT_DATA_t AppCommand)
{
}

LOCAL ICACHE_FLASH_ATTR void navigation_right(struct WIFI_PORT_DATA_t AppCommand)
{
}

LOCAL ICACHE_FLASH_ATTR void navigation_stop(struct WIFI_PORT_DATA_t AppCommand)
{
	WifiModule.NavWorkControl.on_off = 0x01;
	wifi_module_notice_navigation_work();
}

LOCAL ICACHE_FLASH_ATTR void notice_navigation_start_working(void)
{
	WifiModule.NavWorkControl.on_off = 0x00;
	wifi_module_notice_navigation_work();
}

LOCAL ICACHE_FLASH_ATTR void navigation_auto(struct WIFI_PORT_DATA_t AppCommand)
{
	WifiModule.TrajectoryNo = 0;

	navigation_ctime(0, 0);
	navigation_carea(0);

	if(WifiModule.NavWorkControl.mode == none_sweep || \
		WifiModule.NavWorkControl.mode == point_sweep)
		WifiModuleInit();

	notice_navigation_start_working();
}

LOCAL ICACHE_FLASH_ATTR void navigation_discover(struct WIFI_PORT_DATA_t AppCommand)
{
}

LOCAL ICACHE_FLASH_ATTR uint32 get_number_string_for_string(uint8* dst, uint8* src, uint32 length)
{
	uint32_t i = 0;

	for(i = 0; i < length; i++)
	{
		if(' ' == src[i])
		{
			i += 1;
			break;
		}
		else if('-' == src[i] || '.' == src[i] || \
			   ('0' <= src[i] && '9' >= src[i]))
			*dst++ = src[i];
		else
			continue;
	}

	return i;
}

LOCAL os_timer_t navigation_point_clear_timer;
LOCAL ICACHE_FLASH_ATTR navigation_point_clear_strat(void)
{
	os_timer_disarm(&navigation_point_clear_timer);

	wifi_module_set_navigation_virtual_wall();

	if(WifiModule.VirtualInfo.virtual_wall_enable & 0x01)
		WifiModule.VirtualInfo.virtual_wall_enable &= ~(0x01);

	//os_delay_us(1000);

	WifiModule.NavWorkControl.bodymode = auto_sweep;
	WifiModule.NavWorkControl.mode = point_sweep;
	notice_navigation_start_working();
}

ICACHE_FLASH_ATTR void navigation_point_clear_init(void)
{
	os_timer_disarm(&navigation_point_clear_timer);
    os_timer_setfn(&navigation_point_clear_timer, (os_timer_func_t *)navigation_point_clear_strat, NULL);
    os_timer_arm(&navigation_point_clear_timer, 200, 0);
}

LOCAL ICACHE_FLASH_ATTR void navigation_abpath(struct WIFI_PORT_DATA_t AppCommand)
{
	uint32 i = 0, j = 0, k = 0;
	uint8  str[16];
	sint16 x = 0, y = 0;

	WifiModule.VirtualInfo.virtual_wall_enable |= 0x01;
	for(i = 7; i < AppCommand.length; )
	{
		os_memset(str, 0, sizeof(str));
		
		j = get_number_string_for_string(str, AppCommand.framer + i, AppCommand.length - i);
		i += ((0 == j) ? 1 : j);
		if(0 == k)
			x = atoi(str);
		else
			y = atoi(str);
		k++;
	}

	WifiModule.VirtualInfo.PointClean.x = x;
	WifiModule.VirtualInfo.PointClean.y = y;
	//WifiModule.VirtualInfo.virtual_area_enable = 0x00;
	wifi_module_set_navigation_virtual_wall();

	//os_delay_us(1000);
	//os_delay_us(65535);

	navigation_point_clear_init();
}

LOCAL ICACHE_FLASH_ATTR void navigation_reset(struct WIFI_PORT_DATA_t AppCommand)
{
	WifiModule.TrajectoryNo = 0;
	WifiModule.VirtualInfo.virtual_area_enable |= 0x80;
	wifi_module_set_navigation_virtual_wall();
}

LOCAL ICACHE_FLASH_ATTR void navigation_charge(struct WIFI_PORT_DATA_t AppCommand)
{
	WifiModule.TrajectoryNo = 0x00;
	WifiModule.NavWorkControl.mode   = charge;
	notice_navigation_start_working();
}

LOCAL ICACHE_FLASH_ATTR uint32 string_length(uint8* src)
{
	uint32 length;

	length = 0;
	while(src[length] != '\0')
		length++;

	return length;
}

LOCAL uint8 map_send_to_app_enable = 0;
LOCAL ICACHE_FLASH_ATTR void navigation_map(struct WIFI_PORT_DATA_t AppCommand)
{
	if(0 == os_memcmp(AppCommand.framer, "map 1", 5))
		map_send_to_app_enable = 1;
}

LOCAL ICACHE_FLASH_ATTR void navigation_wall(struct WIFI_PORT_DATA_t AppCommand)
{

}

LOCAL ICACHE_FLASH_ATTR void navigation_line(struct WIFI_PORT_DATA_t AppCommand)
{
	uint32 i, j, k;
	uint8 str[32];

	//os_printf("%u, %s\n\r", AppCommand.length, AppCommand.framer);
	if(0 != os_strstr(AppCommand.framer, "delete"))
	{
		WifiModule.VirtualInfo.wall_start[0].x = WifiModule.VirtualInfo.wall_start[0].y = 0;
		WifiModule.VirtualInfo.wall_end[0].x = WifiModule.VirtualInfo.wall_end[0].y = 0;
		WifiModule.VirtualInfo.virtual_wall_enable &= ~(0x02);
	}
	else
	{
		j = k = 0;
		for(i = 5; i < AppCommand.length; )
		{
			os_memset(str, 0, sizeof(str));
			j = get_number_string_for_string(str, AppCommand.framer + i, AppCommand.length - i);
			i += ((0 == j) ? 1 : j);

			//os_printf("%u, %s\n\r", i, str);
			//if('0' > str[0] || '9' < str[0] || '-' != str[0])
			//	continue;

			//os_printf("%u, %s, %d\n\r", k, str, atoi(str));
			if(0 == k)
				WifiModule.VirtualInfo.wall_start[0].x = atoi(str);
			else if(1 == k)
				WifiModule.VirtualInfo.wall_start[0].y = atoi(str);
			else if(2 == k)
				WifiModule.VirtualInfo.wall_end[0].x   = atoi(str);
			else if(3 == k)
				WifiModule.VirtualInfo.wall_end[0].y   = atoi(str);
			k++;
		}

		WifiModule.VirtualInfo.virtual_wall_enable |= 0x02;
	}
	//os_printf("%02X, (%d,%d) ~ (%d,%d)\n\r", WifiModule.VirtualInfo.virtual_wall_enable,
	//	WifiModule.VirtualInfo.wall_start[0].x, WifiModule.VirtualInfo.wall_start[0].y,
	//	WifiModule.VirtualInfo.wall_end[0].x, WifiModule.VirtualInfo.wall_end[0].y);

	wifi_module_set_navigation_virtual_wall();

	navigation_point_clear_init();
}

LOCAL ICACHE_FLASH_ATTR void navigation_itime(struct WIFI_PORT_DATA_t AppCommand)
{
	uint32 i, j;
	uint32 flag;
	uint8 hour,minute;
	uint8 str[32];
	uint8 week[8][4] = {"Non\0", "Mon\0", "Tue\0", "Wed\0", "Thu\0", "Fir\0", "Sat\0", "Sun\0"};

	WifiModule.PlanClean.plan_enable = 0x02;
	WifiModule.PlanClean.plan_cyclic[0] = 0x00;
	for(i = 0; i < 8; i++)
	{
		if(0 != os_strstr(AppCommand.framer, &week[i][0]))
			WifiModule.PlanClean.plan_week[0] |= (0x01 << i);
		else
			WifiModule.PlanClean.plan_week[0] &= ~(0x01 << i);
	}

	flag = 0;
	for(i = 6; i < AppCommand.length; )
	{
		os_memset(str, 0, sizeof(str));
		j = get_number_string_for_string(str, AppCommand.framer + i, AppCommand.length - i);
		i += ((0 == j) ? 1 : j);

		if('0' > str[0] || '9' < str[0])
			continue;

		if(0 == flag)
			WifiModule.PlanClean.stamp = atoi(str);
		else if(1 == flag)
			WifiModule.PlanClean.plan_hour[0] = atoi(str);
		else if(2 == flag)
			WifiModule.PlanClean.plan_minute[0] = atoi(str);
		flag++;
	}

	wifi_module_send_timestamp_timer_init();
}

LOCAL ICACHE_FLASH_ATTR void navigation_plain(struct WIFI_PORT_DATA_t AppCommand)
{
	WifiModule.NavWorkControl.mode = auto_sweep;
	//notice_navigation_start_working();
}

LOCAL ICACHE_FLASH_ATTR void navigation_quiet(struct WIFI_PORT_DATA_t AppCommand)
{
	WifiModule.NavWorkControl.mode = quiet_sweep;
	//notice_navigation_start_working();
}

LOCAL ICACHE_FLASH_ATTR void navigation_corner(struct WIFI_PORT_DATA_t AppCommand)
{
	WifiModule.NavWorkControl.mode = corner_sweep;
	//notice_navigation_start_working();
}
	
LOCAL ICACHE_FLASH_ATTR void navigation_mightiness(struct WIFI_PORT_DATA_t AppCommand)
{	//WifiModule.NavWorkControl.mode = mightiness_sweep;
	WifiModule.NavWorkControl.mode = quick_sweep;
	//notice_navigation_start_working();
}

LOCAL ICACHE_FLASH_ATTR void navigation_zpath(struct WIFI_PORT_DATA_t AppCommand)
{
	WifiModule.NavWorkControl.mode = bow_sweep;
	//notice_navigation_start_working();
}

LOCAL ICACHE_FLASH_ATTR void navigation_systemtime(struct WIFI_PORT_DATA_t AppCommand)
{
	uint32 stamp;
	uint32 i;

	for(stamp = 0, i = 11; i < AppCommand.length; i++)
		stamp = (stamp * 10) + (AppCommand.framer[i] - 0x30);

	WifiModule.PlanClean.stamp = stamp;

//	wifi_module_send_class_time();
	wifi_navigation_handclasp_handler_enable();

	navigation_dormancy_timer_init();

	wifi_module_send_timestamp_timer_init();
}

LOCAL ICACHE_FLASH_ATTR void navigation_getbirdversion(struct WIFI_PORT_DATA_t AppCommand)
{
}

LOCAL ICACHE_FLASH_ATTR void navigation_degree(struct WIFI_PORT_DATA_t AppCommand)
{
}

LOCAL ICACHE_FLASH_ATTR void navigation_traj(struct WIFI_PORT_DATA_t AppCommand)
{
}

APP_ANALYZE_T AppCommandTable[] = {
	"hello",          hello_callback,
	"go",             navigation_go,
	"back",           navigation_back,
	"left",           navigation_left,
	"right",          navigation_right,
	"auto",           navigation_auto,
	"stop",           navigation_stop,
	"discover",       navigation_discover,
	"zpath",          navigation_zpath,
	"abpath",         navigation_abpath,
	"reset",          navigation_reset,
	"charge",         navigation_charge,
	"map",            navigation_map,
	"wall",           navigation_wall,
	"line",           navigation_line,
	"itime",          navigation_itime,
	"plain",          navigation_plain,
	"quiet",          navigation_quiet,
	"corner",         navigation_corner,
	"mightiness",     navigation_mightiness,
//	"status",         robot_status,
//	"updata",         robot_updata,
	"systemtime",     navigation_systemtime,
	"getBirdVersion", navigation_getbirdversion,
	"degree",         navigation_degree,
	"traj",           navigation_traj
};

ICACHE_FLASH_ATTR sint8 user_app_command_analyze(struct WIFI_PORT_DATA_t AppCommand)
{
	uint32 i;

	//if(0 != os_memcmp(AppCommand.framer, "pos", 3) && \
	//   0 != os_memcmp(AppCommand.framer, "map", 3) && \
	//   0 != os_memcmp(AppCommand.framer, "hello", 4))
	//	os_printf("%d, %s\n\r", AppCommand.length, AppCommand.framer);
	for(i = 0; i < sizeof(AppCommandTable) / sizeof(*AppCommandTable); i++)
	{
		if(0 == os_memcmp(AppCommandTable[i].name, \
			              AppCommand.framer, \
			              os_strlen(AppCommandTable[i].name)))
		{
			AppCommandTable[i].operation(AppCommand);
			return 0;
		}
	}

	return -1;
}

/******************************************************************************
 ******************************************************************************
 ******************************************************************************/
LOCAL uint8 report_buff[3096];
ICACHE_FLASH_ATTR void navigation_electric(uint32 electric)
{
	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "electric %u", (100 < electric) ? 100 : electric);
	tcp_send_data(report_buff, os_strlen(report_buff));
}

ICACHE_FLASH_ATTR void navigation_ctime(uint32 hour, uint32 minute)
{
	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "ctime %u", hour * 60 + minute);
	tcp_send_data(report_buff, os_strlen(report_buff));
}

ICACHE_FLASH_ATTR void navigation_carea(uint32 area)
{
	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "carea %u", area);
	tcp_send_data(report_buff, os_strlen(report_buff));
}

ICACHE_FLASH_ATTR void navigation_pos(sint16 x, sint16 y, sint16 angle)
{
	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "pos %d %d %d", x, y, angle);
	tcp_send_data(report_buff, os_strlen(report_buff));
}

ICACHE_FLASH_ATTR void navigation_trajectory_to_app(uint8* data, uint32 length)
{
	uint32 i;
	sint16 x, y;
	uint8  trajectory_mode;
	uint8  trajectory_nums;
	uint32 tcp_length;

	trajectory_mode = data[0];
	if(0x00 == trajectory_mode)
		trajectory_nums = (10 < data[1]) ? 10 : data[1];
	else if(0x01 == trajectory_mode)
		trajectory_nums = (100 < data[1]) ? 100 : data[1];
	else
		return ;

	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "trajectory %d", WifiModule.TrajectoryNo);
	WifiModule.TrajectoryNo++;
	tcp_length = os_strlen(report_buff);
	for(i = 0; i < trajectory_nums; i++)
	{
		x = (data[2 + i * 4] << 8) | data[3 + i * 4];
		y = (data[4 + i * 4] << 8) | data[5 + i * 4];

		os_sprintf(report_buff + tcp_length, " %d %d", x, y);
		tcp_length = os_strlen(report_buff);
	}

	//os_printf("%u %s\n\r", trajectory_nums, report_buff);
	tcp_send_data(report_buff, os_strlen(report_buff));

	//x = (data[length - 4] << 8) | data[length - 3];
	//y = (data[length - 2] << 8) | data[length - 1];
	//os_printf("%u %s; pos %d %d\n\r", trajectory_nums, report_buff, x, y);
	navigation_pos(x, y, 0);
}

ICACHE_FLASH_ATTR void navigation_charge_pos(sint16 x, sint16 y)
{
	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "charge_pos %d %d", x, y);
	tcp_send_data(report_buff, os_strlen(report_buff));
}

LOCAL ICACHE_FLASH_ATTR void navigation_charge_point_upload(void)
{
	navigation_charge_pos(WifiModule.PositionInfo.ChargePoint.x, \
	                      WifiModule.PositionInfo.ChargePoint.y);
	//os_printf("%s\n\r", report_buff);
}

LOCAL os_timer_t navigation_charge_point_timer;
ICACHE_FLASH_ATTR void navigation_charge_point_upload_loop(void)
{
	os_timer_disarm(&navigation_charge_point_timer);
    os_timer_setfn(&navigation_charge_point_timer, (os_timer_func_t *)navigation_charge_point_upload, NULL);
    os_timer_arm(&navigation_charge_point_timer, 1000, 1);
}

ICACHE_FLASH_ATTR void navigation_finished(uint8 flag, uint16 err_info)
{
	if(WifiModule.NavWorkControl.bodymode != WifiModule.NavWorkControl.mode)
		return ;

	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "finished %d", flag);
	tcp_send_data(report_buff, os_strlen(report_buff));
}

ICACHE_FLASH_ATTR void navigation_app_setinfo_feedback(uint8 feedback)
{
	if(0x40 == (feedback & 0x40))
	{
		os_memset(report_buff, 0, sizeof(report_buff));
		os_sprintf(report_buff, "reservation cleaning");
		tcp_send_data(report_buff, os_strlen(report_buff));
	}
}

ICACHE_FLASH_ATTR void navigation_bodystatus_to_endpoint(uint8 bodystatus)
{
	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "bodystatus ");
	switch(bodystatus)
	{
		case 0 :
			os_sprintf(report_buff + 11, "starting");
			break;

		case 1 :
			os_sprintf(report_buff + 11, "stop");
			break;

		case 2 :
			os_sprintf(report_buff + 11, "charging");
			break;

		case 3 :
			os_sprintf(report_buff + 11, "unusal");
			break;

		default:
			return ;
	}
	
	tcp_send_data(report_buff, os_strlen(report_buff));
}

ICACHE_FLASH_ATTR void navigation_map_zip(uint8* data, uint32 length)
{
	uint32 tcp_length;

	if(0 == map_send_to_app_enable)
		return ;

	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "mapzip ");

	tcp_length = os_strlen(report_buff);
	os_memcpy(report_buff + tcp_length, data, length);

	tcp_send_data(report_buff, tcp_length + length);
}

ICACHE_FLASH_ATTR void navigation_charging(void)
{
	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "charging");
	tcp_send_data(report_buff, os_strlen(report_buff));
}


LOCAL os_timer_t navigation_sleep_timer;
#define NAVIGATION_SLEEP_DELAY_TIME    (5.5 * 60 * 1000)
#define NAVIGATION_SLEEP_DELAY_TIME1   (30 * 1000)
LOCAL ICACHE_FLASH_ATTR void navigation_dormancy_cb(void)
{
	os_timer_disarm(&navigation_sleep_timer);
//	if(2 <= WifiModule.DormancyNo)
//		return ;
//	else
		WifiModule.DormancyNo++;

	os_memset(report_buff, 0, sizeof(report_buff));
	os_sprintf(report_buff, "dormancy");
	//os_printf("%s\n\r", report_buff);
	tcp_send_data(report_buff, os_strlen(report_buff));

	os_timer_setfn(&navigation_sleep_timer, (os_timer_func_t *)navigation_dormancy_cb, NULL);
    os_timer_arm(&navigation_sleep_timer, NAVIGATION_SLEEP_DELAY_TIME1, 0);
}

ICACHE_FLASH_ATTR void navigation_dormancy_timer_init(void)
{
	//os_printf("navigation_dormancy_timer_init(), %u\n\r", WifiModule.DormancyNo);
	WifiModule.DormancyNo = 0x00;
	os_timer_disarm(&navigation_sleep_timer);
    os_timer_setfn(&navigation_sleep_timer, (os_timer_func_t *)navigation_dormancy_cb, NULL);
    os_timer_arm(&navigation_sleep_timer, NAVIGATION_SLEEP_DELAY_TIME, 0);
}

