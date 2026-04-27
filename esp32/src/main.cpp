#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <PubSubClient.h>

// ── Pin Definitions ──────────────────────────────────────────────
#define ONE_WIRE_BUS 4
#define GREEN_LED    12
#define RED_LED      14
#define YELLOW_LED   13

// ── OLED Display (I2C: SDA=GPIO21, SCL=GPIO22) ──────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ── Temperature Sensor ───────────────────────────────────────────
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const float TEMP_MIN = 19.0;
const float TEMP_MAX = 26.0;
const int   MEASURE_INTERVAL = 5000;

float currentTemp = 0.0;
unsigned long lastMeasure = 0;
String tempStatus = "";

// WiFi + HTTP Server
const char* ssid     = "WLAN-ESP";
const char* password = "agsesp32";
WebServer server(80);

// MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);

const int   mqtt_port      = 1883;
const char* mqtt_host      = "10.204.119.42";  // Raspberry Pi IP
const char* mqtt_topic     = "iot/esp32/temperature";
const char* mqtt_client_id = "esp32-temp-01";
const char* mqtt_user      = "mqttuser";        // MQTT auth username
const char* mqtt_pass      = "mqttpass";        // MQTT auth password

// API Handler Interface
class APIHandler {
public:
  virtual ~APIHandler() {}
  virtual void handle() = 0;
  virtual String getEndpoint() = 0;
};

// Temperature API Handler
class TemperatureAPIHandler : public APIHandler {
public:
  void handle() override {
    DynamicJsonDocument doc(256);
    doc["temperature"] = currentTemp;
    doc["status"] = tempStatus;
    doc["minTemp"] = TEMP_MIN;
    doc["maxTemp"] = TEMP_MAX;

    String json;
    serializeJson(doc, json);

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
  }

  String getEndpoint() override {
    return "/api/temperature";
  }
};

// Config API Handler
class ConfigAPIHandler : public APIHandler {
public:
  void handle() override {
    DynamicJsonDocument doc(256);
    doc["minTemp"]          = TEMP_MIN;
    doc["maxTemp"]          = TEMP_MAX;
    doc["measureInterval"]  = MEASURE_INTERVAL;

    String json;
    serializeJson(doc, json);

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
  }

  String getEndpoint() override {
    return "/api/config";
  }
};

// LED Controller
void updateLEDs() {
  if (currentTemp >= TEMP_MIN && currentTemp <= TEMP_MAX) {
    tempStatus = "optimal";
    digitalWrite(GREEN_LED,  HIGH);
    digitalWrite(RED_LED,    LOW);
    digitalWrite(YELLOW_LED, LOW);
  }
  else if (currentTemp > TEMP_MAX) {
    tempStatus = "too_high";
    digitalWrite(GREEN_LED,  LOW);
    digitalWrite(RED_LED,    HIGH);
    digitalWrite(YELLOW_LED, LOW);
  }
  else {
    tempStatus = "too_low";
    digitalWrite(GREEN_LED,  LOW);
    digitalWrite(RED_LED,    LOW);
    digitalWrite(YELLOW_LED, HIGH);
  }
}

// Display Controller
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(currentTemp, 1);
  display.println(" C");

  display.setCursor(0, 12);
  display.print("Range: ");
  display.print(TEMP_MIN, 0);
  display.print("-");
  display.print(TEMP_MAX, 0);
  display.println(" C");

  display.setCursor(0, 24);
  display.print("Status: ");
  display.println(tempStatus);

  display.setCursor(0, 36);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi OK: ");
    display.println(WiFi.localIP());
  } else {
    display.println("WiFi: Not connected");
  }

  display.setCursor(0, 48);
  display.print("MQTT: ");
  display.println(mqttClient.connected() ? "OK" : "DISCONNECTED");

  display.display();
}

// CORS Preflight
void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200);
}

// MQTT Reconnect (non-blocking, max 3 attempts)
void mqttReconnect() {
  int attempts = 0;
  while (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && attempts < 3) {
    Serial.print("MQTT connecting (attempt ");
    Serial.print(attempts + 1);
    Serial.print("/3)... ");

    if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      Serial.println("OK");
      // optional: mqttClient.subscribe("iot/esp32/cmd/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry in 2s");
      delay(2000);
    }
    attempts++;
  }
}

// MQTT Publish
void publishTemperatureMqtt() {
  if (!mqttClient.connected()) return;

  DynamicJsonDocument doc(256);
  doc["temperature"] = currentTemp;
  doc["status"]      = tempStatus;
  doc["minTemp"]     = TEMP_MIN;
  doc["maxTemp"]     = TEMP_MAX;
  doc["ts_ms"]       = (uint32_t)millis();

  char payload[256];
  size_t n = serializeJson(doc, payload, sizeof(payload));

  bool ok = mqttClient.publish(mqtt_topic,
                               reinterpret_cast<const uint8_t*>(payload),
                               n, false);

  Serial.print("MQTT publish: ");
  Serial.println(ok ? "OK" : "FAILED");
}

// Temperature Measurement
void measureTemperature() {
  sensors.requestTemperatures();
  currentTemp = sensors.getTempCByIndex(0);

  Serial.print("Temperature: ");
  Serial.print(currentTemp);
  Serial.println(" C");

  updateLEDs();
  updateDisplay();
  publishTemperatureMqtt();
}

//Initialization
void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error: OLED not found!");
    while (1) { delay(100); }
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();
  delay(1000);
}

void initLEDs() {
  pinMode(GREEN_LED,  OUTPUT);
  pinMode(RED_LED,    OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
}

void initWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();

  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("WiFi: OK");
    display.println(WiFi.localIP());
    display.display();
    delay(1500);
  } else {
    Serial.println("\nWiFi error!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("WiFi: error!");
    display.display();
  }
}

void initServer() {
  static TemperatureAPIHandler tempHandler;
  static ConfigAPIHandler configHandler;

  server.on("/api/temperature", HTTP_GET,     []() { tempHandler.handle(); });
  server.on("/api/config",      HTTP_GET,     []() { configHandler.handle(); });
  server.on("/api/temperature", HTTP_OPTIONS, handleOptions);
  server.on("/api/config",      HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println("API Server started!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\nESP32 Engine starting...");

  initDisplay();
  initLEDs();
  sensors.begin();
  initWiFi();

  // MQTT init
  mqttClient.setServer(mqtt_host, mqtt_port);
  if (WiFi.status() == WL_CONNECTED) {
    mqttReconnect();
  }

  initServer();

  measureTemperature();
  lastMeasure = millis();
}

void loop() {
  server.handleClient();

  // MQTT keep-alive + reconnect
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) mqttReconnect();
    mqttClient.loop();
  }

  if (millis() - lastMeasure >= (unsigned long)MEASURE_INTERVAL) {
    measureTemperature();
    lastMeasure = millis();
  }

  delay(50);
}
