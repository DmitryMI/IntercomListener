#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

enum class led_indicator_code
{
    idle,
    wakeup,
    wifi_error,
    http_error,
    unknown_error
};

class led_indicator_task
{
private:
    static const char* log_tag;

    TaskHandle_t task_handle;
    led_indicator_code current_code = led_indicator_code::idle;
    led_indicator_code pending_code = led_indicator_code::idle;
public:
    led_indicator_task()
    {
        ESP_LOGD(log_tag, "led_indicator_task ctor called");

#if CONFIG_INTERCOM_LED_GREEN_GPIO_PIN >= 0
        gpio_set_direction(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_GREEN_GPIO_PIN), GPIO_MODE_OUTPUT);
        gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_GREEN_GPIO_PIN), 0);
#endif
#if CONFIG_INTERCOM_LED_RED_GPIO_PIN >= 0
        gpio_set_direction(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), GPIO_MODE_OUTPUT);
        gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 0);
#endif
#if CONFIG_INTERCOM_LED_BLUE_GPIO_PIN >= 0
        gpio_set_direction(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_BLUE_GPIO_PIN), GPIO_MODE_OUTPUT);
        gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_BLUE_GPIO_PIN), 0);
#endif

        xTaskCreate(led_indicator_task_routine, "led_indicator_task", 1024, this, tskIDLE_PRIORITY, &task_handle);
        ESP_LOGD(log_tag, "led_indicator_task: task created");
    }

    led_indicator_task(led_indicator_task const&) = delete;
    led_indicator_task& operator=(led_indicator_task const&) = delete;

    void print_stack_info()
    {
        if(task_handle != nullptr)
        {
            uint32_t stack_words_extra = uxTaskGetStackHighWaterMark2(task_handle);
            ESP_LOGD(log_tag, "led_indicator_task has %lu extra stack words", stack_words_extra);
        }
    }

    ~led_indicator_task()
    {
        if(task_handle != nullptr)
        {
            vTaskDelete(task_handle);
        }
    }

    void set_code(led_indicator_code code)
    {
        pending_code = code;
        ESP_LOGD(log_tag, "set_code: %d", static_cast<int>(code));
    }

private:
    static void led_indicator_task_routine(void *pvParameters)
    {
        led_indicator_task* task = static_cast<led_indicator_task*>(pvParameters);
        if(task == nullptr)
        {
            return;
        }

        while(true)
        {
            task->current_code = task->pending_code;
            task->pending_code = led_indicator_code::idle;

            switch (task->current_code)
            {
            case led_indicator_code::idle:
#if CONFIG_INTERCOM_LED_GREEN_GPIO_PIN >= 0
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_GREEN_GPIO_PIN), 1);
                vTaskDelay(50 / portTICK_PERIOD_MS);
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_GREEN_GPIO_PIN), 0);
                vTaskDelay(50 / portTICK_PERIOD_MS);
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_GREEN_GPIO_PIN), 1);
                vTaskDelay(50 / portTICK_PERIOD_MS);
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_GREEN_GPIO_PIN), 0);
#endif
                vTaskDelay(850 / portTICK_PERIOD_MS);
            
                break;

            case led_indicator_code::wakeup:
                ESP_LOGD(log_tag, "wakeup");
#if CONFIG_INTERCOM_LED_GREEN_GPIO_PIN >= 0
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_GREEN_GPIO_PIN), 1);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif
                break;

            case led_indicator_code::wifi_error:
                ESP_LOGD(log_tag, "wifi_error");
#if CONFIG_INTERCOM_LED_RED_GPIO_PIN >= 0
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 1);
                vTaskDelay(500 / portTICK_PERIOD_MS);
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                for(int i = 0; i < 2; i++)
                {
                    gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 1);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 0);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
#endif
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                break;

            case led_indicator_code::http_error:
                ESP_LOGD(log_tag, "http_error");
#if CONFIG_INTERCOM_LED_RED_GPIO_PIN >= 0
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 1);
                vTaskDelay(500 / portTICK_PERIOD_MS);
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 0);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                for(int i = 0; i < 4; i++)
                {
                    gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 1);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 0);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
#endif
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                break;

            case led_indicator_code::unknown_error:
                ESP_LOGD(log_tag, "unknown_error");
#if CONFIG_INTERCOM_LED_RED_GPIO_PIN >= 0
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 1);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                gpio_set_level(static_cast<gpio_num_t>(CONFIG_INTERCOM_LED_RED_GPIO_PIN), 0);
#endif
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                task->current_code = led_indicator_code::idle;
                break;    

            default:
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                break;
            }

            task->current_code = task->pending_code;
        }
    }
};

const char* led_indicator_task::log_tag = "led_indicator_task";