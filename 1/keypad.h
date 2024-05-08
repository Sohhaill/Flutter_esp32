#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Keypad.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4
#define I2C_ADDRESS 0x3C

// Define the correct PIN
char correctPin[] = "124"; // Change this to your desired PIN
bool forUpdatePurpose = false;
// Define the buzzer pin
const int buzzerPin = 2; // Replace with your actual buzzer pin

// Define variables
String enteredPin = "";
int wrongAttempts = 0;
// Initialize the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void updatePin();

// Define the keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {14, 27, 26, 25}; // Replace with your actual pin numbers
byte colPins[COLS] = {13, 23, 33, 32}; // Replace with your actual pin numbers
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// Define the relay control pin
const int relayPin = 4; // Replace with your actual relay control pin
const int updatePinKey = '*'; // Key to trigger PIN update

// Define variables

void setup_e() {
  // Initialize the display
  if(!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1.5);
  display.setTextColor(SSD1306_WHITE);

  // Initialize the relay pin as an output
  pinMode(relayPin, OUTPUT);
  // Initially, the lock is locked
  digitalWrite(relayPin, HIGH);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  display.setCursor(0, 0);
  display.println(F("Enter PIN:"));
  display.display();
}

void loop_e() {
  char key = keypad.getKey();
  
  if (key) {
    if (key == '#') { // Check if the entered PIN is complete
      if (enteredPin == correctPin) {
        if (forUpdatePurpose){
          forUpdatePurpose = false;
          updatePin();
        }else{
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println(F("PIN Correct"));
          display.display();
          delay(1000);
          digitalWrite(buzzerPin, HIGH);
          delay(3000); // Adjust the duration as needed
          digitalWrite(buzzerPin, LOW);
          // Turn off the lock (activate relay)
          digitalWrite(relayPin, LOW);
          enteredPin = ""; // Clear the entered PIN
          delay(5000);
          enteredPin = ""; // Clear the entered PIN
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println(F("Enter PIN:"));
          digitalWrite(relayPin, HIGH);
          display.display();
        }
      } else {
         wrongAttempts++;
        
        if (wrongAttempts >= 3) {
          // Activate the buzzer for a few seconds
          digitalWrite(buzzerPin, HIGH);
          delay(5000); // Adjust the duration as needed
          digitalWrite(buzzerPin, LOW);
          // Reset the wrong attempts count
          wrongAttempts = 0;}
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(F("PIN Incorrect"));
        display.print(F("Wrong PIN: "));
        display.print(wrongAttempts);
    display.display();
    digitalWrite(buzzerPin, HIGH);
    delay(1000); // Adjust the duration as needed
    digitalWrite(buzzerPin, LOW);
        digitalWrite(relayPin, HIGH);

        display.display();
        delay(1000);
        enteredPin = ""; // Clear the entered PIN
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(F("Enter PIN:"));
        display.display();
      
      }
    }else if(key == updatePinKey){
      // Check for the update PIN key press
      forUpdatePurpose = true;
      enteredPin = "";
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(F("Enter OLD PIN:"));
      display.display();

  }else {
      enteredPin += key;
      display.setCursor(0, 20);
      display.println(enteredPin);
      display.display();
    }
  }
}

void updatePin() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Enter new PIN:"));
  display.display();
  
  String newPin = "";

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') { // Check if the entered new PIN is complete
        if (newPin.length() >= 4) { // Make sure the new PIN is at least 4 characters long
          newPin.toCharArray(correctPin, sizeof(correctPin));
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println(F("PIN Updated"));
          display.display();
          delay(1000);
          
          return; // Exit the function
        } else {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println(F("PIN is too short"));
          display.display();
          delay(1000);
          newPin = ""; // Clear the entered new PIN
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println(F("Enter new PIN:"));
          display.display();
        }
      } else {
        newPin += key;
        display.setCursor(0, 20);
        display.println(newPin);
        display.display();
      }
    }
  }
}