#include "app_mqtt.h"
#include "esp_log.h"
#include "config_parser.h"
#include "display_manager.hpp"
#include <stdio.h>
#include "esp_event.h"
#include <stdlib.h>
#include <string.h>
#include <mqtt_client.h>
#include "esp_idf_version.h"

static const char *TAG = "MQTT_CLIENT";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int)event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            const app_config_t *config = get_config();
            if (config) {
                for (int i = 0; i < config->num_widgets; i++) {
                    msg_id = esp_mqtt_client_subscribe(client, config->widgets[i].topic, 1);
                    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d, topic=%s", msg_id, config->widgets[i].topic);
                }
            }
            break;
        }
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
        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            char *topic = (char *)malloc((size_t)event->topic_len + 1);
            char *data = (char *)malloc((size_t)event->data_len + 1);
            if (!topic || !data) {
                free(topic);
                free(data);
                break;
            }
            memcpy(topic, event->topic, (size_t)event->topic_len);
            topic[event->topic_len] = '\0';
            memcpy(data, event->data, (size_t)event->data_len);
            data[event->data_len] = '\0';
            display_update_widget_by_topic(topic, data);
            free(topic);
            free(data);
            break;
        }
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
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

    esp_mqtt_client_config_t mqtt_cfg = {0};
#if ESP_IDF_VERSION_MAJOR >= 5
    mqtt_cfg.broker.address.uri = broker_url;
    mqtt_cfg.credentials.username = config->mqtt.username;
    mqtt_cfg.credentials.authentication.password = config->mqtt.password;
    mqtt_cfg.credentials.client_id = config->mqtt.client_id;
#else
    mqtt_cfg.uri = broker_url;
    mqtt_cfg.username = config->mqtt.username;
    mqtt_cfg.password = config->mqtt.password;
    mqtt_cfg.client_id = config->mqtt.client_id;
#endif

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}
