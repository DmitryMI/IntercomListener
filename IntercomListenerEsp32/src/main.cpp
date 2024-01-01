#include "sdkconfig.h"
#include "stdio.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "led_indicator_task.hpp"
// #include "wifi.hpp"
#include <memory>
#include "nvs_flash.h"
#include "timer.hpp"

extern "C" bool wifi_init_sta(void);

static const char* MAIN_LOG_TAG = "Main";

led_indicator_task led_indicator;

#ifdef CONFIG_INTERCOM_DEEP_SLEEP_ENABLED
void enter_deep_sleep()
{
    ESP_LOGI(MAIN_LOG_TAG, "Preparing for deep-sleep...");

    esp_wifi_stop();
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(CONFIG_INTERCOM_RING_GPIO_PIN), CONFIG_INTERCOM_WAKE_LEVEL);

#ifdef CONFIG_INTERCOM_DEEP_SLEEP_TIMER_ENABLED
    ESP_LOGI(MAIN_LOG_TAG, "Waking up in %d seconds...", CONFIG_INTERCOM_DEEP_SLEEP_TIMER_PERIOD);
    esp_sleep_enable_timer_wakeup(1000000 * CONFIG_INTERCOM_DEEP_SLEEP_TIMER_PERIOD);
#endif
    ESP_LOGI(MAIN_LOG_TAG, "Sleeping...");
    esp_deep_sleep_start();
}
#endif

extern "C" void app_main() 
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_LOGI(MAIN_LOG_TAG, "nvs_flash_init called");
    esp_netif_init();
    ESP_LOGI(MAIN_LOG_TAG, "esp_netif_init called");

    esp_sleep_source_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
        ESP_LOGI(MAIN_LOG_TAG, "Wake up by EXT0");
        // TODO Send notification
    }
    else if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
        ESP_LOGI(MAIN_LOG_TAG, "Wake up by TIMER");
    }
    else
    {
        ESP_LOGI(MAIN_LOG_TAG, "Power-up or unexpected wake up source");
    }

    bool wifi_ok = wifi_init_sta();
    if(!wifi_ok)
    {
        led_indicator.set_code(led_indicator_code::wifi_error);
    }
    else
    {

    }

    timer_setup(30);

    while(1)
    {
        timer_event timer_event;
        bool alarm_received = xQueueReceive(timer_queue, &timer_event, 10000 / portTICK_PERIOD_MS);
        if(alarm_received)
        {
            ESP_LOGI(MAIN_LOG_TAG, "Timer event received. Group: %d, index: %d, counter value: %llu, time: %llu", 
                (int)timer_event.info.timer_group, (int)timer_event.info.timer_idx, timer_event.timer_counter_value, timer_event.timer_time_seconds);

#ifdef CONFIG_INTERCOM_DEEP_SLEEP_ENABLED
            enter_deep_sleep();
#endif
        }
        else
        {
            ESP_LOGI(MAIN_LOG_TAG, "Heartbeat.");
        }
    }
}