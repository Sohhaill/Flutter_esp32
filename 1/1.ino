#include <Adafruit_Fingerprint.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "keypad.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include "esp_http_server.h"

#define HTTP_ENDPOINT "http://172.20.10.5:4000/endpoint"
#define WIFI_SSID "Raja.iphone"
#define WIFI_PASSWORD "aaa1234567"
#define SERVER_PORT 80
#define RELAY_PIN 4
#define buzzerPin 2


HTTPClient http;
int noOfFailedAttempts = 0;
bool isFireTime = false;
WiFiServer server(SERVER_PORT);
HardwareSerial serialPort(2);
#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)

SoftwareSerial mySerial(16, 17);
#else

#define mySerial Serial2 

#endif

bool fConfig = false;

uint8_t id;
uint8_t userID;
uint8_t rBuff;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serialPort);
uint8_t getFingerprintID();

uint8_t readnumber(void) {

  uint8_t num = 0;

  while (num == 0) {

    while (! Serial.available());

    num = Serial.parseInt();

  }
  return num;
}

void setup(){
  finger.begin(57600);
  Serial.begin(9600);
  connectToWiFi();
  server.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1.5);
  display.setTextColor(SSD1306_WHITE);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  

  while (!Serial);

  delay(100);

  Serial.println("\n\nAdafruit finger detect test");
  delay(5);

  if (finger.verifyPassword()) {

    Serial.println("Found fingerprint sensor!");

  } else {

    Serial.println("Did not find fingerprint sensor :(");

    while (1) { delay(1); }

  }

  Serial.println(F("Reading sensor parameters"));

  finger.getParameters();

  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);

  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);


  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);

  Serial.print(F("Security level: ")); Serial.println(finger.security_level);

  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);

  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);

  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {

    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");

  }

  else {

    Serial.println("Waiting for valid finger...");

      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");

    }
}

void loop(){
  if (noOfFailedAttempts == 2) {
    if (!isFireTime){
      isFireTime = true;
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Max failed attempts");
      delay(1000);
      display.setCursor(0, 20);
      display.println(F("Shift To Keypad"));
      display.display();
      digitalWrite(buzzerPin, HIGH);
      delay(4000);
 // Adjust the duration as needed
      digitalWrite(buzzerPin, LOW);
      display.display();
      delay(1000);
      setup_e();
      //setup_b();
    }
    loop_e();
    //loop_b();
  }
  else{

   if (Serial.available()) {         

    rBuff = Serial.read();

    if ( rBuff == 'C' ){

    fConfig = true;
    }

     else if ( rBuff == 'D' ){

          Serial.println("Please type in the ID # (from 1 to 10) you want to delete..."); 

          id = readnumber();

          deleteFingerprint(id);

        }

  }  

  if (fConfig)

  {

      Serial.println("Ready to enroll a fingerprint!");

      Serial.println("Please type in the ID # (from 1 to 10) you want to save this finger as...");

      id = readnumber();

      if (id == 10) 

      {  

        Serial.print("exit enroll#");

        fConfig = false;

      }

      else{

        Serial.print("Enrolling ID #");

        Serial.println(id);

        while (!getFingerprintEnroll() );

      }

    }

   else{

    getFingerprintID();
    handleWiFiCommands();

   }

   delay(50);
   
    
  }
  
  

}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Removed the line that printed dots during the connection process
  }
  Serial.println("WiFi connected!"); // Added this line
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}


void handleWiFiCommands() {
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char command = client.read();
        if (command == 'E') {
          Serial.println("Ready to enroll a fingerprint!");
          sendHTTPPost("Please type in the ID # (from 1 to 10) you want to save this finger as...");
          id = readnumber();
          if (id == 10) {  
            Serial.println("Exit enroll");
            fConfig = false;
          } else {
            Serial.print("Enrolling ID #");
            Serial.println(id);
            while (!getFingerprintEnroll());
          }
        } 
        else if (command == 'B') { // Close relay command
          digitalWrite(buzzerPin, HIGH);
          delay(3000); // Adjust the duration as needed
          digitalWrite(buzzerPin, LOW);
          
        }
        
         else if (command == 'O') { // Open relay command
          digitalWrite(RELAY_PIN, HIGH);
          sendHTTPPost("Lock is open");
          client.println("Relay opened");
          
        } 
        else if (command == 'L') { // Close relay command
          digitalWrite(RELAY_PIN, LOW);
          sendHTTPPost("Lock is close");
          client.println("Relay closed");
          
        }
        

        
      }
    }
    client.stop();
  }
}




uint8_t deleteFingerprint(uint8_t id) {

  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {

    Serial.println("Deleted!");

  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {

    Serial.println("Communication error");

  } else if (p == FINGERPRINT_BADLOCATION) {

    Serial.println("Could not delete in that location");

  } else if (p == FINGERPRINT_FLASHERR) {

    Serial.println("Error writing to flash");

  } else {

    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);

  }

  return p;

}

