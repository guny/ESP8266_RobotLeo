
#include "osapi.h"
#include "c_types.h"
#include "mem.h"
#include "user_config.h"
#include "user_navigation_analyze.h"

CONTROLLER_UPLOAD_STRUCT ControllerReg;
CONTROLLER_UPLOAD_STRUCT oControllerReg;

LOCAL POSITION_STRUCT LastPostion, CurrentPosition;

ICACHE_FLASH_ATTR void controller_clrean(void)
{
	os_memset(&ControllerReg, 0x00, sizeof(CONTROLLER_UPLOAD_STRUCT));
	os_memset(&oControllerReg, 0x00, sizeof(CONTROLLER_UPLOAD_STRUCT));

	LastPostion.x = LastPostion.y = LastPostion.th = 0;
	CurrentPosition.x = CurrentPosition.y = CurrentPosition.th = 0;
}

#define min(a, b) ((a > b) ? b : a)
#define max(a, b) ((a > b) ? a : b)

#define BYTE2WORD(src)           ((src[0] << 8) | src[1])
#define ROBOT_POSITION_DIV        (10) //cm
LOCAL ICACHE_FLASH_ATTR void controller_analyze(uint8_t* src, uint32_t length)
{
	uint8 mapinfo[2048];
	uint32 space0 = 0, space1 = 0;

	//os_printf("Cmd >> %02X, length %u\n\r", src[0], length);
	if(NULL == src || 0 == length)
		return ;

	if(0x01 == src[0])
	{
		os_memcpy(&ControllerReg, src, length);
		CurrentPosition.y = BYTE2WORD(ControllerReg.imu_pitch_angle);
		CurrentPosition.y /= ROBOT_POSITION_DIV;
		CurrentPosition.x = BYTE2WORD(ControllerReg.imu_roll_angle);
		CurrentPosition.x /= ROBOT_POSITION_DIV;
		CurrentPosition.th = BYTE2WORD(ControllerReg.imu_path_angle);

		if(LastPostion.x != CurrentPosition.x || LastPostion.y != CurrentPosition.y)
		{
			memset(mapinfo, 0x00, sizeof(mapinfo));
			os_sprintf(mapinfo, "map %d %d %d %d ", \
			       min(CurrentPosition.x, LastPostion.x), min(CurrentPosition.y, LastPostion.y), \
			       max(CurrentPosition.x, LastPostion.x), max(CurrentPosition.y, LastPostion.y));
			space1 = max(CurrentPosition.y, LastPostion.y) - min(CurrentPosition.y, LastPostion.y) + 1;
			space0 = max(CurrentPosition.x, LastPostion.x) - min(CurrentPosition.x, LastPostion.x) + 1;

			os_memset(&mapinfo[strlen(mapinfo)], '1', space1 * space0);
			tcp_send_data(mapinfo, strlen(mapinfo));

			navigation_electric(ControllerReg.electricit_power);
			navigation_pos(CurrentPosition.x, CurrentPosition.y, CurrentPosition.th);
			
			LastPostion.x = CurrentPosition.x;
			LastPostion.y = CurrentPosition.y;
			LastPostion.th = CurrentPosition.th;
		}
	}
}

#define _command_head0 0xA5
#define _command_head1 0xA5
#define _command_head2 0x5A
#define _command_head3 0x5A
#define FRAMER_DATA_NUMBERS_MAX    (3072)

