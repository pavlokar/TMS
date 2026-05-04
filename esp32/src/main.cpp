#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// Pin Definitions
#define ONE_WIRE_BUS 4
#define GREEN_LED    12
#define RED_LED      14
#define YELLOW_LED   13
#define SERVO_PIN    27

// OLED Display
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Servo
Servo ventServo;
int currentServoAngle = 0;

// Constants
const float TEMP_MIN = 19.0;
const float TEMP_MAX = 26.0;
const int MEASURE_INTERVAL = 5000;

// States
float currentTemp = 0.0;
unsigned long lastMeasure = 0;
String tempStatus = "";

// WiFi
const char *ssid = "WLAN-ESP";
const char *password = "agsesp32";

// MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);

const int mqtt_port = 1883;
const char *mqtt_host = "10.204.119.42";
const char *mqtt_client_id = "esp32-temp-01";
const char *mqtt_user = "mqttuser";
const char *mqtt_pass = "mqttpass";

// Topics
const char *topic_pub_temp = "iot/esp32/temperature";
const char *topic_cmd_led = "iot/esp32/cmd/led";
const char *topic_cmd_servo = "iot/esp32/cmd/servo";


void mqttCallback(char *topic, byte *payload, unsigned int length) {
    // Null-terminate payload
    char msg[64];
    unsigned int len = min(length, (unsigned int) (sizeof(msg) - 1));
    memcpy(msg, payload, len);
    msg[len] = '\0';

    Serial.printf("MQTT recv [%s]: %s\n", topic, msg);

    // LED command: "green", "red", "yellow", "off"
    if (strcmp(topic, topic_cmd_led) == 0) {
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);

        if (strcmp(msg, "green") == 0) {
            digitalWrite(GREEN_LED, HIGH);
        } else if (strcmp(msg, "red") == 0) {
            digitalWrite(RED_LED, HIGH);
        } else if (strcmp(msg, "yellow") == 0) {
            digitalWrite(YELLOW_LED, HIGH);
        }
        // "off" -> all remain LOW
    }

    // Servo command: angle 0-180
    if (strcmp(topic, topic_cmd_servo) == 0) {
        int angle = atoi(msg);
        angle = constrain(angle, 0, 180);
        ventServo.write(angle);
        currentServoAngle = angle;
        Serial.printf("Servo -> %d deg\n", angle);
    }
}


void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.printf("Temp: %.1f C", currentTemp);

    display.setCursor(0, 12);
    display.printf("Range: %.0f-%.0f C", TEMP_MIN, TEMP_MAX);

    display.setCursor(0, 24);
    display.printf("Status: %s", tempStatus.c_str());

    display.setCursor(0, 36);
    display.printf("Servo: %d deg", currentServoAngle);

    display.setCursor(0, 48);
    display.printf("MQTT: %s", mqttClient.connected() ? "OK" : "DISC");

    display.display();
}

void mqttReconnect() {
    int attempts = 0;
    while (WiFi.status() == WL_CONNECTED && !mqttClient.connected() && attempts < 3) {
        Serial.printf("MQTT connecting (%d/3)... ", attempts + 1);

        if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
            Serial.println("OK");
            // Subscribe to command topics
            mqttClient.subscribe(topic_cmd_led);
            mqttClient.subscribe(topic_cmd_servo);
            Serial.println("Subscribed to cmd topics");
        } else {
            Serial.printf("failed rc=%d\n", mqttClient.state());
            delay(2000);
        }
        attempts++;
    }
}

void publishTemperature() {
    if (!mqttClient.connected()) return;

    DynamicJsonDocument doc(256);
    doc["temperature"] = currentTemp;
    doc["status"] = tempStatus;
    doc["device_id"] = mqtt_client_id;
    doc["minTemp"] = TEMP_MIN;
    doc["maxTemp"] = TEMP_MAX;
    doc["servo"] = currentServoAngle;

    char payload[256];
    size_t n = serializeJson(doc, payload, sizeof(payload));

    bool ok = mqttClient.publish(topic_pub_temp,
                                 reinterpret_cast<const uint8_t *>(payload),
                                 n, false);
    Serial.printf("MQTT pub: %s\n", ok ? "OK" : "FAIL");
}

// Temperature measurement
void measureTemperature() {
    sensors.requestTemperatures();
    currentTemp = sensors.getTempCByIndex(0);

    if (currentTemp >= TEMP_MIN && currentTemp <= TEMP_MAX) {
        tempStatus = "optimal";
    } else if (currentTemp > TEMP_MAX) {
        tempStatus = "too_high";
    } else {
        tempStatus = "too_low";
    }

    Serial.printf("Temp: %.1f C [%s]\n", currentTemp, tempStatus.c_str());

    updateDisplay();
    publishTemperature();
}

// Initialization Methods
void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED not found!");
        while (1) delay(100);
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Starting...");
    display.display();
    delay(1000);
}

void initWiFi() {
    Serial.printf("WiFi: connecting to %s\n", ssid);
    WiFi.begin(ssid, password);

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi OK - IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nWiFi FAILED");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n== ESP32 CPS Start ==");

    // Hardware init
    initDisplay();
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(YELLOW_LED, OUTPUT);

    ventServo.attach(SERVO_PIN);
    ventServo.write(0);

    sensors.begin();
    initWiFi();

    // MQTT init
    mqttClient.setServer(mqtt_host, mqtt_port);
    mqttClient.setCallback(mqttCallback);

    if (WiFi.status() == WL_CONNECTED) {
        mqttReconnect();
    }

    measureTemperature();
    lastMeasure = millis();
}

void loop() {
    // MQTT keep-alive + reconnect
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) mqttReconnect();
        mqttClient.loop();
    }

    // Periodic measurement
    if (millis() - lastMeasure >= (unsigned long) MEASURE_INTERVAL) {
        measureTemperature();
        lastMeasure = millis();
    }

    delay(50);
}
