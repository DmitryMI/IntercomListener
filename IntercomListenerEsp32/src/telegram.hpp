#pragma once

#if CONFIG_INTERCOM_TELEGRAM_ENABLED

#include "esp_http_client.h"
#include <string>
#include "esp_log.h"
#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#define TELEGRAM_HOSTNAME "api.telegram.org"
#define MAX_HTTP_OUTPUT_BUFFER 512
#define MAX_HTTP_INPUT_BUFFER 512

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

const char *tg_log_tag = "telegram";

extern const char postman_root_cert_pem_start[] asm("_binary_postman_root_cert_pem_start");
extern const char postman_root_cert_pem_end[]   asm("_binary_postman_root_cert_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    // static char *output_buffer;  // Buffer to store response of http request from event handler
    // static int output_len;       // Stores number of bytes read
    switch(evt->event_id)
    {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(tg_log_tag, "HTTP_EVENT_ERROR");
            break;

        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(tg_log_tag, "HTTP_EVENT_ON_CONNECTED");
            break;

        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(tg_log_tag, "HTTP_EVENT_HEADER_SENT");
            break;

        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(tg_log_tag, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;

        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(tg_log_tag, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(tg_log_tag, "HTTP_EVENT_ON_FINISH");
            break;

        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(tg_log_tag, "HTTP_EVENT_DISCONNECTED");
            break;

        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(tg_log_tag, "HTTP_EVENT_REDIRECT");
            break;

        default:
            ESP_LOGW(tg_log_tag, "Unexpected HTTP event: %d", evt->event_id);
            break;
    }

    return ESP_OK;
}

int telegram_send_notification(const char* text)
{
    esp_log_level_set(tg_log_tag, INTERCOM_LOG_LEVEL);    

    ESP_LOGI(tg_log_tag, "telegram_send_notification called");
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
    /**
     * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path parameters.
     * If host and path parameters are not set, query parameter will be ignored. In such cases,
     * query parameter should be specified in URL.
     *
     * If URL as well as host and path parameters are specified, values of host and path will be considered.
     */
    esp_http_client_config_t config = {};
    config.host = TELEGRAM_HOSTNAME;
    config.path = "/bot" CONFIG_INTERCOM_TELEGRAM_API_KEY "/sendMessage";
    config.event_handler = _http_event_handler;
    config.user_data = local_response_buffer;        // Pass address of local buffer to get response
    config.disable_auto_redirect = true;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(tg_log_tag, "esp_http_client_init called");
    // POST
    const char* post_data_base = 
        "{" \
            "\"chat_id\": \"" CONFIG_INTERCOM_TELEGRAM_CHAT_ID "\", " \
            "\"text\": \"%s\", " \
            "\"parse_mode\": \"HTML\", " \
            "\"disable_notification\": false" \
        "}";

    char post_data[MAX_HTTP_INPUT_BUFFER];
    int post_data_len = sprintf(post_data, post_data_base, text);
    ESP_LOGI(tg_log_tag, "JSON Payload len: %d, text: %s", post_data_len, post_data);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, post_data_len);
    esp_err_t err = esp_http_client_perform(client);

    int status_code = esp_http_client_get_status_code(client);

    if (err == ESP_OK) 
    {
        ESP_LOGI(tg_log_tag, "HTTP POST Status = %d, content_length = %" PRId64,
                status_code,
                esp_http_client_get_content_length(client));
    } else 
    {
        ESP_LOGE(tg_log_tag, "HTTP POST request failed: %s", esp_err_to_name(err));
        status_code = -1;
    }

    esp_http_client_cleanup(client);

    return status_code;
}

#endif