#define UsartReceiveAnalyzer receiver_data
ICACHE_FLASH_ATTR void UsartReceiveAnalyzer(uint8_t data)
{
	LOCAL uint16 check_num = 0, check_xor = 0;
	LOCAL uint8 FramerFlag = 0;
	LOCAL uint32 CommandLen = 0;
	LOCAL uint32 CommandCo = 0;
	LOCAL uint8  FramerData[FRAMER_DATA_NUMBERS_MAX];

	//os_printf("FramerFlag %u, %u >> 0x%02X\n\r", FramerFlag, CommandCo, data);
#if 0
	if(12 == FramerFlag)
	{
		FramerData[FramerFlag + CommandCo - 2] = data;
		check_num += data;
		check_xor ^= data;
		CommandCo++;
		if(CommandCo == (CommandLen + 2))
		{
			check_num += 0xBEAF;
			check_xor  += 0xBEAF;
			//os_printf("0x%04X : 0x%04X[0x%04X], length %u, %u\n\r", \
			//	(FramerData[6] << 8) | FramerData[7], \
			//	(FramerData[4] << 8) | FramerData[5], \
			//	check_num, CommandLen, CommandCo);
			if(((FramerData[4] << 8) | FramerData[5]) == check_num || \
			   ((FramerData[4] << 8) | FramerData[5]) == check_xor)
			{
				//os_printf("length >> %u; Receiver >> OK!!\n\r", CommandLen);
				wifi_module_receiver_process(FramerData, FramerFlag + CommandLen);
			}

			check_xor = check_num = 0;
			CommandLen = CommandCo = 0;
			FramerFlag = 0;
			os_memset(FramerData, 0, sizeof(FramerData));
			return ;
		}
		
	}
	else if(10 <= FramerFlag && 12 > FramerFlag)  /* reserved */
	{
		FramerData[FramerFlag] = data;
		FramerFlag++;
		CommandCo++;
		check_num += data;
		check_xor ^= data;
		if(CommandCo == (CommandLen + 2))
		{
			check_num += 0xBEAF;
			check_xor  += 0xBEAF;
			//os_printf("0x%04X : 0x%04X[0x%04X], length %u, %u\n\r", \
			//	(FramerData[6] << 8) | FramerData[7], \
			//	(FramerData[4] << 8) | FramerData[5], \
			//	check_num, CommandLen, CommandCo);
			if(((FramerData[4] << 8) | FramerData[5]) == check_num || \
			   ((FramerData[4] << 8) | FramerData[5]) == check_xor)
			{
				//os_printf("length >> %u; Receiver >> OK!!\n\r", CommandLen);
				wifi_module_receiver_process(FramerData, FramerFlag);
			}
			check_xor = check_num = 0;
			CommandLen = CommandCo = 0;
			FramerFlag = 0;
			os_memset(FramerData, 0, sizeof(FramerData));
			return ;
		}
	}
	else if(8 <= FramerFlag && 10 > FramerFlag) /* length */
	{
		FramerData[FramerFlag] = data;
		FramerFlag++;
		CommandCo = 0;
		CommandLen = (CommandLen << 8) + data;
		check_num += data;
		check_xor ^= data;
	}
	else if(6 <= FramerFlag && 8 > FramerFlag)  /* command */
	{
		FramerData[FramerFlag] = data;
		FramerFlag++;
		CommandLen = 0;
		check_num += data;
		check_xor ^= data;
	}
	else if(4 <= FramerFlag && 6 > FramerFlag) /* check sum */
	{
		FramerData[FramerFlag] = data;
		FramerFlag++;
	}
	else if(_command_head2 == data && 2 <= FramerFlag && 4 > FramerFlag) /* 0x5A */
	{
		FramerData[FramerFlag] = data;
		FramerFlag++;
		check_num += data;
		check_xor ^= data;
	}
	else if(_command_head0 == data && 2 > FramerFlag) /* 0xA5 */
	{
		if(0 == FramerFlag)
		{
			check_xor = check_num = 0;
			os_memset(FramerData, 0, sizeof(FramerData));
		}
		FramerData[FramerFlag] = data;
		FramerFlag++;
		check_num += data;
		check_xor ^= data;
	}
	else
	{
		check_xor = check_num = 0;
		FramerFlag = 0;
	}
#else
	if(5 == FramerFlag)
	{
		os_printf("%u[%u], %02X\n\r", CommandLen, CommandCo, check_xor);
		if(0x5A == data)
			controller_analyze(FramerData, (1 + CommandLen));

		FramerFlag = 0;
	}
	else if(4 == FramerFlag)
	{
		FramerData[CommandCo++] = data;
		check_xor ^= data;
		if((2 + CommandLen) <= CommandCo)
		{
			if(0x00 == check_xor)
				FramerFlag = 5;
			else
				FramerFlag = 0;
		}
	}
	else if(3 == FramerFlag)
	{
		FramerData[CommandCo++] = data;
		check_xor ^= data;
		CommandLen += data;
		FramerFlag = 4;
	}
	else if(2 == FramerFlag)
	{
		FramerData[CommandCo++] = data;
		check_xor ^= data;
		CommandLen = (data << 8);
		FramerFlag = 3;
	}
	else if(1 == FramerFlag)
	{
		FramerData[CommandCo++] = data;
		CommandLen = 0;

		FramerFlag = 2;
	}
	else if(0xA5 == data)
	{
		CommandCo = check_xor = check_num = 0;
		os_memset(FramerData, 0, sizeof(FramerData));
		FramerFlag = 1;
	}
	else
	{
		FramerFlag = 0;
	}
#endif
}
/***********************************************************************
 ***********************************************************************
 ***********************************************************************/
