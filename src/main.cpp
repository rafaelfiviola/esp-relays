#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include "SSD1306Wire.h"
#include "DHT.h"
#include "ota.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* Wifi Config */
#ifndef STASSID
#define STASSID "Pensar rapido"
#define STAPSK "edificil"
#endif

/* DHT Config */
#define DHTTYPE DHT11
uint8_t DHTPin = D7;
DHT dht(DHTPin, DHTTYPE);

/* Display Config */
SSD1306Wire display(0x3c, D3, D5, GEOMETRY_128_32);

/* OTA Config */
OTA ota;
const char *ssid = STASSID;
const char *password = STAPSK;

/* MQTT Config */
WiFiClient espClient;
PubSubClient client(espClient);
const char *mqtt_server = "192.168.1.10";

void setupDisplay()
{
  display.init();
  display.setFont(ArialMT_Plain_10);
}

void setupWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 10, "Connection Failed!");
    Serial.println("Connection Failed! Rebooting...");
    display.display();
    delay(5000);
    ESP.restart();
  }

  /* Connection Successfull */
  Serial.println("Ready");
  Serial.print("IP address: ");
  IPAddress address = WiFi.localIP();
  Serial.println(address);
  display.drawString(0, 0, "Connected!");
  display.drawString(0, 10, "IpAddress:" + address.toString());
  display.display();
}

/* Reconectando ao broker MQTT */
void reconnect()
{
  int numberOfTries = 0;
  while (!client.connected() && numberOfTries < 3)
  {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      client.publish("esp1/status", "connected");
    }
    else
    {
      numberOfTries++;
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup()
{
  /* Basic Info for Debugging */
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Connecting");
  /* Display setup first because wifi setup uses display */
  setupDisplay();
  setupWifi();

  /* DHT Init */
  pinMode(DHTPin, INPUT);
  dht.begin();
  client.setServer(mqtt_server, 1883);

  /* OTA Handles Init */
  ota.init();
  delay(2000);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  else
  {
    display.drawString(0, 80, "MQTT");
  }
  ota.handle();
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  char buffer[512];
  const size_t CAPACITY = JSON_OBJECT_SIZE(2);
  StaticJsonDocument<CAPACITY> doc;

  // create an object
  JsonObject object = doc.to<JsonObject>();
  object["temperature"] = temperature;
  object["humidity"] = humidity;

  serializeJson(object, buffer);
  client.publish("esp1/weather", buffer);

  display.clear();
  display.drawString(0, 0, "Temperature: " + String(temperature, 1) + "ºC");
  display.drawString(0, 10, "Humidity: " + String(humidity) + "%");
  display.drawString(80, 10, client.connected() ? "MQTT" : "N/C");

  display.display();

  delay(5000);
}