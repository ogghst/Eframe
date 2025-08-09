idf_component_register(SRCS "main.c"
                           "wifi_provisioning.c"
                           "web_server.c"
                           "mqtt_client.c"
                           "display_manager.cpp"
                           "button_handler.c"
                           "config_parser.c"
                    INCLUDE_DIRS "."
                     REQUIRES json waveshare_epd esp_http_server esp_wifi mqtt spiffs wifi_provisioning nvs_flash)