LOCAL ICACHE_FLASH_ATTR sint8 send_data_to_navigation(uint8* data, uint32 length)
{
	uint8 command_head[6] = {_command_head0, _command_head1, \
		                     _command_head2, _command_head3, \
		                     0, 0};
	uint32 i;
	uint16 check_sum = 0xC0AD;

	for(i = 0; i < length; i++)
		check_sum += data[i];

	command_head[4] = (check_sum >> 8) & 0xFF;
	command_head[5] =  check_sum & 0xFF;
	uart0_tx_buffer(command_head, 6);
	uart0_tx_buffer(data, length);
}

LOCAL ICACHE_FLASH_ATTR void wifi_send_frame_to_navigation_no_payload(uint16 cmd)
{
	uint8 payload[6];

	os_memset(payload, 0, sizeof(payload));
	payload[0] = (cmd >> 8) & 0xFF;
	payload[1] =  cmd & 0xFF;
	send_data_to_navigation(payload, sizeof(payload));
}

#define TCP_SEND_MAX_SIZE  (3096)
LOCAL uint8 tcp_send_buff[TCP_SEND_MAX_SIZE];
/***********************************************************************
 **************************** 0x0B01 ***********************************
 ***********************************************************************/
#define WIFI_NAVIGATION_HANDCLASP_TIME (2100)
LOCAL os_timer_t wifi_navigation_handclasp_timer;
ICACHE_FLASH_ATTR void wifi_navigation_handclasp_send(void)
{
	os_timer_disarm(&wifi_navigation_handclasp_timer);
	if(10 <= WifiModule.WiNaHandClasp.times)
	{
		WifiModule.WiNaHandClasp.wakeupflag = 0;
		return ;
	}
	else
	{
		//os_printf("stamp %u, times %u\n\r", \
		//	       WifiModule.WiNaHandClasp.stamp, \
		//	       WifiModule.WiNaHandClasp.times);

		if(0 != WifiModule.WiNaHandClasp.stamp)
		{
			WifiModule.WiNaHandClasp.times++;
			WifiModule.WiNaHandClasp.wakeupflag = 0;
		}
		else
			WifiModule.WiNaHandClasp.times = 0;

		WifiModule.WiNaHandClasp.stamp++;
		wifi_send_frame_to_navigation_no_payload(0x0B01);

		os_timer_setfn(&wifi_navigation_handclasp_timer, (os_timer_func_t *)wifi_navigation_handclasp_send, NULL);
	    os_timer_arm(&wifi_navigation_handclasp_timer, WIFI_NAVIGATION_HANDCLASP_TIME, 0);
	}
}

ICACHE_FLASH_ATTR void wifi_navigation_handclasp_handler_enable(void)
{
	WifiModule.WiNaHandClasp.times = 0;
	WifiModule.WiNaHandClasp.stamp = 0;
	WifiModule.WiNaHandClasp.wakeupflag = 0;
	wifi_navigation_handclasp_send();

    os_timer_disarm(&wifi_navigation_handclasp_timer);
    os_timer_setfn(&wifi_navigation_handclasp_timer, (os_timer_func_t *)wifi_navigation_handclasp_send, NULL);
    os_timer_arm(&wifi_navigation_handclasp_timer, WIFI_NAVIGATION_HANDCLASP_TIME, 0);
}

