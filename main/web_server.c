#include "web_server.h"
#include <esp_http_server.h>
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "WEB_SERVER";

static esp_err_t on_url_hit(httpd_req_t *req, const char *url)
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };
    esp_vfs_spiffs_register(&conf);

    char path[600];
    snprintf(path, sizeof(path), "/spiffs%s", url);

    // Check if the file exists
    struct stat st;
    if (stat(path, &st) == -1) {
        // File not found
        httpd_resp_send_404(req);
        return ESP_OK;
    }

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    // Read file content and send it
    char *chunk = malloc(1024);
    size_t chunk_size;
    do {
        chunk_size = fread(chunk, 1, 1024, f);
        if (chunk_size > 0) {
            if (httpd_resp_send_chunk(req, chunk, chunk_size) != ESP_OK) {
                fclose(f);
                free(chunk);
                return ESP_FAIL;
            }
        }
    } while (chunk_size > 0);

    fclose(f);
    free(chunk);
    httpd_resp_send_chunk(req, NULL, 0);

    esp_vfs_spiffs_unregister(NULL);
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    return on_url_hit(req, "/index.html");
}

void start_web_server(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;


    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    httpd_uri_t root = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = root_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &root);
}
