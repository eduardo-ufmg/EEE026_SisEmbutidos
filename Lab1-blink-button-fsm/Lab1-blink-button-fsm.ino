const int buttonPin = 2;
const int ledPin = 3;
const int blink_time = 1000;
const int debounce_time = 50;
const int led_ack_time = 250;
const int button_pressed_state = 0;
const int button_released_state = !button_pressed_state;

enum MAIN_STATE {
  MAIN_STATE_START,
  MAIN_STATE_BUTTON_RELEASED_0,
  MAIN_STATE_BUTTON_PRESSED,
  MAIN_STATE_BUTTON_RELEASED_1,
  MAIN_STATE_LED_BLINK,
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
int button_reading = 0;
int blink_led = 0;
int timer_interrupt_happened = 0;

MAIN_STATE main_state;
LED_BLINK_STATE led_blink_state;
BUTTON_DEBOUNCE_STATE button_debounce_state;

void handle_main_fsm();
void handle_led_blink_fsm();
void handle_button_debounce_fsm();

void on_button_change();
void on_timer_interrupt();

void start_timer_one_shot(int time_ms);

void setup()
{
  main_state = MAIN_STATE_START;
  led_blink_state = LED_BLINK_STATE_WAIT;
  button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT;

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(buttonPin), on_button_change, CHANGE);
}

void loop()
{
  handle_main_fsm();
  handle_led_blink_fsm();
  handle_button_debounce_fsm();
}

void handle_main_fsm()
{
  
}
