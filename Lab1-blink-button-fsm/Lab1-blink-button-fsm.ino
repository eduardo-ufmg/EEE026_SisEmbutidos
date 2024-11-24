// simulation: https://wokwi.com/projects/415435578557033473

#include <TimerOne.h>

#define DEBUG 0

const int buttonPin = 2;
const int ledPin = 3;
const int blink_time = 1000;
const int debounce_time = 50;
const int led_ack_time = 250;
const int button_pressed_val = 0;
const int button_released_val = !button_pressed_val;

enum MAIN_STATE {
  MAIN_STATE_START,
  MAIN_STATE_BUTTON_RELEASED_0,
  MAIN_STATE_BUTTON_PRESSED,
  MAIN_STATE_BUTTON_RELEASED_1,
  MAIN_STATE_LED_BLINK,
  MAIN_STATE_WAIT_BUTTON_RELEASE,
};

enum LED_BLINK_STATE {
  LED_BLINK_STATE_WAIT,
  LED_BLINK_STATE_ON_BLINK,
  LED_BLINK_STATE_OFF_BLINK,
  LED_BLINK_STATE_ON_ACK,
  LED_BLINK_STATE_OFF_ACK,
};

enum BUTTON_DEBOUNCE_STATE {
  BUTTON_DEBOUNCE_STATE_WAIT,
  BUTTON_DEBOUNCE_STATE_CHECK_STATE,
  BUTTON_DEBOUNCE_STATE_WAIT_RELEASE,
  BUTTON_DEBOUNCE_STATE_DEBOUNCE_RELEASE,
  BUTTON_DEBOUNCE_STATE_CHECK_RELEASE,
  BUTTON_DEBOUNCE_STATE_RELEASED,
  BUTTON_DEBOUNCE_STATE_WAIT_PRESS,
  BUTTON_DEBOUNCE_STATE_DEBOUNCE_PRESS,
  BUTTON_DEBOUNCE_STATE_CHECK_PRESS,
  BUTTON_DEBOUNCE_STATE_PRESSED,
};

int is_button_pressed = 0;
int blink_led = 0;

volatile int button_reading = 0;
volatile int timer_interrupt_happened = 0;
volatile int button_interrupt_happened = 0;

MAIN_STATE main_state;
LED_BLINK_STATE led_blink_state;
BUTTON_DEBOUNCE_STATE button_debounce_state;

#if DEBUG
MAIN_STATE last_main_state;
LED_BLINK_STATE last_led_blink_state;
BUTTON_DEBOUNCE_STATE last_button_debounce_state;
#endif

void handle_main_fsm();
void handle_led_blink_fsm();
void handle_button_debounce_fsm();

void on_button_change();
void on_timer_interrupt();

void start_timer_period(int time_ms);

#if DEBUG
void debug_fsm_states();
#endif

void setup()
{
  main_state = MAIN_STATE_START;
  led_blink_state = LED_BLINK_STATE_WAIT;
  button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT;

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(buttonPin), on_button_change, CHANGE);

  Timer1.initialize();
  Timer1.attachInterrupt(on_timer_interrupt);

  #if DEBUG
  Serial.begin(115200);
  #endif

}

void loop()
{
  handle_main_fsm();
  handle_led_blink_fsm();
  handle_button_debounce_fsm();

  #if DEBUG
  debug_fsm_states();
  #endif
}

void handle_main_fsm()
{
  switch (main_state) {
    case MAIN_STATE_START:
      if (not is_button_pressed) {
        main_state = MAIN_STATE_BUTTON_RELEASED_0;
      }
      break;
    case MAIN_STATE_BUTTON_RELEASED_0:
      if (is_button_pressed) {
        main_state = MAIN_STATE_BUTTON_PRESSED;
      }
      break;
    case MAIN_STATE_BUTTON_PRESSED:
      if (not is_button_pressed) {
        main_state = MAIN_STATE_BUTTON_RELEASED_1;
      }
      break;
    case MAIN_STATE_BUTTON_RELEASED_1:
      blink_led = 1;
      main_state = MAIN_STATE_LED_BLINK;
      break;
    case MAIN_STATE_LED_BLINK:
      if (not blink_led) {
        main_state = MAIN_STATE_WAIT_BUTTON_RELEASE;
      }
      break;
    case MAIN_STATE_WAIT_BUTTON_RELEASE:
      if (not is_button_pressed) {
        main_state = MAIN_STATE_BUTTON_RELEASED_0;
      }
      break;
    default:
      main_state = MAIN_STATE_START;
      break;
  }
}

