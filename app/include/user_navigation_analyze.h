
#ifndef __USER_NAVIGATION_ANALYZE_H__
#define __USER_NAVIGATION_ANALYZE_H__

#pragma pack (1)
typedef struct {
	sint16 x;
	sint16 y; 
	sint16 th;
}POSITION_STRUCT;

typedef struct {
	uint8 cmd;
	uint8 length[2];
	uint8 time_stamp[4];
	uint8 error[2];
	uint8 mode;
	uint8 left_wheel[4];
	uint8 right_wheel[4];
	uint8 universal_whee[4];
	uint8 collision_state;
	uint8 measure_ray_left[2];
	uint8 measure_ray_left_middle[2];
	uint8 measure_ray_middle[2];
	uint8 measure_ray_right_middle[2];
	uint8 measure_ray_right[2];
	uint8 measure_ultrasonic_left[2];
	uint8 measure_ultrasonic_middle[2];
	uint8 measure_ultrasonic_right[2];
	uint8 detection_cliff;
	uint8 detection_raise;
	uint8 charging_status;
	uint8 electricit_power;
	uint8 skid_status;
	uint8 controller_ask;
	uint8 imu_path_angle[2];
	uint8 imu_roll_angle[2];
	uint8 imu_pitch_angle[2];
	uint8 imu_accelerated_speed_x[2];
	uint8 imu_accelerated_speed_y[2];
	uint8 imu_accelerated_speed_z[2];
	uint8 controller_ack;
	uint8 main_version;
	uint8 minor_version;
	uint8 reserve[6];
} CONTROLLER_UPLOAD_STRUCT;
#pragma pack ()

ICACHE_FLASH_ATTR void wifi_navigation_handclasp_handler_enable(void);
ICACHE_FLASH_ATTR void wifi_navigation_handclasp_handler_disable(void);
ICACHE_FLASH_ATTR void wifi_module_receiver_wifistatus_request(void);

ICACHE_FLASH_ATTR void wifi_module_set_wifi_status(WIFI_MODULE_STATUS wifistatus);
ICACHE_FLASH_ATTR void wifi_module_notice_navigation_work(void);
ICACHE_FLASH_ATTR void wifi_module_send_class_time(void);
ICACHE_FLASH_ATTR void wifi_module_send_timestamp_timer_init(void);

ICACHE_FLASH_ATTR void wifi_module_receiver_process(uint8* data, uint32 length);

#endif //__USER_NAVIGATION_ANALYZE_H__