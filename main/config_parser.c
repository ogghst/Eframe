#include "config_parser.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "CONFIG_PARSER";
static app_config_t app_config;

static void parse_mqtt_config(cJSON *mqtt_json, mqtt_config_t *mqtt_config) {
    cJSON *server = cJSON_GetObjectItem(mqtt_json, "server");
    if (cJSON_IsString(server)) strncpy(mqtt_config->server, server->valuestring, sizeof(mqtt_config->server) - 1);

    cJSON *port = cJSON_GetObjectItem(mqtt_json, "port");
    if (cJSON_IsNumber(port)) mqtt_config->port = port->valueint;

    cJSON *username = cJSON_GetObjectItem(mqtt_json, "username");
    if (cJSON_IsString(username)) strncpy(mqtt_config->username, username->valuestring, sizeof(mqtt_config->username) - 1);

    cJSON *password = cJSON_GetObjectItem(mqtt_json, "password");
    if (cJSON_IsString(password)) strncpy(mqtt_config->password, password->valuestring, sizeof(mqtt_config->password) - 1);

    cJSON *client_id = cJSON_GetObjectItem(mqtt_json, "client_id");
    if (cJSON_IsString(client_id)) strncpy(mqtt_config->client_id, client_id->valuestring, sizeof(mqtt_config->client_id) - 1);
}

static void parse_widgets_config(cJSON *widgets_json, app_config_t *config) {
    config->num_widgets = cJSON_GetArraySize(widgets_json);
    if (config->num_widgets > 10) config->num_widgets = 10;

    for (int i = 0; i < config->num_widgets; i++) {
        cJSON *widget_json = cJSON_GetArrayItem(widgets_json, i);
        widget_config_t *widget_config = &config->widgets[i];

        cJSON *name = cJSON_GetObjectItem(widget_json, "name");
        if (cJSON_IsString(name)) strncpy(widget_config->name, name->valuestring, sizeof(widget_config->name) - 1);

        cJSON *type = cJSON_GetObjectItem(widget_json, "type");
        if (cJSON_IsString(type)) strncpy(widget_config->type, type->valuestring, sizeof(widget_config->type) - 1);

        cJSON *topic = cJSON_GetObjectItem(widget_json, "topic");
        if (cJSON_IsString(topic)) strncpy(widget_config->topic, topic->valuestring, sizeof(widget_config->topic) - 1);

        cJSON *position = cJSON_GetObjectItem(widget_json, "position");
        if (position) {
            cJSON *x = cJSON_GetObjectItem(position, "x");
            if (cJSON_IsNumber(x)) widget_config->position.x = x->valueint;
            cJSON *y = cJSON_GetObjectItem(position, "y");
            if (cJSON_IsNumber(y)) widget_config->position.y = y->valueint;
        }

        cJSON *size = cJSON_GetObjectItem(widget_json, "size");
        if (size) {
            cJSON *width = cJSON_GetObjectItem(size, "width");
            if (cJSON_IsNumber(width)) widget_config->size.width = width->valueint;
            cJSON *height = cJSON_GetObjectItem(size, "height");
            if (cJSON_IsNumber(height)) widget_config->size.height = height->valueint;
        }
    }
}

static void parse_buttons_config(cJSON *buttons_json, app_config_t *config) {
    config->num_buttons = cJSON_GetArraySize(buttons_json);
    if (config->num_buttons > 4) config->num_buttons = 4;

    for (int i = 0; i < config->num_buttons; i++) {
        cJSON *button_json = cJSON_GetArrayItem(buttons_json, i);
        button_config_t *button_config = &config->buttons[i];

        cJSON *gpio = cJSON_GetObjectItem(button_json, "gpio");
        if (cJSON_IsNumber(gpio)) button_config->gpio = gpio->valueint;

        cJSON *action = cJSON_GetObjectItem(button_json, "action");
        if (action) {
            cJSON *type = cJSON_GetObjectItem(action, "type");
            if (cJSON_IsString(type)) strncpy(button_config->action.type, type->valuestring, sizeof(button_config->action.type) - 1);
            cJSON *topic = cJSON_GetObjectItem(action, "topic");
            if (cJSON_IsString(topic)) strncpy(button_config->action.topic, topic->valuestring, sizeof(button_config->action.topic) - 1);
            cJSON *payload = cJSON_GetObjectItem(action, "payload");
            if (cJSON_IsString(payload)) strncpy(button_config->action.payload, payload->valuestring, sizeof(button_config->action.payload) - 1);
        }
    }
}

bool load_config(void) {
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    esp_vfs_spiffs_register(&conf);

    FILE *f = fopen("/spiffs/config.json", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "config.json not found");
        esp_vfs_spiffs_unregister(NULL);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    fclose(f);
    buffer[size] = '\0';

    esp_vfs_spiffs_unregister(NULL);

    cJSON *root = cJSON_Parse(buffer);
    free(buffer);

    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse config file: [%s]", cJSON_GetErrorPtr());
        return false;
    }

    memset(&app_config, 0, sizeof(app_config_t));

    cJSON *mqtt_json = cJSON_GetObjectItem(root, "mqtt");
    if (mqtt_json) parse_mqtt_config(mqtt_json, &app_config.mqtt);

    cJSON *widgets_json = cJSON_GetObjectItem(root, "widgets");
    if (widgets_json) parse_widgets_config(widgets_json, &app_config);

    cJSON *buttons_json = cJSON_GetObjectItem(root, "buttons");
    if (buttons_json) parse_buttons_config(buttons_json, &app_config);

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Configuration loaded successfully");
    return true;
}

const app_config_t* get_config(void) {
    return &app_config;
}
