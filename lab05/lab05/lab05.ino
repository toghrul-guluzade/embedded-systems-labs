#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>

// -----------------------------
// Analog pin abstraction
// -----------------------------
typedef enum {
  A0_A, A1_A, A2_A, A3_A, A4_A, A5_A
} AnalogPin;

typedef struct {
  AnalogPin pin;
  uint8_t channel;
} PinMap;

const PinMap adc_map[] = {
  {A0_A, 0},
  {A1_A, 1},
  {A2_A, 2},
  {A3_A, 3},
  {A4_A, 4},
  {A5_A, 5}
};

// -----------------------------
// Button debounce state
// -----------------------------
bool last_reading = 0;           // last raw "pressed" state
bool button_state = 0;           // debounced "pressed" state
uint32_t last_debounceTime = 0;  // last change time (ms)

// current mode flag (0=all_off, 1=all_on)
bool button_stat = false;

// -----------------------------
// GPIO init
// -----------------------------
void initialize() {
  DDRB |= (0b00001111);   // PB0..PB3 outputs (LEDs)
  DDRD &= ~(1 << PD7);    // PD7 input (button)
  PORTD |= (1 << PD7);    // pull-up enabled (active-low button)
}

// -----------------------------
// Debounce (triggers on press)
// -----------------------------
bool button_debounce(void) {
  const uint16_t debounce = 20; // ms

  // pressed = 1 when pin reads low (active low)
  bool pressed = ((PIND & (1 << PD7)) == 0);

  if (pressed != last_reading) {
    last_debounceTime = millis();
    last_reading = pressed;
  }

  if ((millis() - last_debounceTime) >= debounce) {
    if (pressed != button_state) {
      button_state = pressed;
      if (button_state) return true; // trigger on press edge
    }
  }
  return false;
}

// -----------------------------
// ADC init (AVR registers)
// -----------------------------
static void adc_init(void) {
  ADMUX = 0x00;
  ADMUX |= (1 << REFS0);     // AVcc reference (5V)
  ADCSRA |= (1 << ADEN);     // enable ADC
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler 128
}

// Raw ADC read: returns 0..1023
uint16_t analogRead_custom(AnalogPin pin) {
  uint8_t found = 0;

  for (uint8_t i = 0; i < (sizeof(adc_map) / sizeof(adc_map[0])); i++) {
    if (adc_map[i].pin == pin) {
      ADMUX = (ADMUX & 0xF0) | (adc_map[i].channel & 0x0F);
      found = 1;
      break;
    }
  }

  if (!found) return 0;

  // dummy conversion after mux change
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));

  // real conversion
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC));

  return (uint16_t)ADCL | ((uint16_t)ADCH << 8);
}

// -----------------------------
// Joystick direction (for LEDs)
// -----------------------------
int8_t joystick_dir(AnalogPin pin) {
  uint16_t v = analogRead_custom(pin);

  if (v < 500) return -1;
  if (v <= 510) return 0;
  return 1;
}

// -------------------------------------------------
// FIX: swap axes mapping to match your hardware
// If your joystick's VRx is wired to A1 and VRy to A0,
// define them like this.
// -------------------------------------------------
#define X_AXIS A1_A
#define Y_AXIS A0_A

// -----------------------------
// LED behavior: All Off mode
// -----------------------------
void all_off(void) {
  int8_t x = joystick_dir(X_AXIS);
  int8_t y = joystick_dir(Y_AXIS);

  // X axis: PB2 = left, PB0 = right
  if (x == -1) {
    PORTB |=  (1 << PB2);
    PORTB &= ~(1 << PB0);
  } else if (x == 0) {
    PORTB &= ~((1 << PB2) | (1 << PB0));
  } else {
    PORTB |=  (1 << PB0);
    PORTB &= ~(1 << PB2);
  }

  // Y axis: PB1 = down, PB3 = up
  if (y == -1) {
    PORTB |=  (1 << PB1);
    PORTB &= ~(1 << PB3);
  } else if (y == 0) {
    PORTB &= ~((1 << PB1) | (1 << PB3));
  } else {
    PORTB |=  (1 << PB3);
    PORTB &= ~(1 << PB1);
  }
}

// -----------------------------
// LED behavior: All On mode
// -----------------------------
void all_on(void) {
  int8_t x = joystick_dir(X_AXIS);
  int8_t y = joystick_dir(Y_AXIS);

  if (x == -1) {
    PORTB &= ~(1 << PB2);
    PORTB |=  (1 << PB0);
  } else if (x == 0) {
    PORTB |=  ((1 << PB2) | (1 << PB0));
  } else {
    PORTB &= ~(1 << PB0);
    PORTB |=  (1 << PB2);
  }

  if (y == -1) {
    PORTB &= ~(1 << PB1);
    PORTB |=  (1 << PB3);
  } else if (y == 0) {
    PORTB |=  ((1 << PB1) | (1 << PB3));
  } else {
    PORTB &= ~(1 << PB3);
    PORTB |=  (1 << PB1);
  }
}

// -----------------------------
// Arduino setup/loop
// -----------------------------
void setup() {
  initialize();
  adc_init();
  Serial.begin(9600);
}

void loop() {
  // RAW values for UI
  uint16_t raw_x = analogRead_custom(X_AXIS);
  uint16_t raw_y = analogRead_custom(Y_AXIS);

  // toggle mode on each button press
  if (button_debounce()) {
    button_stat = !button_stat;
  }

  if (button_stat) all_on();
  else            all_off();

  // Serial output: raw_x,raw_y,mode
  Serial.print(raw_x);
  Serial.print(',');
  Serial.print(raw_y);
  Serial.print(',');
  Serial.println(button_stat ? 1 : 0);

  delay(20);
}