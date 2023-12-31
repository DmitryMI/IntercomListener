#include "sdkconfig.h"
#include "stdio.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "led_indicator_task.hpp"
#include <memory>

static const char* MAIN_LOG_TAG = "Main";

led_indicator_task led_indicator = led_indicator_task();

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

void on_wake()
{
    esp_sleep_source_t wakeup_reason = esp_sleep_get_wakeup_cause();

    if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
        ESP_LOGI(MAIN_LOG_TAG, "Wake up by EXT0");
    }
    else if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
        ESP_LOGI(MAIN_LOG_TAG, "Wake up by TIMER");
    }
    else
    {
        ESP_LOGI(MAIN_LOG_TAG, "Power-up or unexpected wake up source");
    }

    led_indicator.set_code(led_indicator_code::wakeup);
}

extern "C" void app_main() 
{
    on_wake();

    ESP_LOGI(MAIN_LOG_TAG, "Using WiFi settings: SSID: %s, Password: %s", CONFIG_INTERCOM_WIFI_SSID, CONFIG_INTERCOM_WIFI_PASSWORD);

#ifdef CONFIG_INTERCOM_DEEP_SLEEP_ENABLED
    enter_deep_sleep();
#else
    while(1)
    {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        ESP_LOGI(MAIN_LOG_TAG, "Heartbeat");
        // ESP_LOGI(MAIN_LOG_TAG, "Heartbeat");
    }
#endif
}