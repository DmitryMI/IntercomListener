#include "sdkconfig.h"
#include "stdio.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_indicator_task.hpp"
#include <memory>
#include "nvs_flash.h"
#include "timer.hpp"
#include "log_level.h"
#include "driver/rtc_io.h"
#include "freertos/event_groups.h"
#include "events.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "telegram.hpp"

extern "C" bool wifi_init_sta(EventGroupHandle_t event_group_handle);
extern "C" bool wifi_deinit_and_stop(void);

const char* main_log_tag = "Main";

EventGroupHandle_t main_event_group;
led_indicator_task led_indicator;
int64_t ring_sensor_timestamp = -1;
int64_t ring_notification_timestamp = -1;
bool ring_notification_pending = false;

#ifdef CONFIG_INTERCOM_DEEP_SLEEP_ENABLED

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

#if CONFIG_ULP_COPROC_ENABLED
void init_ulp()
{
    ESP_LOGI(main_log_tag, "init_ulp called");
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    ulp_set_wakeup_period(0, 5 * 1000 * 1000);
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
    ESP_LOGI(main_log_tag, "ulp_run called");
}
#endif

void enter_deep_sleep()
{
    ESP_LOGI(main_log_tag, "Preparing for deep-sleep...");
    wifi_deinit_and_stop();

#if CONFIG_ULP_COPROC_ENABLED
    init_ulp();
#endif

    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(CONFIG_INTERCOM_RING_GPIO_PIN), CONFIG_INTERCOM_WAKE_LEVEL);

#ifdef CONFIG_INTERCOM_DEEP_SLEEP_DURATION_ENABLED
    ESP_LOGI(main_log_tag, "Waking up in %d seconds...", CONFIG_INTERCOM_DEEP_SLEEP_DURATION);
    esp_sleep_enable_timer_wakeup(1000000 * CONFIG_INTERCOM_DEEP_SLEEP_DURATION);
#endif
    ESP_LOGI(main_log_tag, "Sleeping...");
    esp_deep_sleep_start();
}
#endif

void ring_isr_handler(void *arg)
{
    uint32_t event_bits;
    int current_level = gpio_get_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_RING_GPIO_PIN));
    if(current_level == CONFIG_INTERCOM_WAKE_LEVEL)
    {
        event_bits = EVENT_RING_SENSOR_START;
    }
    else
    {
        event_bits = EVENT_RING_SENSOR_END;
    }
    
    int higherPriorityTaskWoken = false;
    int result = xEventGroupSetBitsFromISR(main_event_group, event_bits, &higherPriorityTaskWoken);
    if(result != pdFAIL)
    {
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

void setup_ring_sensor()
{
    gpio_num_t ring_in = static_cast<gpio_num_t>(CONFIG_INTERCOM_RING_GPIO_PIN);
    ESP_ERROR_CHECK(rtc_gpio_deinit(ring_in));
    ESP_ERROR_CHECK(gpio_set_direction(ring_in, gpio_mode_t::GPIO_MODE_INPUT));
#if CONFIG_INTERCOM_RING_GPIO_PIN_PULL_DOWN
    ESP_ERROR_CHECK(gpio_set_pull_mode(ring_in, GPIO_PULLDOWN_ONLY));
#elif CONFIG_INTERCOM_RING_GPIO_PIN_PULL_UP
    ESP_ERROR_CHECK(gpio_set_pull_mode(ring_in, GPIO_PULLUP_ONLY));
#endif 

    int current_level = gpio_get_level(ring_in);
    ESP_LOGI(main_log_tag, "Setting ring sensor pin to GPIO %d", CONFIG_INTERCOM_RING_GPIO_PIN);
    ESP_LOGD(main_log_tag, "Current level of GPIO %d: %d", CONFIG_INTERCOM_RING_GPIO_PIN, current_level);

    ESP_ERROR_CHECK(gpio_set_intr_type(ring_in, gpio_int_type_t::GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_intr_enable(ring_in));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ring_in, ring_isr_handler, NULL));
}

void send_ring_notification()
{
    ESP_LOGD(main_log_tag, "send_ring_notification called");

#ifdef CONFIG_INTERCOM_TELEGRAM_ENABLED
    int status_code = telegram_send_notification("Door ring!");
    if(status_code != 200)
    {
        led_indicator.set_code(led_indicator_code::http_error);
    }
#endif

}

