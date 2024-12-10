#ifndef CONFIG_H
#define CONFIG_H

// WiFi設定
#define WIFI_SSID         "<Wi-Fi SSID>"
#define WIFI_PASSWORD     "<Wi-Fi Password>"
#define WIFI_RETRY_DELAY  5000                  // Wi-Fi再接続の待機時間 (ミリ秒)
#define WIFI_MAX_RETRIES  5                     // Wi-Fi接続の最大リトライ回数

// MQTT設定
#define MQTT_BROKER     "<MQTT Broker's Hostname or IP adress>"
#define MQTT_PORT       1883
#define MQTT_USER       "<MQTT Broker user-name>"
#define MQTT_PASSWORD   "<MQTT Broker password>"
#define MQTT_TOPIC      "arduino/sensor"        // MQTTトピック名
#define MQTT_RETRY_DELAY 3000                   // MQTT再接続の待機時間 (ミリ秒)
#define MQTT_MAX_RETRIES 3                      // MQTT接続の最大リトライ回数

// ハードウェア設定
#define LED_PIN                 10
#define SEALEVELPRESSURE_HPA    (1013.25)
#define LCD_ADDRESS             0x3f            // LCDコントローラによっては0x27を指定する
#define LCD_COLS                20
#define LCD_ROWS                4
#define BME280_ADDRESS          0x76

#endif // CONFIG_H
