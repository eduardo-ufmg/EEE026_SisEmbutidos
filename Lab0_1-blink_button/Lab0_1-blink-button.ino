const int ledPin = 2;
const int buttonPin = 3;
const int period = 1000;
const int blinkInterval = period / 2;
const int debounceTime = 100;

bool debounce(int pin, int interval, int stateToCheck);

void setup()
{
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);

  while (not debounce(buttonPin, debounceTime, LOW)) {
    // Wait for the button to be pressed
  }

  while (not debounce(buttonPin, debounceTime, HIGH)) {
    // Wait for the button to be released
  }

}

void loop()
{
  digitalWrite(ledPin, HIGH);
  delay(blinkInterval);
  digitalWrite(ledPin, LOW);
  delay(blinkInterval);
}

bool debounce(int pin, int interval, int stateToCheck) {
  if (digitalRead(pin) == stateToCheck) {
    delay(interval);
    if (digitalRead(pin) == stateToCheck) {
      return true;
    }
  }
  return false;
}
