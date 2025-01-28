#define DEBUG 1

const int buttonPin = 2;
const int ledPin = 3;
const int blink_led_on_time = 500;
const int blink_led_off_time = 500;
const int debounce_time = 50;
const int led_ack_time = 250;
const int button_pressed_val = 0;
const int button_released_val = !button_pressed_val;
const int counter_prescaler = 1024;
const int timer_bits = 16;

const unsigned long max_count = pow(2, timer_bits) - 1;
const unsigned long cpu_freq_hz = F_CPU;
const unsigned long cpu_freq_khz = cpu_freq_hz / 1000;
const unsigned long cpu_freq_Mhz = cpu_freq_khz / 1000;

const float ticks_to_seconds = (float) counter_prescaler / (float) cpu_freq_hz;

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
  LED_BLINK_STATE_ON_ACK_0,
  LED_BLINK_STATE_OFF_ACK_0,
  LED_BLINK_STATE_ON_ACK_1,
  LED_BLINK_STATE_OFF_ACK_1,
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

char process_isr = 0;
char is_button_pressed = 0;
char blink_led = 0;

unsigned button_pressed_ticks = 0;
unsigned long total_button_pressed_ticks = 0;
float button_pressed_time_s = 0;
char times_to_blink = 0;
char should_count = 0;

volatile char button_reading = 0;
volatile char timer_ctc_interrupt_happened = 0;
volatile char button_interrupt_happened = 0;
volatile char ovf_add = 0;

MAIN_STATE main_state;
LED_BLINK_STATE led_blink_state;
BUTTON_DEBOUNCE_STATE button_debounce_state;

void handle_main_fsm();
void handle_led_blink_fsm();
void handle_button_debounce_fsm();

void configure_timer_for_one_shot();
void configure_timer_for_counting();

void on_button_change();
void on_timer_ctc_interrupt();
void on_timer_ovf_interrupt();

void start_timer_period(int time_ms);
void start_timer_as_counter();
void capture_ticks();

constexpr unsigned get_best_prescaler(unsigned time_ms);
constexpr char get_prescaler_mask(unsigned prescaler);

void setup()
{
  main_state = MAIN_STATE_START;
  led_blink_state = LED_BLINK_STATE_WAIT;
  button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT;

  process_isr = 1;

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(buttonPin), on_button_change, CHANGE);

  configure_timer_for_one_shot();

  #if DEBUG
  Serial.begin(115200);
  #endif
}

