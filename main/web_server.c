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

#define CONFIG_FILE_PATH "/spiffs/config.json"

static esp_err_t root_get_handler(httpd_req_t *req)
{
    // Check if the file exists
    struct stat st;
    if (stat(CONFIG_FILE_PATH, &st) != -1) {
        // config.json exists, serve it for display
        return on_url_hit(req, "/config.json");
    }
    // else serve index.html
    return on_url_hit(req, "/index.html");
}

static esp_err_t upload_post_handler(httpd_req_t *req)
{
    char buf[1024];
    int ret, remaining = req->content_len;

    // Open file for writing
    FILE *f = fopen(CONFIG_FILE_PATH, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
        return ESP_FAIL;
    }

    // Read the entire request body and write it to the file
    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            fclose(f);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File reception failed");
            return ESP_FAIL;
        }
        fwrite(buf, 1, ret, f);
        remaining -= ret;
    }

    fclose(f);
    httpd_resp_send(req, "File uploaded successfully", HTTPD_200_OK);
    return ESP_OK;
}

static esp_err_t reboot_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Rebooting...");
    httpd_resp_send(req, "Rebooting...", HTTPD_200_OK);
    vTaskDelay(pdMS_TO_TICKS(100)); // Give a moment for the response to be sent
    esp_restart();
    return ESP_OK;
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

    httpd_uri_t root_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_get_handler,
    };
    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t upload_uri = {
        .uri       = "/upload",
        .method    = HTTP_POST,
        .handler   = upload_post_handler,
    };
    httpd_register_uri_handler(server, &upload_uri);

    httpd_uri_t reboot_uri = {
        .uri       = "/reboot",
        .method    = HTTP_POST,
        .handler   = reboot_post_handler,
    };
    httpd_register_uri_handler(server, &reboot_uri);
}