ICACHE_FLASH_ATTR void wifi_navigation_handclasp_handler_disable(void)
{
    os_timer_disarm(&wifi_navigation_handclasp_timer);
}
/***********************************************************************
 ************************* 0x0B03 **************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_navigation_handclasp_receive(void)
{
	if(0 != WifiModule.WiNaHandClasp.stamp)
		WifiModule.WiNaHandClasp.stamp--;
	else
		WifiModule.WiNaHandClasp.stamp = 0x00;

	WifiModule.WiNaHandClasp.wakeupflag++;
}
/***********************************************************************
 ************************* 0x0B02 **************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_set_navigation_virtual_wall(void)
{
	uint8 payload[68];
	uint32 i;

	os_memset(payload, 0, sizeof(payload));
	payload[0] = 0x0B;
	payload[1] = 0x02;
	payload[3] = 0x3E;

	payload[6] = WifiModule.VirtualInfo.virtual_wall_enable;
	payload[67] = WifiModule.VirtualInfo.virtual_area_enable;

	for(i = 0; i < 7; i++)
	{
		payload[7  + 8 * i] = (WifiModule.VirtualInfo.wall_start[i].x >> 8) & 0xFF;
		payload[8  + 8 * i] =  WifiModule.VirtualInfo.wall_start[i].x & 0xFF;
		payload[9  + 8 * i] = (WifiModule.VirtualInfo.wall_start[i].y >> 8) & 0xFF;
		payload[10 + 8 * i] =  WifiModule.VirtualInfo.wall_start[i].y & 0xFF;
		payload[11 + 8 * i] = (WifiModule.VirtualInfo.wall_end[i].x   >> 8) & 0xFF;
		payload[12 + 8 * i] =  WifiModule.VirtualInfo.wall_end[i].x   & 0xFF;
		payload[13 + 8 * i] = (WifiModule.VirtualInfo.wall_end[i].y   >> 8) & 0xFF;
		payload[14 + 8 * i] =  WifiModule.VirtualInfo.wall_end[i].y   & 0xFF;
	}
	payload[63] = (WifiModule.VirtualInfo.PointClean.x >> 8) & 0xFF;
	payload[64] =  WifiModule.VirtualInfo.PointClean.x & 0xFF;
	payload[65] = (WifiModule.VirtualInfo.PointClean.y >> 8) & 0xFF;
	payload[66] =  WifiModule.VirtualInfo.PointClean.y & 0xFF;

	//os_printf("0x0B02 >> %d[%X][0x%02X.0x%02X], %d[%X][0x%02X.0x%02X]\n\r", \
	//	 WifiModule.VirtualInfo.PointClean.x, \
	//	 WifiModule.VirtualInfo.PointClean.x, payload[63], payload[64], \
	//	 WifiModule.VirtualInfo.PointClean.y, \
	//	 WifiModule.VirtualInfo.PointClean.y, payload[65], payload[66]);
	send_data_to_navigation(payload, sizeof(payload));
}
/***********************************************************************
 ****************************** 0x**** *********************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_mode_statuc_send_to_navigation(uint16 cmd) 
{
	uint8 payload[7];

	os_memset(payload, 0, sizeof(payload));
	payload[0] = (cmd >> 8) & 0xFF;
	payload[1] =  cmd & 0xFF;
	payload[3] = 0x01;
	payload[6] = WifiModule.WifiStatus;
	send_data_to_navigation(payload, sizeof(payload));
}
/***********************************************************************
 ****************************** 0x03C1 *********************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_wifistatus_request(void)
{
	wifi_mode_statuc_send_to_navigation(0x03C2);
}
/***********************************************************************
 ****************************** 0x03EC *********************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_set_wifi_status(WIFI_MODULE_STATUS wifistatus)
{
	WifiModule.WifiStatus = wifistatus;
	wifi_mode_statuc_send_to_navigation(0x03EC);
}
/***********************************************************************
 ************************* 0x03D7 **************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_navigation_reset_rd(void)
{
	wifi_send_frame_to_navigation_no_payload(0x03D7);
}
/***********************************************************************
 ************************* 0x03D1 0x03D2 *******************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_user_info_cleaning(uint8* data, uint32 lenght)
{
	user_config_clean_all();
	wifi_send_frame_to_navigation_no_payload(0x03D2);
	system_restart();
}
/***********************************************************************
 ************************* 0x03D3 **************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_wifi_open_close(uint8* data, uint32 lenght)
{
	if(3 != lenght)
		return ;

	WifiModule.WifiEnable = data[2];
	wifi_send_frame_to_navigation_no_payload(0x03D4);
}
/***********************************************************************
 ************************* 0x03D6 **************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_navigation_reset_ok(void)
{
	//wifi_send_frame_to_navigation_no_payload(0x03E7);
	wifi_module_receiver_navigation_reset_rd();

	os_delay_us(1000);

	system_restart();
}
/***********************************************************************
 ************************* 0x03E8 **************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_restoration_os_factory(uint8* data, uint32 length)
{
	wifi_send_frame_to_navigation_no_payload(0x03E9);

	os_delay_us(1000);

	user_config_clean_all();
	system_restore();

	system_restart();
}
/***********************************************************************
 ************************* 0x03F8 **************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_navigation_baseinfo(uint8* data, uint32 length)
{
	if(7 != length)
		return ;

	WifiModule.NaviBaseInfo.standby = (data[2] & 0x80) >> 7;
	WifiModule.NaviBaseInfo.reserve = (data[2] & 0x40) >> 6;
	WifiModule.NaviBaseInfo.charge  = (data[2] & 0x30) >> 4;
	WifiModule.NaviBaseInfo.acleans = (data[2] & 0x08) >> 3;
	WifiModule.NaviBaseInfo.pcleans = (data[2] & 0x04) >> 2;
	WifiModule.NaviBaseInfo.kcleans = (data[2] & 0x02) >> 1;
	WifiModule.NaviBaseInfo.bodyoperate =  data[2] & 0x01;
	navigation_app_setinfo_feedback(data[2]);

	WifiModule.NaviBaseInfo.dustbox     = (data[3] & 0xC0) >> 6;
	WifiModule.NaviBaseInfo.bodystatus  = (data[3] & 0x30) >> 4;
	WifiModule.NaviBaseInfo.cleanmodule =  data[3] & 0x0F;
	WifiModule.NavWorkControl.bodymode = WifiModule.NaviBaseInfo.cleanmodule;

	navigation_bodystatus_to_endpoint(WifiModule.NaviBaseInfo.bodystatus);
	if(0x01 == WifiModule.NaviBaseInfo.bodystatus)
		navigation_finished(1, 0);
	else if(0x02 == WifiModule.NaviBaseInfo.bodystatus)
		navigation_charging();
	else if(0x03 == WifiModule.NaviBaseInfo.bodystatus)
		navigation_finished(0, 0);

	WifiModule.NaviBaseInfo.reset = (data[4] & 0x80)>> 7;
	WifiModule.NaviBaseInfo.electric = data[4] & 0x7F;

	WifiModule.NaviBaseInfo.errorinfo = (data[5] << 8) | data[6];

	navigation_electric(WifiModule.NaviBaseInfo.electric);

	wifi_send_frame_to_navigation_no_payload(0x03F9);


	if(10 <= WifiModule.WiNaHandClasp.times)
		wifi_navigation_handclasp_handler_enable();
}
/***********************************************************************
 ************************* 0x04A3 **************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_send_time_to_navigation(uint8* data, uint32 length)
{
	wifi_module_send_timestamp_timer_init();
}
/***********************************************************************
 ************************** 0x04A4 *************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_send_class_time(void)
{
	uint32 i;
	uint8 payload[32];

	os_memset(payload, 0, sizeof(payload));
	payload[0] = 0x04;
	payload[1] = 0xA4;
	payload[3] = 0x1A;

	payload[6] = (WifiModule.PlanClean.stamp >> 24) & 0xFF;
	payload[7] = (WifiModule.PlanClean.stamp >> 16) & 0xFF;
	payload[8] = (WifiModule.PlanClean.stamp >>  8) & 0xFF;
	payload[9] = (WifiModule.PlanClean.stamp >>  0) & 0xFF;

	payload[10] = WifiModule.PlanClean.plan_enable;
	for(i = 0; i < 7; i++)
	{
		payload[11 + i * 3] =   WifiModule.PlanClean.plan_week[i];
		payload[12 + i * 3] = ((WifiModule.PlanClean.plan_cyclic[i] & 0x07) << 5) | \
			                   (WifiModule.PlanClean.plan_hour[i] & 0x1F);
		payload[13 + i * 3] =   WifiModule.PlanClean.plan_minute[i] & 0x3F;
	}

	send_data_to_navigation(payload, sizeof(payload));
}

LOCAL os_timer_t wifi_module_send_timestamp_timer;
LOCAL ICACHE_FLASH_ATTR void wifi_module_send_timestamp(void);
LOCAL ICACHE_FLASH_ATTR void _wifi_module_timestamp(void)
{
	os_timer_setfn(&wifi_module_send_timestamp_timer, (os_timer_func_t *)wifi_module_send_timestamp, NULL);
	os_timer_arm(&wifi_module_send_timestamp_timer, 120, 0);
}

LOCAL ICACHE_FLASH_ATTR void wifi_module_send_timestamp(void)
{
	os_timer_disarm(&wifi_module_send_timestamp_timer);
	if(1 > WifiModule.WiNaHandClasp.wakeupflag)
	{
		_wifi_module_timestamp();
		return ;
	}
	else
	{
		if(2 < WifiModule.PlanClean.times)
			return ;
		else
		{
			if(1 == WifiModule.WiNaHandClasp.wakeupflag)
			{
				WifiModule.WiNaHandClasp.wakeupflag++;
				wifi_module_set_wifi_status(wifi_connect_server_ok);
				_wifi_module_timestamp();
				return ;
			}

			WifiModule.PlanClean.times++;
			wifi_module_send_class_time();
			_wifi_module_timestamp();
		}
	}
}

ICACHE_FLASH_ATTR void wifi_module_send_timestamp_timer_init(void)
{
	WifiModule.PlanClean.times = 0x00;
	os_timer_disarm(&wifi_module_send_timestamp_timer);
    _wifi_module_timestamp();
}
/***********************************************************************
 ************************** 0x04A5 *************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_position_info(uint8* data, uint32 length)
{
	uint8_t data_buff[3072];

	WifiModule.PositionInfo.rest           = data[2];
	if(0 != WifiModule.PositionInfo.rest)
	{
		if(WifiModule.VirtualInfo.virtual_area_enable & 0x80)
		{
			WifiModule.VirtualInfo.virtual_area_enable &= ~(0x80);
			wifi_module_set_navigation_virtual_wall();
		}
	}

	WifiModule.PositionInfo.ChargePoint.x  = (data[3] << 8) | data[4];
	WifiModule.PositionInfo.ChargePoint.y  = (data[5] << 8) | data[6];
	//navigation_charge_pos(WifiModule.PositionInfo.ChargePoint.x, \
	//	                  WifiModule.PositionInfo.ChargePoint.y);

	WifiModule.PositionInfo.CurrentPoint.x = (data[7] << 8) | data[8];
	WifiModule.PositionInfo.CurrentPoint.y = (data[9] << 8) | data[10];
	WifiModule.PositionInfo.CurrentAngle   = (data[11] << 8) | data[12];
	//navigation_pos(WifiModule.PositionInfo.CurrentPoint.x, \
	//	           WifiModule.PositionInfo.CurrentPoint.y, \
	//	           WifiModule.PositionInfo.CurrentAngle / 10);
	//os_printf("X %d ", WifiModule.PositionInfo.CurrentPoint.x);
	//os_printf("Y %d ", WifiModule.PositionInfo.CurrentPoint.y);
	//os_printf("Angle %d\n\r", WifiModule.PositionInfo.CurrentAngle);

	WifiModule.PositionInfo.Left.x         = (data[13] << 8) | data[14];
	WifiModule.PositionInfo.Left.y         = (data[15] << 8) | data[16];
	WifiModule.PositionInfo.Right.x        = (data[17] << 8) | data[18];
	WifiModule.PositionInfo.Right.y        = (data[19] << 8) | data[20];
	WifiModule.PositionInfo.primarydata    = (data[21] << 8) | data[22];
	os_memcpy(WifiModule.PositionInfo.primarycheck,  data + 23, 64);
	WifiModule.PositionInfo.validdatalength = (data[87] << 8) | data[88];
	os_memcpy(WifiModule.PositionInfo.map, data + 89, WifiModule.PositionInfo.validdatalength);

	//os_printf("data length >> %d\n\r", length);
	//os_memcpy(data + 23, data + 87, length - 64 - 23);
	navigation_map_zip(data + 13, length - 13);

//	os_memset(data_buff, 0, sizeof(data_buff));
//	os_memcpy(data_buff, data + 13, 10);
//	os_memcpy(data_buff + 10, data + 87, length - 87);
//	navigation_map_zip(data_buff, length - 77);
}

/***********************************************************************
 ************************** 0x04A6 *************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_navigation_version(uint8* data, uint32 length)
{
	if(5 != length)
		return ;

	WifiModule.Navigaiton.major = data[2];
	WifiModule.Navigaiton.minor = data[3];
	WifiModule.Navigaiton.reserved = data[4];
}

/***********************************************************************
 ************************** 0x04A8 *************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_notice_navigation_work(void)
{
	uint8 payload[7];

	os_memset(payload, 0, sizeof(payload));
	payload[0] = 0x04;
	payload[1] = 0xA8;
	payload[3] = 0x01;
	payload[6] = (WifiModule.NavWorkControl.on_off & 0x03) << 6 | \
	              WifiModule.NavWorkControl.mode & 0x3F;
	send_data_to_navigation(payload, sizeof(payload));
}
/***********************************************************************
 ************************** 0x04A9 *************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_trajectory(uint8* data, uint32 length)
{
	if(8 > length)
		return ;

	navigation_trajectory_to_app(data + 2, length - 2);
}
/***********************************************************************
 ************************** 0x04B1 *************************************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_cleaning_area_time(uint8* data, uint32 length)
{
	if(6 != length)
		return ;

	WifiModule.CleanInfo.status = (data[2] & 0xC0) >> 6;
	WifiModule.CleanInfo.minute =  data[2] & 0x3F;
	WifiModule.CleanInfo.hour   =  data[3];
	WifiModule.CleanInfo.area   = (data[4] << 8) | data[5];

	if(0x01 == WifiModule.CleanInfo.status)
	{
		WifiModuleInit();
//		navigation_finished(1, 0);
	}

	navigation_ctime(WifiModule.CleanInfo.hour, WifiModule.CleanInfo.minute);
	navigation_carea(WifiModule.CleanInfo.area);

	wifi_send_frame_to_navigation_no_payload(0x04B2);
}

/***********************************************************************
 **************** Receiver Navigation Framer Data Process **************
 ***********************************************************************/
