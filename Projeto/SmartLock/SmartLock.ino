#include <Arduino.h>
#include <Wire.h>
#include <PCF8574.h>

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
const uint32_t unlockDuration = 5000; // 5 seconds

// Define the password
const char passLength = 4;
const char password[] = "1234";

// Button-related constants
const uint8_t buttonPin = 12;
const uint32_t debounceDelay = 50; // 50 ms
const bool buttonPressedState = LOW;

// Password-related variables
uint8_t passwordPos = 0;
char enteredPassword[passLength + 1] = "";
bool passwordCorrect = false;

// Button-related variables
bool isButtonPressed = false;

// Synchronization semaphores
SemaphoreHandle_t lockSemaphore;
SemaphoreHandle_t passSemaphore;
SemaphoreHandle_t buttonSemaphore;
SemaphoreHandle_t buttonISRSemaphore;

void lockTask(void * parameter)
{
  for (;;) {

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
      xSemaphoreGive(buttonSemaphore);

      // Wait until the button is released
      xSemaphoreTake(buttonSemaphore, portMAX_DELAY);
    }

    // Short delay before the next cycle
    vTaskDelay(1000 / portTICK_PERIOD_MS);
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
          passwordPos ++;

          if (passwordPos == passLength) {

            #if DEBUG
            Serial.print("Entered Password: ");
            Serial.println(enteredPassword);
            #endif

            // Check if the entered password is correct
            passwordCorrect = (strcmp(enteredPassword, password) == 0);

            if (passwordCorrect) {

              #if DEBUG
              Serial.println("Password correct!");
              #endif

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

        isButtonPressed = true;
        xSemaphoreTake(buttonSemaphore, portMAX_DELAY);
        isButtonPressed = false;
        xSemaphoreGive(buttonSemaphore);
      }
    }
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

  // Configure the button pin as input
  pinMode(buttonPin, INPUT_PULLUP);

  // Attach the button interrupt
  attachInterrupt(buttonPin, handleButtonInterrupt, FALLING);

  // Initialize the lock pin
  pinMode(lockPin, OUTPUT);

  passSemaphore = xSemaphoreCreateBinary();
  lockSemaphore = xSemaphoreCreateBinary();
  buttonSemaphore = xSemaphoreCreateBinary();
  buttonISRSemaphore = xSemaphoreCreateBinary();

  if (passSemaphore == NULL || lockSemaphore == NULL || buttonSemaphore == NULL || buttonISRSemaphore == NULL) {
    Serial.println("Failed to create semaphores");
    while (1);
  }

  // Create the FreeRTOS task for controlling the lock
  xTaskCreate(lockTask, "LockTask", 1024, NULL, 1, NULL);

  // Create the FreeRTOS task for scanning the matrix keyboard
  xTaskCreate(passwordTask, "passwordTask", 2048, NULL, 2, NULL);

  // Create the FreeRTOS task for handling the button
  xTaskCreate(buttonTask, "ButtonTask", 1024, NULL, 2, NULL);

}

void loop() {
  // The loop is left empty as the keyboard scanning is handled by the FreeRTOS task
}
