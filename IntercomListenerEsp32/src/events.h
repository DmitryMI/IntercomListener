#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#define EVENT_TIMER_ALARM BIT0
#define EVENT_RING_SENSOR_START BIT1
#define EVENT_RING_SENSOR_END BIT2
#define EVENT_WIFI_CONNECTED BIT3
#define EVENT_WIFI_DISCONNECTED BIT4
#define EVENT_WIFI_FAIL BIT5
#define EVENT_DOOR_SENSOR_START BIT6
#define EVENT_DOOR_SENSOR_END BIT7

#define EVENT_ALL (BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7)
