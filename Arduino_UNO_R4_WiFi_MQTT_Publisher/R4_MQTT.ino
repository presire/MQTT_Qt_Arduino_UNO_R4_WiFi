#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "config.h"


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
Adafruit_BME280 bme;


// Wi-Fi接続関数
bool connectToWiFi()
{
  int retries = 0;
  
  while (WiFi.status() != WL_CONNECTED && retries < WIFI_MAX_RETRIES) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting to WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Attempt: ");
    lcd.print(retries + 1);
    lcd.print("/");
    lcd.print(WIFI_MAX_RETRIES);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int dotCount = 0;
    unsigned long startAttemptTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_RETRY_DELAY) {
      delay(500);
      lcd.setCursor(dotCount, 2);
      lcd.print(".");
      dotCount = (dotCount + 1) % 20;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Connected!");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
      delay(1000);
      return true;
    }
    
    retries++;
    WiFi.disconnect();
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connection");
  lcd.setCursor(0, 1);
  lcd.print("failed!");

  return false;
}


// MQTT接続関数
bool connectToMQTT()
{
  int retries = 0;
  
  while (!mqttClient.connected() && retries < MQTT_MAX_RETRIES) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting to MQTT");
    lcd.setCursor(0, 1);
    lcd.print("Attempt: ");
    lcd.print(retries + 1);
    lcd.print("/");
    lcd.print(MQTT_MAX_RETRIES);
    
    // クライアントIDを一意に設定
    String clientId = "arduino_client_" + String(random(0xffff), HEX);
    mqttClient.setId(clientId.c_str());
    
    // 認証情報の設定
    mqttClient.setUsernamePassword(MQTT_USER, MQTT_PASSWORD);
    
    if (mqttClient.connect(MQTT_BROKER, MQTT_PORT)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT Connected!");
      delay(1000);

      return true;
    }
    
    retries++;
    lcd.setCursor(0, 2);
    lcd.print("Failed. Error: ");
    lcd.setCursor(0, 3);
    lcd.print(mqttClient.connectError());
    delay(MQTT_RETRY_DELAY);
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MQTT connection");
  lcd.setCursor(0, 1);
  lcd.print("failed!");

  return false;
}


// 接続状態の確認と再接続
bool checkConnections()
{
  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi disconnected");
    lcd.setCursor(0, 1);
    lcd.print("Reconnecting...");
    delay(1000);

    return connectToWiFi() && connectToMQTT();
  }
  
  if (!mqttClient.connected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT disconnected");
    lcd.setCursor(0, 1);
    lcd.print("Reconnecting...");
    delay(1000);

    return connectToMQTT();
  }
  
  return true;
}


void setup()
{
  // 初期設定
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  // LCDの設定
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Start Program");
  lcd.setCursor(0, 1);
  lcd.print("Sensor : BME280");

  // BME280センサの初期化
	if (!bme.begin(0x76)) {
    lcd.setCursor(0, 2);
    lcd.print("Invalid BME280 sensor.");
    lcd.setCursor(0, 3);
    lcd.print("Check wiring!");
    while (1) {
      delay(10);
    };
	}

  delay(2000);

  // WiFiとMQTT接続
  if (!connectToWiFi() || !connectToMQTT()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initial connection");
    lcd.setCursor(0, 1);
    lcd.print("failed. Restarting...");

    delay(3000);

    // ハードウェアリセット
    NVIC_SystemReset();
  }
}

void loop()
{
  delay(2000);

  // 温湿度・大気圧を計測・表示
  float percentHumidity = bme.readHumidity();
  float temperature     = bme.readTemperature();
  float Pressure        = bme.readPressure() / 100.0F;

  if(isnan(percentHumidity) || isnan(temperature)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Error");
      return;
  }

  lcd.clear();

  String strTemp = String("Temp : ");
  strTemp += String(temperature, 1);
  strTemp += "[";
  lcd.print(strTemp);
  lcd.print("\xdf");
  lcd.print("C]");

  lcd.setCursor(0, 1);
  String strHumidity = "Humidity : ";
  strHumidity += String(percentHumidity, 1);
  strHumidity += "[%]";
  lcd.print(strHumidity);

  lcd.setCursor(0, 2);
  String strPressure = "Pre : ";
  strPressure += String(Pressure, 1);
  strPressure += "[hPa]";
  lcd.print(strPressure);

  // MQTT接続の維持
  mqttClient.poll();

  // メッセージの送信
  String mqttMsg = strTemp + "\u00B0" + "C]" + " " + strHumidity + " " + strPressure;
  mqttClient.beginMessage(MQTT_TOPIC);
  mqttClient.print(mqttMsg);
  mqttClient.endMessage();

  digitalWrite(10, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);              // wait for a second
  digitalWrite(10, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);              // wait for a second
}
