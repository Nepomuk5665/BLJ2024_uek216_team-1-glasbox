#include <Adafruit_AHTX0.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char* device_id = "DEFINE_DEVICE_ID_HERE";
const char* ssid = "GuestWLANPortal";
const char* mqtt_server = "10.10.2.127";
const char* topic1 = "zuerich/glasbox/oliv/temp";
const char* topic2 = "zuerich/glasbox/oliv/humidity";
const int RED_LED_PIN = 27;
const int YELLOW_LED_PIN = 14;
const int GREEN_LED_PIN = 13;


const unsigned long WIFI_TIMEOUT = 10000; 
const unsigned long MQTT_RETRY_INTERVAL = 5000; 
unsigned long lastMqttRetry = 0;

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_AHTX0 aht;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

bool setup_wifi() {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    unsigned long startAttemptTime = millis();
    WiFi.begin(ssid);
    
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startAttemptTime < WIFI_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("done!");
        return true;
    } else {
        Serial.println("failed!");
        return false;
    }
}

bool tryConnect() {
    if (client.connect(device_id)) {
        Serial.println("MQTT connected!");
        client.subscribe(topic1);
        client.subscribe(topic2);
        return true;
    }
    return false;
}

void manageMQTTConnection() {
    if (!client.connected() && 
        (millis() - lastMqttRetry >= MQTT_RETRY_INTERVAL)) {
        
        Serial.print("Attempting MQTT connection...");
        
        if (tryConnect()) {
            lastMqttRetry = 0;  
        } else {
            lastMqttRetry = millis();  
            Serial.println("failed, will retry later");
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, topic2) == 0) {
        char message[length + 1];
        memcpy(message, payload, length);
        message[length] = '\0';
        Serial.print("Received Message: ");
        Serial.println(message);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("AHT10 + SSD1306 Demo");

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);

    if (!aht.begin()) {
        Serial.println("AHT10 allocation failed");
        while (1) delay(10);
    }

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        while (1) delay(1);
    }

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    Serial.println("Setup complete");

    if (!setup_wifi()) {
        // wlan problem
        return;
    }

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop() {
    
    manageMQTTConnection();
    
    if (client.connected()) {
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);

        
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print(temp.temperature);
        display.println(" degC");
        display.println("----------");
        display.print(humidity.relative_humidity);
        display.println("% rH");
        display.display();

        
        char tempBuffer[10];
        sprintf(tempBuffer, "%.2f", temp.temperature);
        client.publish(topic1, tempBuffer);

        char humidityBuffer[10];
        sprintf(humidityBuffer, "%.2f", humidity.relative_humidity);
        client.publish(topic2, humidityBuffer);

        client.loop();
    }
    
    
    digitalWrite(RED_LED_PIN, HIGH);
    delay(500);
    digitalWrite(RED_LED_PIN, LOW);
    delay(1500); 
}