void sendHTTPPost(String message) {
  
  http.begin(HTTP_ENDPOINT);
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST(message);

  if (httpCode > 0) {
    Serial.printf("[HTTP] POST request sent, status code: %d\n", httpCode);
  } else {
    Serial.printf("[HTTP] POST request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}


uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();

  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Place Finger");
      display.display();
      delay(2000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Comm Error");
      display.display();
      delay(2000);
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Imaging Error");
      display.display();
      delay(2000);
      return p;
    default:
      Serial.println("Unknown error");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Unknown Error");
      display.display();
      delay(2000);
      return p;
  }

  p = finger.image2Tz();

  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Image Messy");
      display.display();
      delay(2000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Comm Error");
      display.display();
      delay(2000);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Feature Fail");
      display.display();
      delay(2000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Invalid Image");
      display.display();
      delay(2000);
      return p;
    default:
      Serial.println("Unknown error");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Unknown Error");
      display.display();
      delay(2000);
      return p;
  }

  p = finger.fingerSearch();

  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    noOfFailedAttempts=0;
    Serial.print("Attempt: ");
    Serial.println(noOfFailedAttempts);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.println("Correct Password");
    display.display();
    digitalWrite(RELAY_PIN, LOW);
  
    delay(5000);
        digitalWrite(RELAY_PIN, HIGH);
        display.display();
       
     // Turn on the relay
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.println("Comm Error");
    display.display();
    digitalWrite(RELAY_PIN, HIGH); // Turn off the relay
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    noOfFailedAttempts++;
    Serial.println("Did not find a match");
    Serial.print("Attempt: ");
    Serial.println(noOfFailedAttempts);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.print(F("Wrong PIN: "));
    display.print(noOfFailedAttempts);
    display.display();
    digitalWrite(buzzerPin, HIGH);
    delay(1000); // Adjust the duration as needed
    digitalWrite(buzzerPin, LOW);
    digitalWrite(RELAY_PIN, HIGH); // Turn off the relay
    return p;
  } else {
    Serial.println("Unknown error");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.println("Unknown Error");
    display.display();
    digitalWrite(RELAY_PIN, HIGH); // Turn off the relay
    return p;
  }

  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);
  return finger.fingerID;
}

int getFingerprintIDez() {

  uint8_t p = finger.getImage();

  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();

  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();

  if (p != FINGERPRINT_OK)  return -1;

  Serial.print("Found ID #"); Serial.print(finger.fingerID);

  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;

}

uint8_t getFingerprintEnroll() {



  int p = -1;

  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);

  while (p != FINGERPRINT_OK) {

    p = finger.getImage();

    switch (p) {

    case FINGERPRINT_OK:

      Serial.println("Image taken");

      break;

    case FINGERPRINT_NOFINGER:

      Serial.println(".");

      break;

    case FINGERPRINT_PACKETRECIEVEERR:

      Serial.println("Communication error");

      break;

    case FINGERPRINT_IMAGEFAIL:

      Serial.println("Imaging error");

      break;

    default:

      Serial.println("Unknown error");

      break;

    }

  }

  p = finger.image2Tz(1);

  switch (p) {

    case FINGERPRINT_OK:

      Serial.println("Image converted");

      break;

    case FINGERPRINT_IMAGEMESS:

      Serial.println("Image too messy");

      return p;

    case FINGERPRINT_PACKETRECIEVEERR:

      Serial.println("Communication error");

      return p;

    case FINGERPRINT_FEATUREFAIL:

      Serial.println("Could not find fingerprint features");

      return p;

    case FINGERPRINT_INVALIDIMAGE:

      Serial.println("Could not find fingerprint features");

      return p;

    default:

      Serial.println("Unknown error");

      return p;

  }



  Serial.println("Remove finger");

  delay(2000);

  p = 0;

  while (p != FINGERPRINT_NOFINGER) {

    p = finger.getImage();

  }

  Serial.print("ID "); Serial.println(id);

  p = -1;

  Serial.println("Place same finger again");

  while (p != FINGERPRINT_OK) {

    p = finger.getImage();

    switch (p) {

    case FINGERPRINT_OK:

      Serial.println("Image taken");

      break;

    case FINGERPRINT_NOFINGER:

      Serial.print(".");

      break;

    case FINGERPRINT_PACKETRECIEVEERR:

      Serial.println("Communication error");

      break;

    case FINGERPRINT_IMAGEFAIL:

      Serial.println("Imaging error");

      break;

    default:

      Serial.println("Unknown error");

      break;

    }

  }


  p = finger.image2Tz(2);

  switch (p) {

    case FINGERPRINT_OK:

      Serial.println("Image converted");

      break;

    case FINGERPRINT_IMAGEMESS:

      Serial.println("Image too messy");

      return p;

    case FINGERPRINT_PACKETRECIEVEERR:

      Serial.println("Communication error");

      return p;

    case FINGERPRINT_FEATUREFAIL:

      Serial.println("Could not find fingerprint features");

      return p;

    case FINGERPRINT_INVALIDIMAGE:

      Serial.println("Could not find fingerprint features");

      return p;

    default:

      Serial.println("Unknown error");

      return p;

  }



  Serial.print("Creating model for #");  Serial.println(id);



  p = finger.createModel();

  if (p == FINGERPRINT_OK) {

    Serial.println("Prints matched!");

  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {

    Serial.println("Communication error");

    return p;

  } else if (p == FINGERPRINT_ENROLLMISMATCH) {

    Serial.println("Fingerprints did not match");

    return p;

  } else {

    Serial.println("Unknown error");

    return p;

  }

  Serial.print("ID "); Serial.println(id);

  p = finger.storeModel(id);

  if (p == FINGERPRINT_OK) {

    Serial.println("Stored!");

  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {

    Serial.println("Communication error");

    return p;

  } else if (p == FINGERPRINT_BADLOCATION) {

    Serial.println("Could not store in that location");

    return p;

  } else if (p == FINGERPRINT_FLASHERR) {

    Serial.println("Error writing to flash");

    return p;

  } else {

    Serial.println("Unknown error");

    return p;

  }
  return true;

}