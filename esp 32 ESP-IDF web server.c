#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"         // <-- PENTING: Header untuk ESP_LOGI
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#define LED_GPIO_PIN        GPIO_NUM_2     
#define WIFI_SSID           "ESP32_LED_TOMBOL_FINAL"
#define WIFI_PASS           "12345678"
#define TAG                 "WEB_LED_SERVER" 
static int led_state = 0; 


void configure_led(void) {
    gpio_reset_pin(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO_PIN, 0); 
}


void set_led_state(int state) {
    led_state = state;
    gpio_set_level(LED_GPIO_PIN, state);
    ESP_LOGI(TAG, "LED diatur ke: %s (GPIO Level: %d)", state ? "ON" : "OFF", state);
}
const char *HTML_TEMPLATE = 
"<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>ESP32 LED Control</title>"
"<style>"
"body{text-align:center; font-family:sans-serif;} "
".btn{padding: 12px 25px; margin: 10px; font-size: 18px; border: none; border-radius: 8px; cursor: pointer; transition: 0.3s;} "
".on{background-color:#4CAF50; color:white;} "
".off{background-color:#f44336; color:white;} "
".status{margin-top:25px; font-size:22px;}"
"</style>"
"</head>"
"<body>"
"<h1>Kontrol LED ESP32</h1>"
"<p class='status'>Status LED: <b>%s</b></p>"
"<p>"
"<a href='/on'><button class='btn on'>NYALAKAN LED</button></a>"
"<a href='/off'><button class='btn off'>MATIKAN LED</button></a>"
"</p>"
"<p style='font-size:12px; color:gray;'>Akses via: 192.168.4.1</p>"
"</body></html>";
esp_err_t root_get_handler(httpd_req_t *req) {
    char html_response[1500]; 
    ESP_LOGI(TAG, "Permintaan Root (/) diterima."); 
    snprintf(html_response, sizeof(html_response), 
             HTML_TEMPLATE, 
             led_state ? "NYALA" : "MATI");         
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
esp_err_t command_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Perintah kontrol diterima: %s", req->uri); 
    if (strcmp(req->uri, "/on") == 0) {
        set_led_state(1); 
    } else if (strcmp(req->uri, "/off") == 0) {
        set_led_state(0); 
    }
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0); 
    return ESP_OK;
}
static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};
static const httpd_uri_t on_uri = {
    .uri       = "/on",
    .method    = HTTP_GET,
    .handler   = command_handler
};
static const httpd_uri_t off_uri = {
    .uri       = "/off",
    .method    = HTTP_GET,
    .handler   = command_handler
};


void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init()); ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    }; ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "AP dimulai. SSID: %s. Akses: http://192.168.4.1", WIFI_SSID); 
}
httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG()
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &on_uri);
        httpd_register_uri_handler(server, &off_uri);
        ESP_LOGI(TAG, "Web Server berjalan."); 
    }
    return server;
}


void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    configure_led();
    set_led_state(0); 
    wifi_init_softap();
    start_webserver();
}
