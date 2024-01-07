#include "wifi.h"
#include "log_level.h"
#include "events.h"

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if(!wifi_enabled)
        {
            ESP_LOGI(wifi_log_tag, "WIFI_EVENT_STA_DISCONNECTED event fired");
             xEventGroupSetBits(wifi_event_group, EVENT_WIFI_DISCONNECTED);
            return;
        }
        if (retry_num < CONFIG_INTERCOM_WIFI_MAXIMUM_RETRY) 
        {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(wifi_log_tag, "retry to connect to the AP");
        } 
        else
        {
            ESP_LOGI(wifi_log_tag, "max number of reties reached");
            xEventGroupSetBits(wifi_event_group, EVENT_WIFI_FAIL);
        } 
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(wifi_log_tag, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupSetBits(wifi_event_group, EVENT_WIFI_CONNECTED);
    }
}

void wifi_deinit_and_stop(void)
{
    wifi_enabled = 0;
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, IP_EVENT_STA_GOT_IP, &instance_got_ip));
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}

bool wifi_init_sta(EventGroupHandle_t event_group_handle)
{
    esp_log_level_set(wifi_log_tag, INTERCOM_LOG_LEVEL);
    wifi_event_group = event_group_handle;

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_INTERCOM_WIFI_SSID,
            .password = CONFIG_INTERCOM_WIFI_PASSWORD,
            .threshold.authmode = INTERCOM_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            #ifdef INTERCOM_WIFI_SAE_MODE
            .sae_pwe_h2e = INTERCOM_WIFI_SAE_MODE,
            .sae_h2e_identifier = INTERCOM_WIFI_H2E_IDENTIFIER,
            #endif
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    wifi_enabled = true;
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGD(wifi_log_tag, "wifi_init_sta finished.");

    return true;
}