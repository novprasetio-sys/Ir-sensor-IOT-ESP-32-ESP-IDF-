#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "driver/gpio.h"

#define WIFI_SSID       "WIFI_KAMU"
#define WIFI_PASS       "PASSWORD_KAMU"

#define IR_PIN          4   // ubah sesuai wiring kamu
#define API_KEY         "APIKEY_THINGSPEAK"
#define WRITE_URL       "http://api.thingspeak.com/update"

static const char *TAG = "APP";

void send_to_thingspeak(int value) {
char url[200];
sprintf(url, "%s?api_key=%s&field1=%d", WRITE_URL, API_KEY, value);

esp_http_client_config_t config = {  
    .url = url,  
};  

esp_http_client_handle_t client = esp_http_client_init(&config);  
esp_http_client_perform(client);  
esp_http_client_cleanup(client);  

ESP_LOGI(TAG, "Sent to Thingspeak: %d", value);

}

void wifi_init() {
ESP_LOGI(TAG, "Init NVS");
nvs_flash_init();

esp_netif_init();  
esp_event_loop_create_default();  
esp_netif_create_default_wifi_sta();  

wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();  
esp_wifi_init(&cfg);  

wifi_config_t wifi_config = {  
    .sta = {  
        .ssid = WIFI_SSID,  
        .password = WIFI_PASS,  
    },  
};  

esp_wifi_set_mode(WIFI_MODE_STA);  
esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);  
esp_wifi_start();  
esp_wifi_connect();

}

void app_main() {
wifi_init();

gpio_config_t io = {  
    .pin_bit_mask = 1ULL << IR_PIN,  
    .mode = GPIO_MODE_INPUT,  
    .pull_up_en = GPIO_PULLUP_DISABLE,  
    .pull_down_en = GPIO_PULLDOWN_DISABLE  
};  
gpio_config(&io);  

while (1) {  
    int raw = gpio_get_level(IR_PIN);  

    // Active low → 0 = detected, 1 = no object  
    int detected = (raw == 0) ? 1 : 0;  

    ESP_LOGI(TAG, "IR RAW=%d, DETECT=%d", raw, detected);  

    send_to_thingspeak(detected);  

    vTaskDelay(20000 / portTICK_PERIOD_MS); // 20 detik  
}

}