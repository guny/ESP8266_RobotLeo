/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "c_types.h"

#define SMARTCONFIG_ENABLE

#define USER_LOCATION_PORT   (14000)
#define USER_SERVER_PORT     (14001)

typedef struct {
	uint32 stamp;
	uint32 times;
	uint8  wakeupflag;
} WIFI_NAVIGATION_HANDCLASP_T;

typedef enum {
	wifi_searching = 0,
	wifi_password_searched,
	wifi_connect_ap_ok,
	wifi_connect_server_ok,
	wifi_conenct_err,
	wifi_unbound,
	wifi_bound_err,
	wifi_module_max
} WIFI_MODULE_STATUS;

typedef enum {
	none_sweep = 0x00,
	auto_sweep = 0x01,
	corner_sweep = 0x02,
	quiet_sweep = 0x03,
	quick_sweep = 0x04,/* mightiness_sweep */
	point_sweep = 0x05,
	area_sweep = 0x06,
	charge = 0x07,
	bow_sweep
} WORKING_MODE;

typedef struct {
	uint8 major;
	uint8 minor;
	uint8 reserved;
} VERSION_NUMBER_T; /* 0x04A6 */

typedef struct {
	uint8 standby;
	uint8 reserve;
	uint8 charge;
	uint8 acleans;
	uint8 pcleans;
	uint8 kcleans;
	uint8 bodyoperate;
	uint8 dustbox;
	uint8 bodystatus;
	uint8 cleanmodule;
	uint8 reset;
	uint8 electric;
	uint16 errorinfo;
} BASEINFO_T; /* 0x03F8 */

typedef struct {
	uint8 status;
	uint8 minute;
	uint8 hour;
	uint16 area;
}CLEAN_INFO_T; /* 0x04B1 */

typedef struct {
	uint32 stamp;
	uint8  plan_enable;
	uint8  plan_week[7];
	uint8  plan_cyclic[7];
	uint8  plan_minute[7];
	uint8  plan_hour[7];
	uint32 times;
}PLANCLEAN_T; /* 0x04A4 */

typedef struct {
	uint8 on_off;
	uint8 mode;
	uint8 bodymode;
} WORK_T; /* 0x04A8 */

typedef struct {
	sint16 x;
	sint16 y;
}POINT_T;

typedef struct {
	sint8   rest;
	POINT_T CurrentPoint;
	sint16 CurrentAngle;
	POINT_T ChargePoint;

	POINT_T Left;
	POINT_T Right;
	uint16 	primarydata;
	uint8   primarycheck[64];
	uint16  validdatalength;
	uint8   map[2048];
} POSITION_T; /* 0x04A5 */

typedef struct {
	uint8 virtual_wall_enable;
	POINT_T wall_start[7];
	POINT_T wall_end[7];
	POINT_T PointClean;
	uint8 virtual_area_enable;
} VIRTUALWALL_T; /* 0x0B02 */

typedef struct {
	uint8  DeviceFindFlag;
	uint32 DormancyNo;
	uint8 WifiEnable;
	uint32 TrajectoryNo;
	VERSION_NUMBER_T Navigaiton;
	WIFI_MODULE_STATUS WifiStatus;
	VIRTUALWALL_T VirtualInfo;
	WIFI_NAVIGATION_HANDCLASP_T WiNaHandClasp;
	BASEINFO_T NaviBaseInfo;
	CLEAN_INFO_T CleanInfo;
	PLANCLEAN_T  PlanClean;
	WORK_T       NavWorkControl;
	POSITION_T   PositionInfo;
} WIFI_MODULE_T;
extern WIFI_MODULE_T WifiModule;

#endif

