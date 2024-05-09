/**
 * @file BudgetBasket_ESP_V1.ino
 * 
 * Purpose:
 * This program leverages the ESP8266 module to serve as a Wi-Fi-enabled web server for a retail RFID scanning system. It connects to the local network, syncs time via NTP for accurate receipt timestamping, and communicates with an Arduino system to receive and display transaction data dynamically on a web interface.
 * 
 * Description:
 * - **Network Configuration**: Establishes a connection to a specified Wi-Fi network, necessary for all other network-related functionalities.
 * - **Web Server**: Uses the ESP8266WebServer library to create a simple HTTP server that responds to various endpoints, managing budget inputs and displaying the latest transaction data.
 * - **Data Handling**: Receives serialized string data from the Arduino, parses it, and updates the web page in real-time. This includes detailed transaction records and budget tracking.
 * - **User Interaction**: Provides a web interface for setting a shopping budget, viewing itemized receipts, and monitoring the remaining budget. 
 * 
 * Implementation:
 * - The `setup` function configures the Wi-Fi settings, initializes the web server, and sets up routes for handling HTTP requests.
 * - The `loop` function continuously checks for new data from the Arduino via serial communication and handles client requests to serve fresh data.
 * - Additional functions include parsing transaction data, calculating budget adjustments, formatting data for web display, and timestamping for receipts.
 * 
 * Note:
 * This part of the system focuses on the network and server-side management, ensuring data from the shopping transactions is accurately reflected on the web interface and providing a responsive user experience.
 *
 * Authors: Vahe Petrosian
 *          Sverre Riis Rasmussen
 *          Jamie Tan
 *          Anna Christensen Mathiassen
 *          Anna Haarsaker Olaussen
 *
 * Date:    May 9, 2024
 */


// Library for using ESP as a Wifi-module
#include <ESP8266WiFi.h>
// Library for creating a simple webserver
#include <ESP8266WebServer.h>
// Time library used for timestamp for the receipt
#include <time.h>

// Wifi SSID and password for connecting the ESP to Wifi
// MUST BE CHANGES TO CURRENTLY USED WIFI
// ****************************************************
const char* ssid = "BudgetBasket";                  //*
const char* password = "budget101";                 //*
// ****************************************************

// Starting a webserver
ESP8266WebServer server(80);

float totalBudget = 0.0;
float remainingBudget = 0.0;
float budget = 0.0;
String currentData = "";  // Holds the latest data string from the ATmega

// Setup for ESP wifi and webserver
void setup() {
    Serial.begin(9600);
    WiFi.mode(WIFI_STA); // Set WiFi to station mode
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");

    // Initialize NTP for timestamping.
    configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // 2 is for Denmark in summer time

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) { // 30 seconds timeout
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP()); // Print the IP address to serial monitor
    } else {
        Serial.println("\nFailed to connect to WiFi. Please check your credentials.");
    }

    server.on("/", []() {
        server.send(200, "text/html", buildWebpage());
    });

    server.on("/submit_budget", []() {
        if (server.hasArg("budget")) {
            totalBudget = server.arg("budget").toFloat();
            remainingBudget = totalBudget;  // Reset remaining budget on new budget input
            Serial.print("Budget: ");
            Serial.println(remainingBudget);
        }
        server.sendHeader("Location", String("/"), true);
        server.send(302, "text/plain", "");
    });

    server.on("/data", []() {
        server.send(200, "text/plain", formatItemLines(currentData));
    });

    server.begin();
    Serial.println("HTTP server started");
}

// Loop for reading serial data
void loop() {
    server.handleClient();
    if (Serial.available()) {
        currentData = Serial.readStringUntil('\n'); // Read until newline
        if (currentData.startsWith("Update:")) {
            handleItemUpdates(currentData.substring(7));  // Process the item data
        }
    }
}

// Calculating remaining budget
void calculateRemainingBudget() {
    // Parse total amount from the currentData string
    int lastCommaIndex = currentData.lastIndexOf(',');
    if (lastCommaIndex != -1) {
        float totalAmount = currentData.substring(lastCommaIndex + 1).toFloat();
        remainingBudget = totalBudget - totalAmount;
    }
}

// Update budget price
void updateBudget(float amount) {
  // Function to modify budget and send update
  budget -= amount;
  Serial.println(budget); // Send updated budget to Arduino
}

// Handle item changes and changing between modes (standard, low budget and no budget mode)
void handleItemUpdates(String data) {
    float totalScannedAmount = extractTotalAmount(data);
    remainingBudget = totalBudget - totalScannedAmount;
    Serial.print("Budget: ");
    Serial.println(remainingBudget);  // Send the updated budget back to Arduino

    // Calculate the percentage of the remaining budget
    float budgetPercentage = (remainingBudget / totalBudget) * 100;
    Serial.print("Budget Percentage: ");  // Debugging
    Serial.println(budgetPercentage);

    if (budgetPercentage <= 20 && budgetPercentage > 0) { // Check that percentage is valid and below threshold
        Serial.println("LowBudget"); // Send a low budget warning to Arduino
    } else if (budgetPercentage <= 0) { // Check if budget percentage is zero or negative
        Serial.println("NoBudget"); // Send a no budget warning to Arduino
    }
}