extern "C" void app_main() 
{
    esp_log_level_set(main_log_tag, INTERCOM_LOG_LEVEL);
    // rtc_gpio_deinit(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN));
    // rtc_gpio_deinit(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_GREEN_GPIO_PIN));
    // rtc_gpio_deinit(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_BLUE_GPIO_PIN));

    main_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    setup_ring_sensor();
    led_indicator.set_code(led_indicator_code::wakeup);
    
    esp_sleep_source_t wakeup_reason = esp_sleep_get_wakeup_cause();

    int timer_alarm_time = CONFIG_INTERCOM_DEEP_SLEEP_DELAY;
    bool wifi_should_connect = true;
    if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
        ESP_LOGI(main_log_tag, "Wake up by EXT0");
        ring_sensor_timestamp = esp_timer_get_time();
        ring_notification_pending = true;
        ESP_LOGD(main_log_tag, "ring_notification_pending set to true");
    }
    else if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
        ESP_LOGI(main_log_tag, "Wake up by TIMER");
        int ring_level = gpio_get_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_RING_GPIO_PIN));
        if(ring_level != CONFIG_INTERCOM_WAKE_LEVEL)
        {
            timer_alarm_time = CONFIG_INTERCOM_DEEP_SLEEP_DELAY_SHORT;
            wifi_should_connect = false;
        }
        else
        {
            ring_sensor_timestamp = esp_timer_get_time();
            ring_notification_pending = true;
            ESP_LOGD(main_log_tag, "ring_notification_pending set to true");
        }
    }
    else
    {
        ESP_LOGI(main_log_tag, "Power-up or unexpected wake up source");
    }

    if(wifi_should_connect)
    {
        bool wifi_ok = wifi_init_sta(main_event_group);
        if(!wifi_ok)
        {
            led_indicator.set_code(led_indicator_code::wifi_error);
        }
    }

    timer_setup(timer_alarm_time, main_event_group);
    
    bool wifi_connected = false;
    while(1)
    {
        EventBits_t event_bits = xEventGroupWaitBits(main_event_group, EVENT_ALL, pdTRUE, pdFALSE, 10000 / portTICK_PERIOD_MS);

        if((event_bits & EVENT_WIFI_CONNECTED) == EVENT_WIFI_CONNECTED)
        {
            ESP_LOGI(main_log_tag, "wifi connected");
            wifi_connected = true;
        }

        if((event_bits & EVENT_WIFI_DISCONNECTED) == EVENT_WIFI_DISCONNECTED)
        {
            ESP_LOGI(main_log_tag, "wifi disconnected");
            wifi_connected = false;
        }

        if((event_bits & EVENT_WIFI_FAIL) == EVENT_WIFI_FAIL)
        {
            ESP_LOGE(main_log_tag, "wifi failed to connect");
            led_indicator.set_code(led_indicator_code::wifi_error);
            wifi_connected = false;
        }

        if((event_bits & EVENT_RING_SENSOR_START) == EVENT_RING_SENSOR_START)
        {
            ESP_LOGD(main_log_tag, "Ring start detected!");
            if(!wifi_should_connect)
            {
                wifi_should_connect = true;
                bool wifi_ok = wifi_init_sta(main_event_group);
                if(!wifi_ok)
                {
                    led_indicator.set_code(led_indicator_code::wifi_error);
                }
            }

            int64_t timestamp = esp_timer_get_time();
            if(ring_sensor_timestamp == -1 || (timestamp - ring_sensor_timestamp > CONFIG_INTERCOM_RING_DETECTION_COOLDOWN * 1000LL))
            {
                timer_reset(CONFIG_INTERCOM_DEEP_SLEEP_DELAY);
                ring_sensor_timestamp = timestamp;
                ring_notification_pending = true;
                ESP_LOGD(main_log_tag, "ring_notification_pending set to true");
            }
        }

        if((event_bits & EVENT_RING_SENSOR_END) == EVENT_RING_SENSOR_END)
        {
            ESP_LOGD(main_log_tag, "Ring end detected!");
        }

        if(wifi_connected && ring_notification_pending)
        {
            int64_t timestamp = esp_timer_get_time();
            if(ring_notification_timestamp == -1 || (timestamp - ring_notification_timestamp > CONFIG_INTERCOM_RING_NOTIFICATION_COOLDOWN * 1000LL))
            {
                ring_notification_timestamp = timestamp;
                send_ring_notification();
            }
            
            // Clear pending flag, since we don't want deferred notification
            ring_notification_pending = false;
        }

        if((event_bits & EVENT_TIMER_ALARM) == EVENT_TIMER_ALARM)
        {
            ESP_LOGI(main_log_tag, "sleep timer expired");
            int ring_level = gpio_get_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_RING_GPIO_PIN));
            if(ring_level == CONFIG_INTERCOM_WAKE_LEVEL)
            {
                ESP_LOGW(main_log_tag, "Ring sensor still at level %d. Extending timer.", ring_level);
                timer_reset(CONFIG_INTERCOM_DEEP_SLEEP_DELAY);
            }
            else
            {
#if CONFIG_INTERCOM_DEEP_SLEEP_ENABLED
                enter_deep_sleep();
#endif
            }
        }
    }
}