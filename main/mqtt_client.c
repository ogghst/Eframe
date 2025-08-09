#include "mqtt_client.h"
#include "esp_log.h"
#include "config_parser.h"
#include "display_manager.h"
#include <stdio.h>
#include "esp_event.h"
#include "mqtt_client.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT_CLIENT";

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your event handler
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            const app_config_t *config = get_config();
            for (int i = 0; i < config->num_widgets; i++) {
                msg_id = esp_mqtt_client_subscribe(client, config->widgets[i].topic, 1);
                ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d, topic=%s", msg_id, config->widgets[i].topic);
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            // Null-terminate topic and data
            char *topic = malloc(event->topic_len + 1);
            memcpy(topic, event->topic, event->topic_len);
            topic[event->topic_len] = '\0';

            char *data = malloc(event->data_len + 1);
            memcpy(data, event->data, event->data_len);
            data[event->data_len] = '\0';

            ESP_LOGI(TAG, "TOPIC=%s", topic);
            ESP_LOGI(TAG, "DATA=%s", data);

            display_update_widget_by_topic(topic, data);

            free(topic);
            free(data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_app_start(void)
{
    const app_config_t *config = get_config();
    if (!config || strlen(config->mqtt.server) == 0) {
        ESP_LOGE(TAG, "MQTT configuration not found, skipping MQTT start.");
        return;
    }

    char broker_url[256];
    snprintf(broker_url, sizeof(broker_url), "mqtt://%s:%d", config->mqtt.server, config->mqtt.port);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_url,
        .credentials.username = config->mqtt.username,
        .credentials.authentication.password = config->mqtt.password,
        .credentials.client_id = config->mqtt.client_id,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}
