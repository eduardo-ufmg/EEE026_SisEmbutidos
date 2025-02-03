#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>

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

// Define the password
const char passLength = 4;
const char password[] = "1234";

// Lock-related constants
const uint8_t lockPin = 16;
const uint32_t unlockDuration = 5000; // 5 seconds

// Password-related variables
uint8_t passwordPos = 0;
char enteredPassword[passLength + 1] = "";
bool passwordCorrect = false;

// Lock-related variables
bool isLocked = true;

// Synchronization semaphores
SemaphoreHandle_t lockSemaphore;
SemaphoreHandle_t passSemaphore;

/**
 * @brief FreeRTOS task that scans the 3x3 matrix keyboard
 *
 * This task iterates over each row by driving it LOW while keeping the other rows HIGH
 * It then reads the column pins to determine if a key is pressed. If a pressed key is
 * detected (i.e., the corresponding column reads LOW), the task prints the key value
 * to the Serial monitor. Debouncing is implemented by waiting until the key is released
 *
 * @param parameter Unused
 */
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

          Serial.print("Key Pressed: ");
          Serial.println(key);

          enteredPassword[passwordPos] = key;
          passwordPos ++;

          if (passwordPos == passLength) {

            Serial.print("Entered Password: ");
            Serial.println(enteredPassword);
            
            // Check if the entered password is correct
            passwordCorrect = (strcmp(enteredPassword, password) == 0);

            if (passwordCorrect) {
              Serial.println("Password correct!");

              // Wait until lockTask uses passwordCorrect
              xSemaphoreTake(lockSemaphore, portMAX_DELAY);
            }

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

void lockTask(void * parameter)
{
  for (;;) {
     if (isLocked) {
      isLocked = false;

      if (passwordCorrect) {
        Serial.println("Unlock");
        digitalWrite(lockPin, HIGH);
        vTaskDelay(unlockDuration / portTICK_PERIOD_MS);
        digitalWrite(lockPin, LOW);
        Serial.println("Lock");

        // Signals that passwordCorrect was used
        xSemaphoreGive(lockSemaphore);

        // Wait until the password is reseted
        xSemaphoreTake(passSemaphore, portMAX_DELAY);
      }

      isLocked = true;
    }
    // Short delay before the next cycle
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  // Initialize Serial communication for debugging
  Serial.begin(115200);
  
  // Initialize the I2C bus
  Wire.begin();

  // Initialize the PCF8574
  pcf8574.begin();

  // Configure the row pins as outputs and set them HIGH
  for (uint8_t i = 0; i < 3; i++) {
    pcf8574.write(rowPins[i], HIGH);
  }

  // Initialize the lock pin
  pinMode(lockPin, OUTPUT);

  passSemaphore = xSemaphoreCreateBinary();
  lockSemaphore = xSemaphoreCreateBinary();

  if (passSemaphore == NULL || lockSemaphore == NULL) {
    Serial.println("Failed to create semaphores");
    while (1);
  }

  // Create the FreeRTOS task for scanning the matrix keyboard
  xTaskCreate(
    passwordTask,   // Task function
    "passwordTask", // Name of the task
    2048,           // Stack size in words
    NULL,           // Task input parameter
    2,              // Priority of the task
    NULL            // Task handle
  );

  // Create the FreeRTOS task for controlling the lock
  xTaskCreate(
    lockTask,    // Task function
    "LockTask",  // Name of the task
    1024,        // Stack size in words
    NULL,        // Task input parameter
    1,           // Priority of the task
    NULL         // Task handle
  );
}

void loop() {
  // The loop is left empty as the keyboard scanning is handled by the FreeRTOS task
}
