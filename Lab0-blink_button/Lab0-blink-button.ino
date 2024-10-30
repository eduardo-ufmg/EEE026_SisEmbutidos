// simulation: https://wokwi.com/projects/411926223458480129

const int ledPin = 2;
const int buttonPin = 3;
const int period = 2000;
const int blinkInterval = period / 2;
const int debounceTime = 100;
const int ledAckHighDuration = 250;

bool debounce(int pin, int interval, int stateToCheck);
void waitForButtonTobePressedandReleased(int buttonPin, int debounceTime, int pressedState = LOW);
void blinkLed(int ledPin, int interval);
void ackLed(int ledPin, int duration);

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); // no need for external pull-up resistor
}

void loop()
{
  waitForButtonTobePressedandReleased(buttonPin, debounceTime);
  blinkLed(ledPin, blinkInterval);
  ackLed(ledPin, ledAckHighDuration);
}

bool debounce(int pin, int interval, int stateToCheck)
{
  if (digitalRead(pin) == stateToCheck) {
    delay(interval);
    if (digitalRead(pin) == stateToCheck) {
      return true;
    }
  }
  return false;
}

void waitForButtonTobePressedandReleased(int buttonPin, int debounceTime, int pressedState)
{
  int releasedState = not pressedState;

  while (!debounce(buttonPin, debounceTime, releasedState)) {
    // make sure button is not pressed already
  }
  
  while (!debounce(buttonPin, debounceTime, pressedState)) {
    // wait for the button to be pressed
  }

  while (!debounce(buttonPin, debounceTime, releasedState)) {
    // wait for the button to be released
  }
}

void blinkLed(int ledPin, int interval)
{
  int ledState = digitalRead(ledPin);

  digitalWrite(ledPin, not ledState);
  delay(interval);
  digitalWrite(ledPin, ledState);
  delay(interval);
}

void ackLed(int ledPin, int duration)
{
  int ledState = digitalRead(ledPin);

  digitalWrite(ledPin, not ledState);
  delay(duration);
  digitalWrite(ledPin, ledState);
}
