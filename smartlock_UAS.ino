#include <SPI.h>
#include <MFRC522.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define SS_PIN D4
#define RST_PIN D3
#define LED_G D1 //define green LED pin
#define LED_R D2 //define red LED
#define RELAY D0 //relay pin
#define BUZZER D8 //buzzer pin
#define ACCESS_DELAY 2000
#define DENIED_DELAY 1000

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

// WiFi and MQTT settings
#define SSID "Up"
#define SSID_PASSWORD "asdfghjkl"

const char* mqtt_server = "103.167.112.188";
const int mqtt_port = 1883;
const char* mqtt_user = "/fik:fik";
const char* mqtt_password = "fik123";
const char* queue_name = "smartlock";

String status = "";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void setup() 
{
  Serial.begin(115200);   // Initiate a serial communication
  SPI.begin();          // Initiate SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);
  digitalWrite(RELAY, HIGH);
  Serial.println("Put your card to the reader...");
  Serial.println();

 // Menghubungkan ke jaringan WiFi
  WiFi.begin(SSID, SSID_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Menghubungkan ke RabbitMQ server
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    if (client.connect("ArduinoClient", mqtt_user, mqtt_password)) {
      Serial.println("Connected to RabbitMQ");
    } else {
      Serial.print("Failed to connect to RabbitMQ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void loop() 
{
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  // Show UID on the serial monitor
  Serial.print("UID tag: ");
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message: ");
  content.toUpperCase();

  if (content.substring(1) == "33 91 6B A9") // change here the UID of the card/cards that you want to give access
  {
    Serial.println("Akses berhasil");
    status = "Akses berhasil";
    Serial.println();
    delay(500);
    digitalWrite(RELAY, LOW);
    digitalWrite(LED_G, HIGH);
    tone(BUZZER, 300);
    delay(300);
    delay(ACCESS_DELAY);
    digitalWrite(RELAY, HIGH);
    digitalWrite(LED_G, LOW);
    tone(BUZZER, 1000);
  }
  else
  {
    Serial.println("Akses gagal");
    status = "Akses gagal";
    digitalWrite(LED_R, HIGH);
    tone(BUZZER, 300);
    delay(1000);
    delay(DENIED_DELAY);
    digitalWrite(LED_R, LOW);
    noTone(BUZZER);
}

if (client.connected()) {
    char message[100];
    snprintf(message, sizeof(message), "{\"status\": \"%s\"}", status.c_str());
    client.publish(queue_name, message);
    Serial.println("Data published to RabbitMQ");
  } else {
    Serial.println("Failed to publish data. MQTT client disconnected");
  }

  delay(2000);
}
