#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/timer.h"
#include "soc/rtc.h"
#include "events.h"
#include "log_level.h"

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider

static const char* timer_log_tag = "timer";

EventGroupHandle_t timer_event_group;

uint32_t get_apb_freq()
{
    return 80000000;
}

struct timer_info 
{
    timer_group_t timer_group;
    timer_idx_t timer_idx;
    int alarm_interval;
    bool auto_reload;
};

bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;

    //xQueueSendFromISR(timer_queue, &evt, &high_task_awoken);
    int higherPriorityTaskWoken = false;
    int result = xEventGroupSetBitsFromISR(timer_event_group, EVENT_TIMER_ALARM, &higherPriorityTaskWoken);
    if(result != pdFAIL)
    {
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }

    return high_task_awoken == pdTRUE;
}

void timer_reset(int timer_interval_sec)
{
    ESP_LOGI(timer_log_tag, "timer_reset called");
    const auto group = timer_group_t::TIMER_GROUP_0;
    const auto index = timer_idx_t::TIMER_0;

    ESP_ERROR_CHECK(timer_set_counter_value(group, index, 0));
    ESP_ERROR_CHECK(timer_set_alarm(group, index, TIMER_ALARM_EN));
}

void timer_setup(int timer_interval_sec, EventGroupHandle_t event_group_handle)
{
    esp_log_level_set(timer_log_tag, INTERCOM_LOG_LEVEL);    

    timer_event_group = event_group_handle;

    const auto group = timer_group_t::TIMER_GROUP_0;
    const auto index = timer_idx_t::TIMER_0;
    const auto reload = timer_autoreload_t::TIMER_AUTORELOAD_DIS;

    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {};
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.auto_reload = reload;

    timer_init(group, index, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, index, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    const uint32_t timer_scale = rtc_clk_apb_freq_get() / TIMER_DIVIDER;
    timer_set_alarm_value(group, index, timer_interval_sec * timer_scale);
    timer_enable_intr(group, index);

    timer_info *info = new timer_info();
    info->timer_group = group;
    info->timer_idx = index;
    info->auto_reload = reload;
    info->alarm_interval = timer_interval_sec;
    timer_isr_callback_add(group, index, timer_group_isr_callback, info, 0);

    timer_start(group, index);
}