void handle_led_blink_fsm()
{
  switch (led_blink_state) {
    case LED_BLINK_STATE_WAIT:
      if (blink_led) {
        digitalWrite(ledPin, HIGH);
        start_timer_period(blink_time);
        led_blink_state = LED_BLINK_STATE_ON_BLINK;
      }
      break;
    case LED_BLINK_STATE_ON_BLINK:
      if (timer_interrupt_happened) {
        timer_interrupt_happened = 0;
        digitalWrite(ledPin, LOW);
        start_timer_period(blink_time);
        led_blink_state = LED_BLINK_STATE_OFF_BLINK;
      }
      break;
    case LED_BLINK_STATE_OFF_BLINK:
      if (timer_interrupt_happened) {
        timer_interrupt_happened = 0;
        digitalWrite(ledPin, HIGH);
        start_timer_period(led_ack_time);
        led_blink_state = LED_BLINK_STATE_ON_ACK;
      }
      break;
    case LED_BLINK_STATE_ON_ACK:
      if (timer_interrupt_happened) {
        timer_interrupt_happened = 0;
        digitalWrite(ledPin, LOW);
        led_blink_state = LED_BLINK_STATE_OFF_ACK;
      }
      break;
    case LED_BLINK_STATE_OFF_ACK:
      blink_led = 0;
      led_blink_state = LED_BLINK_STATE_WAIT;
      break;
    default:
      led_blink_state = LED_BLINK_STATE_WAIT;
      break;
  }
}

void handle_button_debounce_fsm()
{
  switch (button_debounce_state) {
    case BUTTON_DEBOUNCE_STATE_WAIT:
      if (not blink_led) {
        button_debounce_state = BUTTON_DEBOUNCE_STATE_CHECK_STATE;
      }
      break;
    case BUTTON_DEBOUNCE_STATE_CHECK_STATE:
      if (is_button_pressed) {
        button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT_RELEASE;
      } else {
        button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT_PRESS;
      }
      break;
    case BUTTON_DEBOUNCE_STATE_WAIT_RELEASE:
      if (button_interrupt_happened) {
        button_interrupt_happened = 0;
        if (button_reading == button_released_val) {
          start_timer_period(debounce_time);
          button_debounce_state = BUTTON_DEBOUNCE_STATE_DEBOUNCE_RELEASE;
        }
      }
      break;
    case BUTTON_DEBOUNCE_STATE_DEBOUNCE_RELEASE:
      if (timer_interrupt_happened) {
        timer_interrupt_happened = 0;
        button_debounce_state = BUTTON_DEBOUNCE_STATE_CHECK_RELEASE;
      }
      break;
    case BUTTON_DEBOUNCE_STATE_CHECK_RELEASE:
      if (button_reading == button_released_val) {
        button_debounce_state = BUTTON_DEBOUNCE_STATE_RELEASED;
      } else {
        button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT;
      }
      break;
    case BUTTON_DEBOUNCE_STATE_RELEASED:
      is_button_pressed = 0;
      button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT;
      break;
    case BUTTON_DEBOUNCE_STATE_WAIT_PRESS:
      if (button_interrupt_happened) {
        button_interrupt_happened = 0;
        if (button_reading == button_pressed_val) {
          start_timer_period(debounce_time);
          button_debounce_state = BUTTON_DEBOUNCE_STATE_DEBOUNCE_PRESS;
        }
      }
      break;
    case BUTTON_DEBOUNCE_STATE_DEBOUNCE_PRESS:
      if (timer_interrupt_happened) {
        timer_interrupt_happened = 0;
        button_debounce_state = BUTTON_DEBOUNCE_STATE_CHECK_PRESS;
      }
      break;
    case BUTTON_DEBOUNCE_STATE_CHECK_PRESS:
      if (button_reading == button_pressed_val) {
        button_debounce_state = BUTTON_DEBOUNCE_STATE_PRESSED;
      } else {
        button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT;
      }
      break;
    case BUTTON_DEBOUNCE_STATE_PRESSED:
      is_button_pressed = 1;
      button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT;
      break;
    default:
      button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT;
      break;
  }
}