void loop()
{
  handle_main_fsm();
  handle_led_blink_fsm();
  handle_button_debounce_fsm();
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
        button_pressed_ticks = 0;
        ovf_add = 0;
        should_count = 1;
        main_state = MAIN_STATE_BUTTON_PRESSED;
      }
      break;
    case MAIN_STATE_BUTTON_PRESSED:
      if (not is_button_pressed) {
        should_count = 0;

        total_button_pressed_ticks = (unsigned long) button_pressed_ticks + (unsigned long) ovf_add * max_count;

        button_pressed_time_s = (float) total_button_pressed_ticks * ticks_to_seconds;
        times_to_blink = ceil(button_pressed_time_s);

        #if DEBUG
          Serial.print("button_pressed_ticks: ");
          Serial.print(button_pressed_ticks);
          Serial.print(" ovf_add: ");
          Serial.print((int) ovf_add);
          Serial.print(" total_button_pressed_ticks: ");
          Serial.print(total_button_pressed_ticks);
          Serial.print(" button_pressed_time_s: ");
          Serial.print(button_pressed_time_s);
          Serial.print(" times_to_blink: ");
          Serial.println((int) times_to_blink);
        #endif

        main_state = MAIN_STATE_BUTTON_RELEASED_1;
      }
      break;
    case MAIN_STATE_BUTTON_RELEASED_1:
      blink_led = 1;

      process_isr = 0;
      
      main_state = MAIN_STATE_LED_BLINK;
      break;
    case MAIN_STATE_LED_BLINK:
      if (not blink_led) {

        process_isr = 1;
        
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
        start_timer_period(blink_led_on_time);
        led_blink_state = LED_BLINK_STATE_ON_BLINK;
      }
      break;
    case LED_BLINK_STATE_ON_BLINK:
      if (timer_ctc_interrupt_happened) {
        timer_ctc_interrupt_happened = 0;
        digitalWrite(ledPin, LOW);
        start_timer_period(blink_led_off_time);
        led_blink_state = LED_BLINK_STATE_OFF_BLINK;
      }
      break;
    case LED_BLINK_STATE_OFF_BLINK:
      if (timer_ctc_interrupt_happened) {

        timer_ctc_interrupt_happened = 0;
        digitalWrite(ledPin, HIGH);

        if (times_to_blink > 1) {
          times_to_blink--;
          led_blink_state = LED_BLINK_STATE_WAIT;
        } else {
          start_timer_period(led_ack_time);
          led_blink_state = LED_BLINK_STATE_ON_ACK_0;
        }

      }
      break;
    case LED_BLINK_STATE_ON_ACK_0:
      if (timer_ctc_interrupt_happened) {
        timer_ctc_interrupt_happened = 0;
        digitalWrite(ledPin, LOW);
        start_timer_period(led_ack_time);
        led_blink_state = LED_BLINK_STATE_OFF_ACK_0;
      }
      break;
    case LED_BLINK_STATE_OFF_ACK_0:
      if (timer_ctc_interrupt_happened) {
        timer_ctc_interrupt_happened = 0;
        digitalWrite(ledPin, HIGH);
        start_timer_period(led_ack_time);
        led_blink_state = LED_BLINK_STATE_ON_ACK_1;
      }
      break;
    case LED_BLINK_STATE_ON_ACK_1:
      if (timer_ctc_interrupt_happened) {
        timer_ctc_interrupt_happened = 0;
        digitalWrite(ledPin, LOW);
        led_blink_state = LED_BLINK_STATE_OFF_ACK_1;
      }
      break;
    case LED_BLINK_STATE_OFF_ACK_1:
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

        if (should_count) {
          start_timer_as_counter();
        }

        button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT_RELEASE;
      } else {
        button_debounce_state = BUTTON_DEBOUNCE_STATE_WAIT_PRESS;
      }
      break;
    case BUTTON_DEBOUNCE_STATE_WAIT_RELEASE:
      if (button_interrupt_happened) {
        button_interrupt_happened = 0;
        if (button_reading == button_released_val) {

          if (should_count) {
            capture_ticks();
          }

          start_timer_period(debounce_time);
          button_debounce_state = BUTTON_DEBOUNCE_STATE_DEBOUNCE_RELEASE;
        }
      }
      break;
    case BUTTON_DEBOUNCE_STATE_DEBOUNCE_RELEASE:
      if (timer_ctc_interrupt_happened) {
        timer_ctc_interrupt_happened = 0;
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
      if (timer_ctc_interrupt_happened) {
        timer_ctc_interrupt_happened = 0;
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

void configure_timer_for_one_shot()
{
  cli();

  TCCR1A = 0;             // clear config register
  TCCR1B = (1 << WGM12);  // configure timer 1 for CTC mode
  TIMSK1 = (1 << OCIE1A); // only CTC interrupt

  sei();
}

void configure_timer_for_counting()
{
  cli();

  TCCR1A = 0;            // clear config register
  TCCR1B = 0;            // clear config register
  TIMSK1 = (1 << TOIE1); // only overflow interrupt
  TCNT1 = 0;             // reset counter

  sei();
}

void on_button_change()
{
  if (not process_isr) {
    return;
  }
  
  button_reading = digitalRead(buttonPin);
  button_interrupt_happened = 1;
}

void on_timer_ctc_interrupt()
{
  timer_ctc_interrupt_happened = 1;
  TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10)); // stop timer
}

void on_timer_ovf_interrupt()
{
  ovf_add++;
}

ISR(TIMER1_COMPA_vect)
{
  on_timer_ctc_interrupt();
}

ISR(TIMER1_OVF_vect)
{
  on_timer_ovf_interrupt();
}

void start_timer_period(int time_ms)
{
  unsigned prescaler = get_best_prescaler(time_ms);
  unsigned mask = get_prescaler_mask(prescaler);

  configure_timer_for_one_shot();
  
  OCR1A = time_ms * cpu_freq_khz / prescaler; // set compare value
  TCCR1B |= mask;                               // start timer with appropriate prescaler
}

void start_timer_as_counter()
{
  configure_timer_for_counting();

  TCCR1B |= get_prescaler_mask(counter_prescaler);  // start timer as counter with 1024 prescaler
}

void capture_ticks()
{
  button_pressed_ticks += TCNT1;
}

constexpr unsigned get_best_prescaler(unsigned time_ms)
{
  return (time_ms * (cpu_freq_khz / 1) <= max_count) ? 1 :
         (time_ms * (cpu_freq_khz / 8) <= max_count) ? 8 :
         (time_ms * (cpu_freq_khz / 64) <= max_count) ? 64 :
         (time_ms * (cpu_freq_khz / 256) <= max_count) ? 256 : 1024;
}

constexpr char get_prescaler_mask(unsigned prescaler)
{
  return (prescaler == 1) ? (1 << CS10) :
         (prescaler == 8) ? (1 << CS11) :
         (prescaler == 64) ? (1 << CS11) | (1 << CS10) :
         (prescaler == 256) ? (1 << CS12) :
         (prescaler == 1024) ? (1 << CS12) | (1 << CS10) : 0;
}
