#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Pin Definitions
#define ONE_WIRE_BUS 4
#define GREEN_LED 14
#define RED_LED 12
#define YELLOW_LED 13

// OLED Display (I2C: SDA=GPIO21, SCL=GPIO22)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const float TEMP_MIN = 19.0;
const float TEMP_MAX = 26.0;
const int MEASURE_INTERVAL = 5000;

float currentTemp = 0.0;
unsigned long lastMeasure = 0;
String tempStatus = "";

// WiFi + Server
const char* ssid = "WLAN-ESP";
const char* password = "agsesp32";
WebServer server(80);

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
    doc["minTemp"] = TEMP_MIN;
    doc["maxTemp"] = TEMP_MAX;
    doc["measureInterval"] = MEASURE_INTERVAL;

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
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
  }
  else if (currentTemp > TEMP_MAX) {
    tempStatus = "too_high";
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
  }
  else {
    tempStatus = "too_low";
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
  }
}

// Display Controller
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Temperature: ");
  display.print(currentTemp, 1);
  display.println("C");

  display.setTextSize(1);
  display.setCursor(0, 25);
  display.print("Range: ");
  display.print(TEMP_MIN, 0);
  display.print("-");
  display.print(TEMP_MAX, 0);
  display.println("C");

  display.setCursor(0, 35);
  display.setTextSize(1);
  if (tempStatus == "optimal") {
    display.println("Status: Optimal [GREEN]");
  }
  else if (tempStatus == "too_high") {
    display.println("Status: Too HIGH! [RED]");
  }
  else {
    display.println("Status: Too LOW! [YEL]");
  }

  display.setCursor(0, 50);
  display.setTextSize(1);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi: OK ");
    display.println(WiFi.localIP());
  } else {
    display.println("WiFi: Not connected");
  }

  display.display();
}

// Temperature measurement
void measureTemperature() {
  sensors.requestTemperatures();
  currentTemp = sensors.getTempCByIndex(0);

  Serial.print("Temperature: ");
  Serial.print(currentTemp);
  Serial.println(" C");

  updateLEDs();
  updateDisplay();
}

// CORS preflight
void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(200);
}

// Initialization Methods
void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error: OLED not found!");
    while (1);
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
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
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
    delay(2000);
  }
  else {
    Serial.println("\nWiFi error!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("WiFi: error!");
    display.display();
  }
}

void initServer() {
  TemperatureAPIHandler tempHandler;
  ConfigAPIHandler configHandler;

  server.on("/api/temperature", HTTP_GET, [&tempHandler]() { tempHandler.handle(); });
  server.on("/api/config", HTTP_GET, [&configHandler]() { configHandler.handle(); });
  server.on("/api/temperature", HTTP_OPTIONS, handleOptions);
  server.on("/api/config", HTTP_OPTIONS, handleOptions);

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
  initServer();

  measureTemperature();
  lastMeasure = millis();
}

void loop() {
  server.handleClient();
  
  if (millis() - lastMeasure >= MEASURE_INTERVAL) {
    measureTemperature();
    lastMeasure = millis();
  }
  
  delay(50);
}
