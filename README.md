# BudgetBasket
HOW TO USE:
- Modify the wifi SSID and password in BudgetBasket_ESP_V1 to match your network settings
- For webserver IP read serial monitor for ESP. Restart may be necessary.

OVERVEIW:
This project integrates RFID technology with Arduino and ESP8266 modules to create a seamless checkout experience in retail environments. Customers can scan items, remove items, and view item details including individual prices and total costs. The system supports setting and tracking budgets, viewing receipts online, and managing transactions through a user-friendly web interface.

COMPONENTS:
- Arduino ATmega2560: Manages RFID scanning, user interactions, and display updates.
- ESP8266 NodeMCU: Handles Wi-Fi connectivity and serves a web server for displaying receipts and managing budgets.
- MFRC522 RFID Reader: Used for scanning RFID tags on items.
- I2C LCD Display: Shows information about scanned items, total price, and budget.
- Push Button: Allows user to interact with the system, for instance, to confirm the addition or removal of items.
- Passive Buzzer: Provides auditory feedback during operations.

FILES:
BudgetBasket_ATMEGA_V1.ino
BudgetBasket_ESP_V1.ino

SETUP:

	1. Hardware Assembly
	- Connect the MFRC522 to the Arduino using SPI pins.
	- Attach the I2C LCD Display to the corresponding I2C pins on the Arduino.
	- Integrate the ESP8266 by connecting it through the designated serial pins on the Arduino.
	- Ensure all components are powered appropriately, adhering to their voltage requirements.
 
	2. Software Configuration
	- Upload the BudgetBasket_ATMEGA_V1.ino to your Arduino board.
	- Upload the BudgetBasket_ESP_V1.ino to the ESP8266 module.
	- Modify the Wi-Fi SSID and password in the ESP8266 code to match your local network settings.

	3. Web Server Setup
	- The ESP8266 hosts a web server accessible within the same Wi-Fi network.
	- Customers can access the web interface to view and manage their receipts and budget.

USAGE:
- Scanning Items: Present the items RFID tag near the MFRC522 reader to add it to the cart.
- Removing Items: Use the push button to remove the last scanned item.
- Viewing Receipts: Access the provided URL on any device connected to the same Wi-Fi network to see detailed receipts and budget information.

PIN SETUP:

	* RFID MFRC522 reader *
	SDA   - PIN  9
	SCK   - PIN 52 (SCK)
 	MOSI  - PIN 51 (MOSI)
  	MISO  - PIN 50 (MISO)
	RST   - PIN  8

     * Multicolor RGB LED *
	RED   - PIN 12 (PWM)
 	GREEN - PIN 11 (PWM)
  	BLUE  - PIN 10 (PWM)

    * I2C LCD 16x2 display*
     SDA   - PIN 20 (SDA)
      SCL   - PIN 21 (SCL)

       * ESP8266 nodeMCU module *
       RX    - PIN 18 (TX1)
       TX    - PIN 19 (RX1)

       * Passive Buzzer *
       OUTPUT - PIN 49 (PWM)

       * Push Button *
       INPUT - PIN 48

FUTURE ENHANCEMENTS
- Implement a more dynamic web interface with the ability to handle multiple sessions simultaneously.
- Expand the database of items to increase the versatility of the system.
- Add security features to protect transaction and customer data.

For further assistance or to contribute to the project, please contact the repository maintainers or submit an issue on the project’s GitHub page.
