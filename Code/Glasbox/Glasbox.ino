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

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_AHTX0 aht;
Adafruit_SSD1306 display(128, 64, &Wire, -1);


void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("done!");
}


void reconnect() {
  Serial.print("Attempting MQTT connection...");
  while (!client.connected()) {
    if (client.connect(device_id)) {
      Serial.println("done!");
      client.subscribe(topic1);
      client.subscribe(topic2);
    } else {
      delay(500);
      Serial.print(".");
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

  setup_aht();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);




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
  sprintf(tempBuffer, "%f", temp.temperature);
  client.publish(topic1, tempBuffer);


  char tempBuffer[10];
  sprintf(tempBuffer, "%f", humidity.relative_humidity);
  client.publish(topic2, tempBuffer);


  delay(500);
  client.loop();
}
