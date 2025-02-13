#include <TridentTD_LineNotify.h>
#include <Servo.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define LINE_TOKEN "yttj7enQ6pPV5tloZYzxrqWc2HOU45lfS8sf7IZ4Qzb"

unsigned long lastNotificationTime = 0; 
const unsigned long notificationInterval = 5000; //120000
bool initialNotificationSent = false;

const int pirPin = D1;         // PIR motion sensor pin
const int ledPin = D3;         // LED pin
const int servoPin = D6;       // Servo motor pin
const int servoPin2 = D4;
const int trigPin = D7;        // Ultrasonic sensor trigger pin
const int echoPin = D8;        // Ultrasonic sensor echo pin
const int buzzerPin = D0;      // Buzzer pin
const char* publish_topic_distance = "@msg/proj/distance";
const char* publish_topic_pir_count = "@msg/proj/pir_count";
const char* ssid = "MEEN_2.4G";
const char* password = "20032003m";
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = "ea7a512e-683d-479d-80ea-55dde8f36148"; 
const char* mqtt_username = "fNVz5F1CMMLxXxTGYKFTrNoXtKEHYQua";
const char* mqtt_password = "HreLYfJAQBEpZPCqhLbxtaqtZFVUnttr";


WiFiClient espClient;
PubSubClient client(espClient);


long lastMsg = 0;
int pirCount = 0;               // Variable to count PIR sensor detections
char msg[50]; 
char msg_fb[100];

Servo servo;                    // Create a servo object
Servo servo2;
int mapDistanceToPercentage(int distance) {
  if (distance >= 29) {
    return 0; 
  } else if (distance <= 10) {
    return 100; 
  } else {
    // Map distances between 10 and 30 to a percentage between 100 and 0
    return map(distance, 10, 30, 100, 0);
  }
}

void sendLineNotification(String message) {
  unsigned long currentTime = millis();
  if (!initialNotificationSent) {
    
    LINE.notify(message); // Send the initial Line notification
    initialNotificationSent = true; // Set the flag to true
    lastNotificationTime = currentTime; // Update the last notification time
  }
  else if (currentTime - lastNotificationTime >= notificationInterval) {
    LINE.notify(message);
    lastNotificationTime = currentTime;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection‚Ä¶");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      Serial.println("connected");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  servo2.attach(servoPin2);
  servo.attach(servoPin);
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, mqtt_port);
  LINE.setToken(LINE_TOKEN);
  LINE.notify("Bin start!!");
}

void loop() {
  // Check Wifi connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Check PIR motion sensor
  if (digitalRead(pirPin) == HIGH) {
    Serial.println("Motion detected!");
    digitalWrite(ledPin, HIGH);    // Turn on LED
    servo.write(0);                // Rotate servo to 180 degrees
    servo2.write(180);
    delay(8000);                   // Wait for the servo to reach position
    pirCount++;                    // Increment PIR count
  } else {
    Serial.println("No motion detected.");
    digitalWrite(ledPin, LOW);     // Turn off LED
    servo.write(180);              // Rotate servo to 0 degrees
    servo2.write(0);
  }

  // Check ultrasonic sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  int percentage = mapDistanceToPercentage(distance);
  

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    snprintf(msg, 50, "%d", percentage);
    client.publish(publish_topic_distance, msg);
    Serial.println(distance);

    snprintf(msg, 50, "%d", pirCount);
    client.publish(publish_topic_pir_count, msg);
    Serial.println(pirCount);

    // Send Line notification
    String notificationMessage = "Capacity: " + String(percentage) + "%, Motion_count: " + String(pirCount);
    String data = "{\"capacity\":"+String(percentage)+",\"motion_count\":"+String(pirCount)+"}";
    String payload = "{\"data\":" + data + "}";
    Serial.println(payload);
    
    payload.toCharArray(msg_fb, (payload.length() + 1));
    client.publish("@shadow/data/update", msg_fb);
    if (distance < 10) {
      Serial.println("Object detected within 10 cm!");
      tone(buzzerPin, 1000);        // Turn on buzzer
      delay(1000);
      noTone(buzzerPin);            // Turn off buzzer
      
    }
    //pirCount = 0;  // Reset PIR count
    sendLineNotification(notificationMessage);
    delay(100);
  }
}

