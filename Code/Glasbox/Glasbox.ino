#include <Adafruit_AHTX0.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// wifi stuff
String myDeviceId = "DEFINE_DEVICE_ID_HERE";
String wifi_name = "GuestWLANPortal";
String mqtt_ip = "10.10.2.127";

// where to send temperature
String temp_topic = "zuerich/glasbox/oliv/temp";
String humidity_topic = "zuerich/glasbox/oliv/humidity";

// settings topics
String temp_max_topic = "zuerich/glasbox/oliv/settings/tempMax";
String temp_min_topic = "zuerich/glasbox/oliv/settings/tempMin";
String humidity_max_topic = "zuerich/glasbox/oliv/settings/humidityMax";
String humidity_min_topic = "zuerich/glasbox/oliv/settings/humidityMin";

// led pins
int red_led = 27;
int yellow_led = 14;
int green_led = 13;

// temperature limits
float min_temp = 27.0;
float max_temp = 50.0;
float min_humidity = 23.0;
float max_humidity = 50.0;

// make all the objects we need
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
Adafruit_AHTX0 temp_sensor;
Adafruit_SSD1306 screen(128, 64, &Wire, -1);

// Buzzer Pin
const int buzzerPin = 25;

// connect to wifi
void connect_wifi() {
    Serial.println("Trying to connect to wifi...");
    WiFi.disconnect();
    delay(100);
    
    WiFi.begin(wifi_name.c_str());
    
    // wait until connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("Connected to wifi!");
    Serial.print("IP address is: ");
    Serial.println(WiFi.localIP());
}

// handle messages from mqtt
void got_mqtt_message(char* topic, byte* message, unsigned int length) {
    
    String msg = "";
    for(int i = 0; i < length; i++) {
        msg += (char)message[i];
    }
    
    float number = msg.toFloat();
    
    
    if(String(topic) == temp_max_topic) {
        max_temp = number;
        Serial.print("Changed max temp to: ");
        Serial.println(max_temp);
    }
    
    if(String(topic) == temp_min_topic) {
        min_temp = number;
        Serial.print("Changed min temp to: ");
        Serial.println(min_temp);
    }
    
    if(String(topic) == humidity_max_topic) {
        max_humidity = number;
        Serial.print("Changed max humidity to: ");
        Serial.println(max_humidity);
    }
    
    if(String(topic) == humidity_min_topic) {
        min_humidity = number;
        Serial.print("Changed min humidity to: ");
        Serial.println(min_humidity);
    }
}

// try to connect to mqtt
void connect_mqtt() {
    while (!mqtt_client.connected()) {
        Serial.println("Trying to connect to MQTT...");
        
        
        String clientId = myDeviceId + String(random(1000));
        
        if (mqtt_client.connect(clientId.c_str())) {
            Serial.println("Connected to MQTT!");
            
            // subscribe to all settings
            mqtt_client.subscribe(temp_max_topic.c_str());
            mqtt_client.subscribe(temp_min_topic.c_str());
            mqtt_client.subscribe(humidity_max_topic.c_str());
            mqtt_client.subscribe(humidity_min_topic.c_str());
        } else {
            Serial.println("Failed to connect to MQTT :(");
            delay(5000);
        }
    }
}
// note for Red light
#define NOTE_A5  880

// update the leds based on temperature and humidity
void update_leds(float temperature, float humidity) {
    // check if temperature is good
    bool temp_is_good = false;
    if(temperature >= min_temp && temperature <= max_temp) {
        temp_is_good = true;
    }
    
    // check if humidity is good
    bool humidity_is_good = false;
    if(humidity >= min_humidity && humidity <= max_humidity) {
        humidity_is_good = true;
    }
    
    // turn all leds off first
    digitalWrite(red_led, LOW);
    digitalWrite(yellow_led, LOW);
    digitalWrite(green_led, LOW);
    
    // if both are bad, red light
    if(!temp_is_good && !humidity_is_good) {
        digitalWrite(red_led, HIGH);
        tone(buzzerPin, NOTE_A5, 4);
    }
    
    // if one is bad, yellow light
    if((temp_is_good && !humidity_is_good) || (!temp_is_good && humidity_is_good)) {
        digitalWrite(yellow_led, HIGH);
    }
    
    // if both good, green light
    if(temp_is_good && humidity_is_good) {
        digitalWrite(green_led, HIGH);
    }
}

void setup() {
    // start serial so we can debug
    Serial.begin(115200);
    Serial.println("Starting...");
    
    // setup wifi
    WiFi.mode(WIFI_STA);
    Wire.setClock(400000);
    
    // setup led pins
    pinMode(red_led, OUTPUT);
    pinMode(yellow_led, OUTPUT);
    pinMode(green_led, OUTPUT);
    
    // turn off all leds
    digitalWrite(red_led, LOW);
    digitalWrite(yellow_led, LOW);
    digitalWrite(green_led, LOW);
    
    // start temperature sensor
    if (!temp_sensor.begin()) {
        Serial.println("Couldn't start temperature sensor!");
        while (1) delay(10);
    }
    
    // start screen
    if (!screen.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Couldn't start screen!");
        while (1) delay(10);
    }
    
    // setup screen
    screen.clearDisplay();
    screen.setTextSize(2);
    screen.setTextColor(WHITE);
    
    // connect to wifi
    connect_wifi();
    
    // setup mqtt
    mqtt_client.setServer(mqtt_ip.c_str(), 1883);
    mqtt_client.setCallback(got_mqtt_message);
    mqtt_client.setBufferSize(256);
    mqtt_client.setSocketTimeout(10);
}

void loop() {
    // check wifi
    if(WiFi.status() != WL_CONNECTED) {
        connect_wifi();
    }
    
    // check mqtt
    if(!mqtt_client.connected()) {
        connect_mqtt();
    }
    
    // let mqtt do its thing
    mqtt_client.loop();
    
    // get temperature and humidity
    sensors_event_t humidity_reading, temp_reading;
    temp_sensor.getEvent(&humidity_reading, &temp_reading);
    
    float current_temp = temp_reading.temperature;
    float current_humidity = humidity_reading.relative_humidity;
    
    // update the leds
    update_leds(current_temp, current_humidity);
    
    // update the screen
    screen.clearDisplay();
    screen.setCursor(0, 0);
    screen.print(current_temp);
    screen.println(" C");
    screen.println("--------");
    screen.print(current_humidity);
    screen.println("% rH");
    screen.display();
    
    // send to mqtt
    String temp_string = String(current_temp, 2);
    String humidity_string = String(current_humidity, 2);
    
    mqtt_client.publish(temp_topic.c_str(), temp_string.c_str());
    mqtt_client.publish(humidity_topic.c_str(), humidity_string.c_str());
    
    // wait a bit
    delay(200);
}