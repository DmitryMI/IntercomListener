#pragma once

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "sdkconfig.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#if CONFIG_INTERCOM_WPA3_SAE_PWE_HUNT_AND_PECK
    #define INTERCOM_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
    #define INTERCOM_WIFI_H2E_IDENTIFIER ""
#elif CONFIG_INTERCOM_WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define INTERCOM_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
    #define INTERCOM_WIFI_H2E_IDENTIFIER CONFIG_INTERCOM_WIFI_PW_ID
#elif CONFIG_INTERCOM_WPA3_SAE_PWE_BOTH
    #define INTERCOM_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
    #define INTERCOM_WIFI_H2E_IDENTIFIER CONFIG_INTERCOM_WIFI_PW_ID
#else

#endif

#if CONFIG_INTERCOM_WIFI_AUTH_OPEN
    #define INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_INTERCOM_WIFI_AUTH_WEP
    #define INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_INTERCOM_WIFI_AUTH_WPA_PSK
    #define INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_INTERCOM_WIFI_AUTH_WPA2_PSK
    #define INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_INTERCOM_WIFI_AUTH_WPA_WPA2_PSK
    #define INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_INTERCOM_WIFI_AUTH_WPA3_PSK
    #define INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_INTERCOM_WIFI_AUTH_WPA2_WPA3_PSK
    #define INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_INTERCOM_WIFI_AUTH_WAPI_PSK
    #define INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif


/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";
static int s_retry_num = 0;
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

