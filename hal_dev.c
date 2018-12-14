//Standard head file

//RTK8710 head file

//IOT SDK head
#include "log.h"
//Adapter head file
#include "led.h"
#include "product_config.h"
#include "hal_dev.h"
#include "hal_event.h"

void PowerOn_LampEffect_Task(void *arg)
{
    led_power_trigger(true);
    vTaskDelay(500/portTICK_RATE_MS);

    led_power_trigger(false);
    vTaskDelay(500/portTICK_RATE_MS);

    led_power_trigger(true);
    vTaskDelay(500/portTICK_RATE_MS);

    led_power_trigger(false);
    vTaskDelay(500/portTICK_RATE_MS);

    led_power_trigger(true);
    vTaskDelay(500/portTICK_RATE_MS);

    led_power_trigger(false);
    vTaskDelay(500/portTICK_RATE_MS);

    vTaskDelete(NULL);
}

void HalDev_Init(void)
{
    struct led_gpio_info    l_led_gpio_info;

    //led init info
    l_led_gpio_info.gpio_pin_num_wifi         = GPIO_LED_WIFI;
    l_led_gpio_info.gpio_pin_directionc_wifi  = PIN_OUTPUT;
    l_led_gpio_info.gpio_pin_mode_wifi        = PullNone;
    l_led_gpio_info.led_wifi_on               = LED_WIFI_ON;
    l_led_gpio_info.led_wifi_off              = LED_WIFI_OFF;
#if ((defined POWER_CONTROL_INDIVIDUAL) && (POWER_CONTROL_INDIVIDUAL == 1))
    l_led_gpio_info.gpio_pin_num_power        = GPIO_LED_POWER;
    l_led_gpio_info.gpio_pin_directionc_power = PIN_OUTPUT;
    l_led_gpio_info.gpio_pin_mode_power       = PullNone;
    l_led_gpio_info.led_power_on              = LED_POWER_ON;
    l_led_gpio_info.led_power_off             = LED_POWER_OFF;
#endif
    led_init(l_led_gpio_info);
    xTaskCreate(PowerOn_LampEffect_Task,"PowerOn_LampEffect_Task" , 256,NULL, 4,NULL);
}

