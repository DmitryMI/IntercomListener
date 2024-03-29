#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_INTERCOM_LOG_LEVEL_ERROR
#define INTERCOM_LOG_LEVEL ESP_LOG_ERROR
#elif CONFIG_INTERCOM_LOG_LEVEL_WARNING
#define INTERCOM_LOG_LEVEL ESP_LOG_WARNING
#elif CONFIG_INTERCOM_LOG_LEVEL_INFO
#define INTERCOM_LOG_LEVEL ESP_LOG_INFO
#elif CONFIG_INTERCOM_LOG_LEVEL_DEBUG
#define INTERCOM_LOG_LEVEL ESP_LOG_DEBUG
#elif CONFIG_INTERCOM_LOG_LEVEL_VERBOSE
#define INTERCOM_LOG_LEVEL ESP_LOG_VERBOSE
#else
#define INTERCOM_LOG_LEVEL ESP_LOG_ERROR
#endif