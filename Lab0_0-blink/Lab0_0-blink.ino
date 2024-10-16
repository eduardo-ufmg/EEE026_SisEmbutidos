const int ledPin = 2;
const int period = 1000;
const int blinkInterval = period / 2;

void setup()
{
  pinMode(ledPin, OUTPUT);
}

void loop()
{
  digitalWrite(ledPin, HIGH);
  delay(blinkInterval);
  digitalWrite(ledPin, LOW);
  delay(blinkInterval);
}