ICACHE_FLASH_ATTR void wifi_module_receiver_process(uint8* data, uint32 length)
{
	//uart1_tx_buffer(data, length);
	//os_printf("0x%04X, %u\n\r", (data[6] << 8) | data[7], length);
	switch((data[6] << 8) | data[7])
	{
		case 0x03C1 :
			wifi_module_receiver_wifistatus_request();
			break;

		case 0x03D1 :
			wifi_module_user_info_cleaning(data + 10, length - 10);
			break;

		case 0x03D3 :
			wifi_module_receiver_wifi_open_close(data + 10, length - 10);
			break;

		case 0x03D6 :
			wifi_module_receiver_navigation_reset_ok();
			break;

		case 0x03E8 :
			wifi_module_restoration_os_factory(data + 10, length - 10);
			break;

		case 0x03F8 :
			wifi_module_receiver_navigation_baseinfo(data + 10, length - 10);
			break;

		case 0x04A3 :
			wifi_module_send_time_to_navigation(data + 10, length - 10);
			break;

		case 0x04A5 :
			wifi_module_receiver_position_info(data + 10, length - 10);
			break;

		case 0x04A6 :
			wifi_module_receiver_navigation_version(data + 10, length - 10);
			break;

		case 0x04A9 :
			wifi_module_receiver_trajectory(data + 10, length - 10);
			navigation_dormancy_timer_init();
			break;

		case 0x04B1 :
			wifi_module_receiver_cleaning_area_time(data + 10, length - 10);
			break;

		case 0x0B03 :
			wifi_navigation_handclasp_receive();
			break;

		case 0x0B04 :
			break;
	}
}

