
#include "c_types.h"
#include "os_type.h"
#include "eagle_soc.h"
#include "osapi.h"
#include "gpio.h"
#include "user_linkled.h"

#define PLUG_LINK_LED_IO_MUX     PERIPHS_IO_MUX_GPIO0_U
#define PLUG_LINK_LED_IO_NUM     0
#define PLUG_LINK_LED_IO_FUNC    FUNC_GPIO0

#define LINK_LED_TWINKLE_FRE     (80)

LOCAL os_timer_t link_led_timer;
LOCAL uint8 link_led_level = 0;

/******************************************************************************
 * FunctionName : user_link_led_init
 * Description  : int led gpio
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR user_link_led_init(void)
{
    PIN_FUNC_SELECT(PLUG_LINK_LED_IO_MUX, PLUG_LINK_LED_IO_FUNC);
}

void ICACHE_FLASH_ATTR user_link_led_output(uint8 level)
{
    GPIO_OUTPUT_SET(GPIO_ID_PIN(PLUG_LINK_LED_IO_NUM), level);
}

LOCAL void ICACHE_FLASH_ATTR user_link_led_timer_cb(void)
{
    link_led_level = (~link_led_level) & 0x01;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(PLUG_LINK_LED_IO_NUM), link_led_level);
}

void ICACHE_FLASH_ATTR user_link_led_timer_done(void)
{
    os_timer_disarm(&link_led_timer);
	link_led_level = 0;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(PLUG_LINK_LED_IO_NUM), link_led_level);
}

void ICACHE_FLASH_ATTR user_link_led_timer_rst(void)
{
	os_timer_disarm(&link_led_timer);
	link_led_level = 1;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(PLUG_LINK_LED_IO_NUM), link_led_level);
}

void ICACHE_FLASH_ATTR user_link_led_timer_init(void)
{
//	user_link_led_init();
	
//    user_link_led_timer_done();

    os_timer_disarm(&link_led_timer);
    os_timer_setfn(&link_led_timer, (os_timer_func_t *)user_link_led_timer_cb, NULL);
    os_timer_arm(&link_led_timer, LINK_LED_TWINKLE_FRE, 1);
}