void on_button_change()
{
  button_reading = digitalRead(buttonPin);
  button_interrupt_happened = 1;
}

void on_timer_interrupt()
{
  timer_interrupt_happened = 1;
  Timer1.stop();
}

void start_timer_period(int time_ms)
{
  Timer1.setPeriod(time_ms * 1000);
  Timer1.start();
}

#if DEBUG
void print_fsm_state_main(MAIN_STATE state);
void print_fsm_state_led_blink(LED_BLINK_STATE state);
void print_fsm_state_button_debounce(BUTTON_DEBOUNCE_STATE state);

void debug_fsm_states()
{
  if (main_state != last_main_state
      or led_blink_state != last_led_blink_state
      or button_debounce_state != last_button_debounce_state) {

    print_fsm_state_main(main_state);
    Serial.print("\t");
    print_fsm_state_led_blink(led_blink_state);
    Serial.print("\t");
    print_fsm_state_button_debounce(button_debounce_state);
    Serial.println();

    last_main_state = main_state;
    last_led_blink_state = led_blink_state;
    last_button_debounce_state = button_debounce_state;
  }
}

void print_fsm_state_main(MAIN_STATE state)
{
  Serial.print("MAIN_STATE_");
  switch (state) {
    case MAIN_STATE_START:
      Serial.print("START");
      break;
    case MAIN_STATE_BUTTON_RELEASED_0:
      Serial.print("BUTTON_RELEASED_0");
      break;
    case MAIN_STATE_BUTTON_PRESSED:
      Serial.print("BUTTON_PRESSED");
      break;
    case MAIN_STATE_BUTTON_RELEASED_1:
      Serial.print("BUTTON_RELEASED_1");
      break;
    case MAIN_STATE_LED_BLINK:
      Serial.print("LED_BLINK");
      break;
    case MAIN_STATE_WAIT_BUTTON_RELEASE:
      Serial.print("WAIT_BUTTON_RELEASE");
      break;
    default:
      Serial.print("UNKNOWN");
      break;
  }
}

void print_fsm_state_led_blink(LED_BLINK_STATE state)
{
  Serial.print("LED_BLINK_STATE_");
  switch (state) {
    case LED_BLINK_STATE_WAIT:
      Serial.print("WAIT");
      break;
    case LED_BLINK_STATE_ON_BLINK:
      Serial.print("ON_BLINK");
      break;
    case LED_BLINK_STATE_OFF_BLINK:
      Serial.print("OFF_BLINK");
      break;
    case LED_BLINK_STATE_ON_ACK:
      Serial.print("ON_ACK");
      break;
    case LED_BLINK_STATE_OFF_ACK:
      Serial.print("OFF_ACK");
      break;
    default:
      Serial.print("UNKNOWN");
      break;
  }
}

void print_fsm_state_button_debounce(BUTTON_DEBOUNCE_STATE state)
{
  Serial.print("BUTTON_DEBOUNCE_STATE_");
  switch (state) {
    case BUTTON_DEBOUNCE_STATE_WAIT:
      Serial.print("WAIT");
      break;
    case BUTTON_DEBOUNCE_STATE_CHECK_STATE:
      Serial.print("CHECK_STATE");
      break;
    case BUTTON_DEBOUNCE_STATE_WAIT_RELEASE:
      Serial.print("WAIT_RELEASE");
      break;
    case BUTTON_DEBOUNCE_STATE_DEBOUNCE_RELEASE:
      Serial.print("DEBOUNCE_RELEASE");
      break;
    case BUTTON_DEBOUNCE_STATE_CHECK_RELEASE:
      Serial.print("CHECK_RELEASE");
      break;
    case BUTTON_DEBOUNCE_STATE_RELEASED:
      Serial.print("RELEASED");
      break;
    case BUTTON_DEBOUNCE_STATE_WAIT_PRESS:
      Serial.print("WAIT_PRESS");
      break;
    case BUTTON_DEBOUNCE_STATE_DEBOUNCE_PRESS:
      Serial.print("DEBOUNCE_PRESS");
      break;
    case BUTTON_DEBOUNCE_STATE_CHECK_PRESS:
      Serial.print("CHECK_PRESS");
      break;
    case BUTTON_DEBOUNCE_STATE_PRESSED:
      Serial.print("PRESSED");
      break;
    default:
      Serial.print("UNKNOWN");
      break;
  }
}
#endif