float extractTotalAmount(String data) {
    int totalPos = data.lastIndexOf("Total,");
    if (totalPos != -1) {
        return data.substring(totalPos + 6).toFloat();
    }
    return 0.0;
}


String formatItemLines(const String& data) {
    // Making a table for webserver. Used to hold the text in place.
    String formattedLines = "<table>";
    formattedLines += "<tr><th>Item</th><th style='text-align:right;'>Unit Price</th><th style='text-align:right;'>Amount</th><th style='text-align:right;'>Price</th></tr>";

    // Correctly skip the 'Update:' prefix if present
    int updatePos = data.indexOf("Update:");
    String cleanData = (updatePos != -1) ? data.substring(updatePos + strlen("Update:")) : data;

    // Look for commas (used for data)
    int lastCommaPos = cleanData.lastIndexOf(',');
    String itemsData = cleanData.substring(0, lastCommaPos);
    String totalAmount = cleanData.substring(lastCommaPos + 1);

    // Iterate through the itemsData string which contains item details separated by semicolons
    int start = 0;
    int sepPos = itemsData.indexOf(';');
    // Loop through the entire string to extract and process each item's data
    while (start < itemsData.length() && sepPos != -1) {
        String line = itemsData.substring(start, sepPos); // Extract the substring up to the semicolon
        processLine(formattedLines, line); // Process the line to format it for HTML display
        start = sepPos + 1; // Move the start index past the current semicolon
        sepPos = itemsData.indexOf(';', start); // Find the next semicolon from the new start position
    }
    // Check if there's any remaining part of the string after the last semicolon to process
    if (start < itemsData.length()) {
        String line = itemsData.substring(start);
        processLine(formattedLines, line);
    }

    formattedLines += "<tr><td colspan='3' style='text-align:right;'>TOTAL AMOUNT:</td><td style='text-align:right;'>" + totalAmount + " kr.</td></tr>";
    formattedLines += "</table>";
    formattedLines += "<div style='text-align:right;'>TIMESTAMP: " + getTimestamp() + "</div>";
    formattedLines += "<div style='text-align:right;'>Remaining Budget: " + String(remainingBudget, 2) + " kr.</div>";

    return formattedLines;
}

// Printing variables to the webserver
void processLine(String& formattedLines, const String& line) {
    int commaPos = line.indexOf(','); // Find the position of the comma that separates item name from quantity
    if (commaPos != -1) { // Check if the comma is found in the string
        String itemName = line.substring(0, commaPos); // Extract the item name from the start of the string to the comma
        String itemQuantity = line.substring(commaPos + 1); // Extract the quantity after the comma
        float unitPrice = getItemPrice(itemName); // Get the unit price of the item based on its name
        float itemTotalPrice = unitPrice * itemQuantity.toInt(); // Calculate total price by multiplying unit price with quantity
        formattedLines += "<tr><td style='text-align:left;'>" + itemName + "</td><td>" + String(unitPrice, 2) + " kr.</td><td>" + itemQuantity + "</td><td>" + String(itemTotalPrice, 2) + " kr.</td></tr>";
    }
}

// Making a timestamp for the receipt
String getTimestamp() {
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}

// Get item price by name
float getItemPrice(const String& itemName) {
    if (itemName == "Milk") return 7.0;
    else if (itemName == "Egg") return 25.0;
    else if (itemName == "Butter") return 18.0;
    else if (itemName == "Chocolate") return 20.0;
    return 0;
}

// Setting up the webserver design in HTML
String buildWebpage() {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Receipt</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<link rel=\"icon\" href=\"data:,\">";
    html += "<style>body { font-family: 'Courier New', monospace; font-size: 14px; margin: 0; padding: 0; background-color: #f4f4f4; }";
    html += ".receipt { margin: 20px auto; padding: 10px; background: #fff; border: 1px dashed #000; width: auto; display: table; }";
    html += "table { width: 100%; border-collapse: collapse; }";
    html += "th, td { padding: 8px; }";
    html += "th { text-align: left; }";
    html += "td { text-align: right; }";
    html += ".title { text-align: center; font-weight: bold; font-size: 24px; margin: 0; }";
    html += ".budget-form { text-align: center; margin: 20px; }";  // Adding style for the budget form
    html += "</style>";
    html += "<script>function fetchData() {";
    html += "fetch('/data').then(response => response.text()).then(data => {";
    html += "document.getElementById('receipt').innerHTML = data;";
    html += "});";
    html += "setTimeout(fetchData, 2000);"; // Poll every 2000 milliseconds
    html += "}";
    html += "window.onload = fetchData;</script>";
    html += "</head><body>";
    html += "<div class='budget-form'>";  // Wrap the form in a div with class for styling
    html += "<form action='/submit_budget' method='POST'>";
    html += "Budget: <input type='number' name='budget' step='0.01' required> kr. ";
    html += "<input type='submit' value='Set'>";
    html += "</form>";
    html += "</div>";
    html += "<div class='receipt'><div class='title'>RECEIPT</div><div id='receipt'>Loading...</div></div>";
    html += "</body></html>";
    return html;
}
