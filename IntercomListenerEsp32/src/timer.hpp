#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/timer.h"
#include "soc/rtc.h"

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider

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

/**
 * @brief A sample structure to pass events from the timer ISR to task
 *
 */
struct timer_event
 {
    timer_info info;
    uint64_t timer_counter_value;
    uint64_t timer_time_seconds;
};

QueueHandle_t timer_queue;

bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    timer_info *info = static_cast<timer_info*>(args);

    uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(info->timer_group, info->timer_idx);
    const uint32_t timer_scale = rtc_clk_apb_freq_get() / TIMER_DIVIDER;
    timer_event evt = {};
    evt.timer_counter_value = timer_counter_value;
    evt.timer_time_seconds = timer_counter_value / timer_scale; 
    evt.info.timer_group = info->timer_group;
    evt.info.timer_idx = info->timer_idx;
    evt.info.auto_reload = info->timer_group;
    evt.info.alarm_interval = info->alarm_interval;

    xQueueSendFromISR(timer_queue, &evt, &high_task_awoken);

    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

void timer_reset()
{
    const auto group = timer_group_t::TIMER_GROUP_0;
    const auto index = timer_idx_t::TIMER_0;
    timer_set_counter_value(group, index, 0);
}

void timer_setup(int timer_interval_sec)
{
    const auto group = timer_group_t::TIMER_GROUP_0;
    const auto index = timer_idx_t::TIMER_0;
    const auto reload = timer_autoreload_t::TIMER_AUTORELOAD_DIS;

    timer_queue = xQueueCreate(10, sizeof(timer_event));

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