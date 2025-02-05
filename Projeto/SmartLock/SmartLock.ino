#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include "WiFiCredentials.h"

#define DEBUG 1

// Define the I2C address for the PCF8574
#define PCF8574_ADDRESS 0x20

// Instantiate the PCF8574 object
PCF8574 pcf8574(PCF8574_ADDRESS);

// Define the PCF8574 pins used for rows and columns
// In this configuration, pins P1, P2, and P3 are used for rows,
// and pins P4, P5, and P6 are used for columns
const uint8_t rowPins[3] = {1, 2, 3};
const uint8_t colPins[3] = {4, 5, 6};

// Define a key mapping for the 3x3 matrix
// Adjust the mapping if your keys are labeled differently
char keyMap[3][3] =
{
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'}
};

// Lock-related constants
const uint8_t lockPin = 16;
const uint8_t incorrectCredentialLED = 17;
const uint32_t unlockDuration = 2000;
const uint32_t incorrectCredentialOnDuration = 1000;

// const for credential length
const uint8_t credentialLength = 4;

// Event-related types and structures
enum EventType {
  PASSWORD,
  BUTTON,
  RFID,
};

union Credential
{
  char password[credentialLength + 1];
  uint8_t UID[credentialLength + 1];
  char button[credentialLength + 1];
};

typedef struct {
  EventType type;
  struct tm timestamp;
  bool valid;
  Credential credential;
} EventLog;

// Log-related constants
const uint8_t logBufferSize = 16; // Must be a power of 2
const uint32_t logWriteInterval = 10000;
const char* ntpServer = "time.google.com";
const long gmtOffset_sec = -3 * 3600;
const int daylightOffset_sec = 0;

// Log-related variables
EventLog logBuffer[logBufferSize];
uint8_t logBufferWritePos = 0;
uint8_t logBufferReadPos = 0;

// LogBuffer protection mutex
SemaphoreHandle_t logMutex;

// Define the password
const char passLength = credentialLength;
const char password[] = "1234";

// Button-related constants
const uint8_t buttonPin = 12;
const uint32_t debounceDelay = 50;
const bool buttonPressedState = LOW;

// RFID-related constants
const uint8_t RST_PIN = 13;
const uint8_t SS_PIN = 14;
const uint8_t registeredUID[] = {0xB3, 0x91, 0x21, 0x2D};
const uint8_t registeredUIDSize = sizeof(registeredUID) / sizeof(*registeredUID);

// Common lock-related variables
uint8_t wrongCredential = 0;

// Password-related variables
uint8_t passwordPos = 0;
char enteredPassword[passLength + 1] = "";
bool passwordCorrect = false;

// Button-related variables
bool isButtonPressed = false;

// RFID-related variables
MFRC522 mfrc522(SS_PIN, RST_PIN);
bool validRFID = false;

// Synchronization semaphores
SemaphoreHandle_t lockSemaphore;
SemaphoreHandle_t passSemaphore;
SemaphoreHandle_t buttonSemaphore;
SemaphoreHandle_t buttonISRSemaphore;
SemaphoreHandle_t rfidSemaphore;

void insertLog(EventType type, bool valid, Credential credential)
{
  EventLog logEntry;
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  logEntry.type = type;
  logEntry.timestamp = timeinfo;
  logEntry.valid = valid;
  logEntry.credential = credential;

  xSemaphoreTake(logMutex, portMAX_DELAY);
  logBuffer[logBufferWritePos] = logEntry;
  logBufferWritePos++; // don't need to take mod
                                               // because buffer size is a power of 2 (keep it that way)
  xSemaphoreGive(logMutex);
}

