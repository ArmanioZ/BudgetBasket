/**
 * @file BudgetBasket_ATMEGA_V1.ino
 * 
 * Purpose:
 * This program utilizes an Arduino ATmega2560 to create a retail item scanning system, leveraging RFID technology for item detection and I2C for display communications. The system aims to enhance the shopping experience by providing immediate feedback on product information and pricing, managing shopping lists, and handling budgets interactively.
 * 
 * Description:
 * - **RFID Interaction**: Utilizes the MFRC522 RFID reader for scanning items, where each item is associated with a unique tag.
 * - **Display Interface**: Employs a 16x2 I2C LCD display to show item details, total price, and budget information in real-time.
 * - **Communication**: Establishes a serial communication link with an ESP8266 module to send and receive data relevant to the shopping session, including budget constraints and item lists.
 * - **User Input**: Includes a push button to toggle between item addition and removal modes, enhancing user control.
 * - **Auditory Feedback**: Integrates a passive buzzer to provide sound notifications during item scanning and error indications.
 * - **Visual Feedback**: Utilizes RGB LEDs to give visual cues about the system status, such as low budget warnings or errors.
 * 
 * Implementation:
 * - The setup function initializes all connected hardware components, sets pin modes, and prepares the system for operation.
 * - The main loop handles incoming serial data from the ESP module, processes RFID scans, updates the LCD display, and manages user inputs through the button.
 * - Additional functions are defined for adding and removing items from the shopping list, updating the total price, sending data to the ESP, and managing LED and buzzer outputs based on system events.
 * 
 * Note:
 * The code is designed for expandability and can be easily adapted to include more items, enhanced user interfaces, or additional hardware components.
 *
 * Authors: Vahe Petrosian
 *          Sverre Riis Rasmussen
 *          Jamie Tan
 *          Anna Christensen Mathiassen
 *          Anna Haarsaker Olaussen
 *
 * Date:    May 9, 2024
 */


// Serial Peripheral Interface communication protocol
#include <SPI.h>
// Library for RFID reader
#include <MFRC522.h>
// Library for LCD display
#include <LiquidCrystal_I2C.h>

#define RST_PIN         8
#define SS_PIN          9
#define ESP_SERIAL      Serial1  // Serial1 on pins 18 (TX) and 19 (RX)

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Item Names and Prices
const String itemNames[] = {"Milk", "Egg", "Butter", "Chocolate"};
const float itemPrices[] = {7.0, 25.0, 18.0, 20.0};

// RFID tag ID's
const byte itemUIDs[][4] = {{0x4A, 0x5C, 0xE2, 0x17}, {0x3A, 0x64, 0xD2, 0x12}, {0x92, 0x9A, 0x84, 0x51}, {0x60, 0x62, 0x9D, 0x55}};

// Variables for handling data
const int numItems = 4;
int itemCounts[numItems] = {0}; 
float totalPrice = 0.0;
String incomingData;
float budget;
bool inRemoveMode = false;
unsigned long buttonPressedTime = 0;

// Hardware setup
int buzzerPin = 49;
int redLED = 12;
int greenLED = 11;
int blueLED = 10;
int buttonPin = 48;

// Constants used for Dart Vader melody
const int c = 261;
const int d = 294;
const int e = 329;
const int f = 349;
const int g = 391;
const int gS = 415;
const int a = 440;
const int aS = 455;
const int b = 466;
const int cH = 523;
const int cSH = 554;
const int dH = 587;
const int dSH = 622;
const int eH = 659;
const int fH = 698;
const int fSH = 740;
const int gH = 784;
const int gSH = 830;
const int aH = 880;

int counter = 0;

// Hardware setups
void setup() {
    // Buzzer, led and button
    pinMode(buzzerPin, OUTPUT);
    pinMode(redLED, OUTPUT);
    pinMode(greenLED, OUTPUT);
    pinMode(blueLED, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    setLEDColor(0, 255, 0);

    // Serial connections RFID and LCD
    ESP_SERIAL.begin(9600);
    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
    lcd.init();
    lcd.backlight();
    lcd.begin(16, 2); // set up the LCD's number of columns and rows:
    lcd.print("Ready to scan...");
}

// Loop for reading data from ESP and setting mode (standard, low budget and no budget)
void loop() {
    checkButton();
    if (ESP_SERIAL.available() > 0) {
        incomingData = ESP_SERIAL.readStringUntil('\n'); // Read the string up to newline
        incomingData.trim(); // Remove any whitespace or newline characters
        if (incomingData.startsWith("Budget:")) {
            budget = incomingData.substring(7).toFloat(); // Parse budget value
            lcd.setCursor(0, 1);
            lcd.print("Budget: ");
            lcd.print(String(budget, 2) + "kr.");
        } else if (incomingData == "LowBudget") {
            setLEDColor(100, 20, 0);  // Set LED to yellowish color for low budget warning
        } else if (incomingData == "NoBudget") {
            firstSection();
            setLEDColor(255, 0, 0);  // Set LED to red color for no budget warning
        }
    }

    // Handling RFID tags and LCD display
    // Check if card is present and read the card
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        lcd.clear();
        // Handle the UID data
        byte *id = mfrc522.uid.uidByte;
        processCard(id);
        delay(2000);  // Delay to allow reading display before clearing
        showReadyScreen();
    }
}

