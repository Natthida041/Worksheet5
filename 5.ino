#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid = "nntdy";
const char* password = "123456789";
const char* SERVER_URL = "http://172.20.10.2:3000/Hudi";

const int DHT_PIN = D4;
DHT sensor(DHT_PIN, DHT11);

const long GMT_OFFSET_SEC = 25200;
const int NTP_REFRESH_INTERVAL_MS = 60000;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", GMT_OFFSET_SEC, NTP_REFRESH_INTERVAL_MS);

WiFiClient networkClient;
HTTPClient webClient;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  sensor.begin();
  timeClient.begin();

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected successfully");
}

void loop() {
  static unsigned long lastUpdateTimestamp = 0;
  const unsigned long UPDATE_DELAY_MS = 10000;

  if ((millis() - lastUpdateTimestamp) > UPDATE_DELAY_MS) {
    timeClient.update();

    float humidity = sensor.readHumidity();
    float temperature = sensor.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read data from DHT sensor");
    } else {
      Serial.printf("Humidity: %.2f%%\n", humidity);
      Serial.printf("Temperature: %.2f°C\n", temperature);

      time_t now = timeClient.getEpochTime();
      struct tm* timeDetails = gmtime(&now);
      char formattedTime[25];
      strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", timeDetails);

      StaticJsonDocument<300> jsonDoc;
      JsonObject sensorData = jsonDoc.createNestedObject(formattedTime);
      sensorData["humidity"] = humidity;
      sensorData["temperature"] = temperature;

      String jsonData;
      serializeJson(jsonDoc, jsonData);
      webClient.begin(networkClient, SERVER_URL);
      webClient.addHeader("Content-Type", "application/json");
      int httpResponseCode = webClient.PATCH(jsonData);

      if (httpResponseCode > 0) {
        String response = webClient.getString();
        Serial.printf("HTTP Response Code: %d\n", httpResponseCode);
        Serial.println("Received Payload:");
        Serial.println(response);
      } else {
        Serial.printf("Error sending PATCH request: %d\n", httpResponseCode);
      }

      webClient.end();
    }

    lastUpdateTimestamp = millis();
  }
}