void lockTask(void * parameter)
{
  for (;;) {

    if (wrongCredential) {

      #if DEBUG
      Serial.println("Incorrect Credential");
      #endif

      digitalWrite(incorrectCredentialLED, HIGH);
      vTaskDelay(incorrectCredentialOnDuration / portTICK_PERIOD_MS);
      digitalWrite(incorrectCredentialLED, LOW);

      wrongCredential = 0;
    }

    if (passwordCorrect) {

      #if DEBUG
      Serial.println("Unlock [password]");
      #endif
      
      digitalWrite(lockPin, HIGH);
      vTaskDelay(unlockDuration / portTICK_PERIOD_MS);
      digitalWrite(lockPin, LOW);
      
      #if DEBUG
      Serial.println("Lock");
      #endif

      // Signals that passwordCorrect was used
      xSemaphoreGive(lockSemaphore);

      // Wait until the password is reseted
      xSemaphoreTake(passSemaphore, portMAX_DELAY);
    }

    if (isButtonPressed) {

      #if DEBUG
      Serial.println("Unlock [button]");
      #endif

      digitalWrite(lockPin, HIGH);
      vTaskDelay(unlockDuration / portTICK_PERIOD_MS);
      digitalWrite(lockPin, LOW);

      #if DEBUG
      Serial.println("Lock");
      #endif

      // Signals that the button was used
      xSemaphoreGive(lockSemaphore);

      // Wait until the button is released
      xSemaphoreTake(buttonSemaphore, portMAX_DELAY);
    }

    if (validRFID) {
      
      #if DEBUG
      Serial.println("Unlock [RFID]");
      #endif

      digitalWrite(lockPin, HIGH);
      vTaskDelay(unlockDuration / portTICK_PERIOD_MS);
      digitalWrite(lockPin, LOW);

      #if DEBUG
      Serial.println("Lock");
      #endif

      // Signals that the RFID was used
      xSemaphoreGive(lockSemaphore);

      // Wait until the RFID is reseted
      xSemaphoreTake(rfidSemaphore, portMAX_DELAY);
    }

    // Short delay before the next cycle
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void logTask(void * parameter)
{
  EventLog tempLogBuffer[logBufferSize];
  bool logSent;

  for (;;) {
    vTaskDelay(logWriteInterval / portTICK_PERIOD_MS);

    xSemaphoreTake(logMutex, portMAX_DELAY);
    memcpy(tempLogBuffer, logBuffer, sizeof(logBuffer));
    xSemaphoreGive(logMutex);

    // For each unread log entry
    while (logBufferReadPos != logBufferWritePos) {
      // Retrieve the log entry and increment the read position
      EventLog logEntry = tempLogBuffer[logBufferReadPos];

      // Prepare logSent flag
      logSent = false;

      /*
        Send the log entry
      */

      if (true) { // Make it conditional to the log transmission being successful
        logSent = true;
      }

      if (logSent) {
        // Increment the read position
        logBufferReadPos++; // don't need to take mod
                            // because buffer size is a power of 2 (keep it that way)
      }
    }
  }
}

void passwordTask(void * parameter)
{
  for (;;) {
    // Iterate through each row
    for (uint8_t row = 0; row < 3; row++) {
      // Ensure all rows are HIGH before scanning
      for (uint8_t i = 0; i < 3; i++) {
        pcf8574.write(rowPins[i], HIGH);
      }
      // Drive the current row LOW
      pcf8574.write(rowPins[row], LOW);

      // Allow the signal to stabilize
      vTaskDelay(10 / portTICK_PERIOD_MS);

      // Check each column for a key press
      for (uint8_t col = 0; col < 3; col++) {
        if (pcf8574.read(colPins[col]) == LOW) {
          char key = keyMap[row][col];

          #if DEBUG
          Serial.print("Key Pressed: ");
          Serial.println(key);
          #endif

          enteredPassword[passwordPos] = key;
          passwordPos++;

          if (passwordPos == passLength) {

            #if DEBUG
            Serial.print("Entered Password: ");
            Serial.println(enteredPassword);
            #endif

            // Check if the entered password is correct
            passwordCorrect = (strcmp(enteredPassword, password) == 0);

            if (passwordCorrect) {

              #if DEBUG
              Serial.println("Password Correct");
              #endif

              // Wait until lockTask uses passwordCorrect
              xSemaphoreTake(lockSemaphore, portMAX_DELAY);
            } else {
              
              #if DEBUG
              Serial.println("Password Incorrect");
              #endif

              wrongCredential = 1;
            }

            // Insert the log entry
            Credential logCredential;
            strncpy(logCredential.password, enteredPassword, passLength);
            insertLog(PASSWORD, passwordCorrect, logCredential);

            memset(enteredPassword, 0, sizeof(enteredPassword));
            passwordPos = 0;
            passwordCorrect = false;

            // Signals that the password was reseted
            // and thus the lockTask can use passwordCorrect
            xSemaphoreGive(passSemaphore);
          }

          // Debounce: wait until the key is released
          while (pcf8574.read(colPins[col]) == LOW) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
          }
          // Additional delay to avoid multiple detections
          vTaskDelay(50 / portTICK_PERIOD_MS);
        }
      }
    }
    // Short delay before the next scan cycle
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void IRAM_ATTR handleButtonInterrupt() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(buttonISRSemaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void buttonTask(void * parameter)
{
  for (;;) {
    // Wait indefinitely until the semaphore is given by the ISR
    if (xSemaphoreTake(buttonISRSemaphore, portMAX_DELAY) == pdTRUE) {
      vTaskDelay(debounceDelay / portTICK_PERIOD_MS);

      if (digitalRead(buttonPin) == buttonPressedState) {

        #if DEBUG
        Serial.println("Button Pressed");
        #endif

        insertLog(BUTTON, true, (Credential) { .button = "HOME" });

        isButtonPressed = true;
        xSemaphoreTake(lockSemaphore, portMAX_DELAY);
        isButtonPressed = false;
        xSemaphoreGive(buttonSemaphore);
      }
    }
  }
}

void RFIDTask(void * parameter)
{
  for(;;) {
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      continue;
    }

    #if DEBUG
    Serial.println("RFID Card Detected");
    #endif

    // Check if the card is registered
    bool cardMatch = true;
    if (mfrc522.uid.size != registeredUIDSize) {
      cardMatch = false;
    } else {
      for (uint8_t i = 0; i < registeredUIDSize; i++) {
        if (mfrc522.uid.uidByte[i] != registeredUID[i]) {
          cardMatch = false;
          break;
        }
      }
    }

    if (cardMatch) {
      #if DEBUG
      Serial.println("Valid RFID Card");
      #endif

      validRFID = true;
      xSemaphoreTake(lockSemaphore, portMAX_DELAY);
      validRFID = false;
      xSemaphoreGive(rfidSemaphore);
    } else {
      #if DEBUG
      Serial.println("Invalid RFID Card");
      #endif

      wrongCredential = 1;
    }

    // Insert the log entry
    Credential logCredential;
    memcpy(logCredential.UID, mfrc522.uid.uidByte, registeredUIDSize);
    insertLog(RFID, cardMatch, logCredential);

    // Halt the PICC and stop encryption
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

void setup()
{
  // Initialize Serial communication for debugging
  Serial.begin(115200);

  // Initialize the lock pin
  pinMode(lockPin, OUTPUT);

  // Initialize the incorrect credential LED pin
  pinMode(incorrectCredentialLED, OUTPUT);

  // Make sure that logBufferSize is a power of 2
  static_assert((logBufferSize & (logBufferSize - 1)) == 0, "logBufferSize must be a power of 2");

  // Create the mutex for the log buffer
  logMutex = xSemaphoreCreateMutex();

  if (logMutex == NULL) {
    Serial.println("Failed to create log mutex");
    while (1);
  }

  // Connect to the Wi-Fi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  delay(10000);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to Wi-Fi");
    while (1);
  }

  // Configure the NTP client
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Initialize the I2C bus
  Wire.begin();

  // Initialize the PCF8574
  pcf8574.begin();

  // Drive all row pins HIGH
  for (uint8_t i = 0; i < 3; i++) {
    pcf8574.write(rowPins[i], HIGH);
  }

  // Configure the button pin as input
  pinMode(buttonPin, INPUT_PULLUP);

  // Attach the button interrupt
  attachInterrupt(buttonPin, handleButtonInterrupt, FALLING);

  // Initialize the RFID reader
  SPI.begin();
  mfrc522.PCD_Init();

  passSemaphore = xSemaphoreCreateBinary();
  lockSemaphore = xSemaphoreCreateBinary();
  buttonSemaphore = xSemaphoreCreateBinary();
  buttonISRSemaphore = xSemaphoreCreateBinary();
  rfidSemaphore = xSemaphoreCreateBinary();

  if (lockSemaphore == NULL || passSemaphore == NULL || buttonSemaphore == NULL || buttonISRSemaphore == NULL || rfidSemaphore == NULL) {
    Serial.println("Failed to create semaphores");
    while (1);
  }

  // Create the FreeRTOS task for controlling the lock
  xTaskCreate(lockTask, "LockTask", 1024, NULL, 1, NULL);

  // Create the FreeRTOS task for scanning the matrix keyboard
  xTaskCreate(passwordTask, "passwordTask", 2048, NULL, 2, NULL);

  // Create the FreeRTOS task for handling the button
  xTaskCreate(buttonTask, "ButtonTask", 1024, NULL, 2, NULL);

  // Create the FreeRTOS task for handling the RFID
  xTaskCreate(RFIDTask, "RFIDTask", 2048, NULL, 2, NULL);

  // Create the FreeRTOS task for logging
  xTaskCreate(logTask, "LogTask", 2048, NULL, 1, NULL);

}

void loop() {
  // The loop is left empty as all the functionality is handled by the FreeRTOS tasks
}