// Function for handling items (tags)
void processCard(byte *id) {
    int index = identifyItem(id);
    if (index != -1) {
        makeScanSound();  // Sound for a successful scan
        if (inRemoveMode) {
            removeItem(index);
        } else {
            addItem(index);
        }
    } else {
        makeErrorSound();  // Error sound for unrecognized card
        setLEDColor(255, 0, 0); // LED goes red when error
        lcd.setCursor(0, 0);
        lcd.print("Unknown item");
    }
}

// Function for adding items to the list
void addItem(int index) {
    itemCounts[index]++;
    updateTotal();
    displayItem(itemNames[index], String(itemPrices[index]) + " kr");
    sendStatusToESP();
}

// Function for removing items from the list
void removeItem(int index) {
    if (itemCounts[index] > 0) {
        itemCounts[index]--;
        updateTotal();
        displayItem(itemNames[index], "Removed");
        sendStatusToESP();
    } else {
        setLEDColor(255, 0, 0); // LED goes red if trying to remove item that is not on the list yet.
        lcd.print("Item not found");
        makeErrorSound();  // Error sound if the item to remove was not found
    }
    inRemoveMode = false;
}

// Updating total price when adding or removing items
void updateTotal() {
    totalPrice = 0.0;
    for (int i = 0; i < numItems; i++) {
        totalPrice += itemPrices[i] * itemCounts[i];
    }
}

// Send updated price to the ESP
void sendStatusToESP() {
    String data = "Update:";  // Prefix to identify the message purpose
    for (int i = 0; i < numItems; i++) {
        if (itemCounts[i] > 0) {
            data += itemNames[i] + "," + String(itemCounts[i]) + ";";
        }
    }
    data += "Total," + String(totalPrice);
    ESP_SERIAL.println(data);
}

// Display items on the LCD
void displayItem(const String& item, const String& action) {
    lcd.setCursor(0, 0);
    lcd.print(item + " " + action);
    lcd.setCursor(0, 1);
    lcd.print("Total: " + String(totalPrice) + " kr.");
}

// Ready to scan mode displayed on the LCD
void showReadyScreen() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ready to scan...");
    setLEDColor(0, 255, 0);
}

// Identifying items
int identifyItem(byte *id) {
    for (int i = 0; i < numItems; i++) {
        bool match = true;
        for (int j = 0; j < 4; j++) {
            if (id[j] != itemUIDs[i][j]) {
                match = false;
                break;
            }
        }
        if (match) return i;
    }
    return -1; // Not found
}

// Write LED colors to the diode
void setLEDColor(int red, int green, int blue) {
    analogWrite(redLED, red);
    analogWrite(greenLED, green);
    analogWrite(blueLED, blue);
}

// Scanning sound when scanning RFID tags
void makeScanSound() {
    tone(buzzerPin, 2000, 50); // Beep for 50 ms
}

// Error sound buzzer
void makeErrorSound() {
    tone(buzzerPin, 100, 350); // Error sound for 350 ms
}

// Button configuration
void checkButton() {
    if (digitalRead(buttonPin) == LOW) {
        if (!inRemoveMode) {  // Button pressed, not already in remove mode
            inRemoveMode = true;
            buttonPressedTime = millis();
            lcd.clear();
            lcd.print("Remove item...");
            setLEDColor(0, 0, 255); // Set LED to yellow
        }
    }
    
    // Stay in remove mode for 5 seconds and go back to ready mode if nothing is scanned
    if (inRemoveMode) {
        if (millis() - buttonPressedTime > 5000) { 
            inRemoveMode = false;
            showReadyScreen();
        }
    }
}

// LED sequence for Darth Vader melody
void beep(int note, int duration) {
  tone(buzzerPin, note, duration);

  if (counter % 3 == 0) {
    digitalWrite(redLED, HIGH);
    delay(duration);
    digitalWrite(redLED, LOW);
  } else if (counter % 3 == 1) {
    digitalWrite(greenLED, HIGH);
    delay(duration);
    digitalWrite(greenLED, LOW);
  } else {
    digitalWrite(blueLED, HIGH);
    delay(duration);
    digitalWrite(blueLED, LOW);
  }

  noTone(buzzerPin);
  delay(50);
  counter++;
}

// First section of Dart Vader melody
void firstSection() {
  beep(a, 500);
  beep(a, 500);    
  beep(a, 500);
  beep(f, 350);
  beep(cH, 150);  
  beep(a, 500);
  beep(f, 350);
  beep(cH, 150);
  beep(a, 650);

  delay(500);

  beep(eH, 500);
  beep(eH, 500);
  beep(eH, 500);  
  beep(fH, 350);
  beep(cH, 150);
  beep(gS, 500);
  beep(f, 350);
  beep(cH, 150);
  beep(a, 650);

  delay(500);
}

// Second section of Darth Vader melody (not used)
void secondSection() {
  beep(aH, 500);
  beep(a, 300);
  beep(a, 150);
  beep(aH, 500);
  beep(gSH, 325);
  beep(gH, 175);
  beep(fSH, 125);
  beep(fH, 125);    
  beep(fSH, 250);

  delay(325);

  beep(aS, 250);
  beep(dSH, 500);
  beep(dH, 325);  
  beep(cSH, 175);  
  beep(cH, 125);  
  beep(b, 125);  
  beep(cH, 250);  

  delay(350);
}

