#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdbool.h>
#include <stdint.h>




//Enumeration representing logical analog pins (Symbolic names)
typedef enum {
  A0_A, A1_A, A2_A, A3_A, A4_A, A5_A //Different than original names (e.g. A0, A1) for avoiding interference with arduino libaries.
} AnalogPin;


//Structrure mapping a logical pin to an ADC channel number
typedef struct{
  AnalogPin pin; //symbolic pin name
  uint8_t channel;
} PinMap;


//Lookup table that maps analogPin to ADC channel number
const PinMap adc_map[]{
  {A0_A, 0},
  {A1_A, 1},
  {A2_A, 2},
  {A3_A, 3},
  {A4_A, 4},
  {A5_A, 5}
};


//------ Global Variables ------


volatile uint32_t elapsed_time = 0; //Milliseconds since startup
volatile uint16_t fraction = 0; //fractional microseconds


//------ Button Debounce State

bool last_reading = 1; //last raw button reading
bool button_state = 1; // debounced button state
uint32_t last_debounceTime = 0; //time of last change


// Initialize GPIO
void initialize () {
  DDRB |= (0b00001111); // PORTB 0-3 OUT
  DDRD &= ~(1 << PD7); // PORTD PD7 IN
  PORTD |= (1 << PD7); // Internal pull-up resistor

}


//-------------------
//TIMER
//-------------------

//----Timer0 initialization (Overflow mode)
static void timer0_init_overflow(void){
  //normal mode
  TCCR0A = 0;
  TCCR0B = 0;

  //Timer tick = 4ns at 16MHz
  //Overflow period = 256 * 4ns = 1024ns
  TCCR0B |= (1 << CS01) | (1 << CS00); // prescaler 64
  TIMSK0 |= (1 << TOIE0);              // overflow interrupt enable

  sei(); //global intrupts enable
}


//-----Timer0 overflow ISR-----

/*
  Timer overflows in each 1024ns (~1.024ms)
  fixing the 0.024ms issue we add 1 ms each overflow accumulate the extra 24ns and add it when
  reaches to 1 second 
*/
ISR(TIMER0_OVF_vect){
  elapsed_time++;      // 1 ms base
  fraction += 24;      // leftover (1024-1000)
  if(fraction >= 1000){
    elapsed_time++; //carry into milliseconds
    fraction -= 1000;
  }
}


// ------ Millisecond timer access (atomic)
uint32_t myMillis(){
  uint32_t t;
  uint8_t s = SREG; //save interrupt state
  cli(); //disable interrupts
  t = elapsed_time; //atomic read
  SREG = s; //load the previous interrup state

  return t;
}



//--------------------
// BUTTON
//--------------------

//-----Button debounce------
bool button_debounce(void){

  const uint16_t debounce = 20; //debaunce time (increase if button still bounces)


  bool reading = (PIND & (1<<PD7)) == 0; //button is active low 

  //if raw reading changed, reset debounce timer
  if(reading != last_reading){
    last_debounceTime = myMillis();
    last_reading = reading;
  }


  //if the reading has been stable long enough (20ms)
  if((myMillis() - last_debounceTime) >= debounce){
    //update debounced state if changed
    if(reading != button_state) {
      button_state = reading;

      //trigger on button press
      if(button_state == 0){
      return true;
      }
    }
  }
    
  return false;
}




//--------------------------------------
//ANALOG
//-------------------------------------



//ADC Initialization
static void adc_init(void){

  //Clear ADMUX 
  ADMUX = 0x00;
  
  //Select AVcc (5V) as reference voltage
  ADMUX |= (1 << REFS0);  // REFS0 = 1, REFS0 = 0

  //Enable ADC subsystem
  ADCSRA |= (1<<ADEN);

  //Set ADC prescaler to 128
  // FCPU = 16MHz -> ADC clock = 125kHz (recommended)
  ADCSRA |= (1<<ADPS2) | (1<< ADPS1) | (1 << ADPS0);
}


//Custom analogRead function using direct register access
uint16_t analogRead(AnalogPin pin){
  
  uint8_t found = 0; //flag to check if pin exists in adc_map


  //search lookup table for matching logical pin
  for(uint8_t i=0; i<sizeof(adc_map)/sizeof(adc_map[0]); i++){
    if(adc_map[i].pin == pin){

      //Clear lower 4 bits and set new ADC channel
      ADMUX = (ADMUX & 0xF0) | (adc_map[i].channel & 0x0F); 

      found = 1; // update flag
      break;
    }
  }

  //if the pin is invalid return 0
  if(found == 0) return 0;


  //start a dummy conversation after MUX change
  //Cleares ADC cample-and-hold capacitor
  ADCSRA |= (1<<ADSC);
  while(ADCSRA  & (1<<ADSC));
  
  //Start the real conversion
  ADCSRA |= (1<<ADSC);
  while(ADCSRA  & (1<<ADSC));


  //Read ADC result
  return (uint16_t)ADCL | ((uint16_t)ADCH << 8);

}






//--------------------------
//JOYSTICK
//--------------------------




/*
  Read Joystick axis and convert it to direction
  Returns: 
   -1 -> joystick moved in negative direction
    0 -> centered
    1 -> joystick moved in positive direction
*/
int8_t joystick(AnalogPin pin){
  //Perform ADC conversion on selected pin
  /*
    Note: analogRead function have internal stabilizer logic after pin changes
    no need to implement again
  */
  uint16_t read = analogRead(pin);

  //Interperet ADC value with small dead zone
  if(read < 500)                   return -1; //negative direction
  if((read > 500) && (read < 510)) return 0; //dead zone
  if(read > 510)                   return 1; //positive direction
}



//Logical joystick mapping
#define X_AXIS A0_A
#define Y_AXIS A1_A



// All Off mode
//Only one LED per axis is active depending on joystick direction
void all_off(void){
  
  //Read joystick directions
  int8_t read_X = joystick(X_AXIS);
  int8_t read_Y = joystick(Y_AXIS);

  // X direction handling
  if(read_X == -1){
    PORTB |=  (1<<PB2); // left LED on
    PORTB &= ~(1<<PB0); // right LED off
  } 
  else if(read_X == 0){
    PORTB &= ~((1 << PB2) | (1 << PB0)); // both off
  }
  else{
    PORTB |= (1 << PB0); // right LED on
    PORTB &= ~(1 << PB2); // left LED off
  }



  // Y axis handling
  if(read_Y == -1){
    PORTB |=  (1<<PB1); // down LED on
    PORTB &= ~(1<<PB3); // up LED off
  }  
  else if(read_Y == 0){
    PORTB &= ~((1 << PB1) | (1 << PB3)); // both off
  }
  else{
    PORTB |= (1 << PB3); // up LED on
    PORTB &= ~(1 << PB1); // down LED off
  } 
    
}



// All On mode
// Both LEDs per axis are on at center, otherwise inverted
void all_on(void){

  // Read joystick directions
  int8_t read_X = joystick(X_AXIS);
  int8_t read_Y = joystick(Y_AXIS);

  // X axis handling
  if(read_X == -1){
    PORTB &= ~(1<<PB2); // left LED off
    PORTB |=  (1<<PB0); // right LED on
  }
  else if(read_X == 0){
    PORTB |= ((1 << PB2) | (1 << PB0)); // both on
  } 
  else{
    PORTB &= ~(1 << PB0); // right LED off
    PORTB |= (1 << PB2); // left LED on
  }


  //Y axis handling
  if(read_Y == -1){
    PORTB &= ~(1<<PB1); // down LED off
    PORTB |=  (1<<PB3); // up LED on
  } 
  else if(read_Y == 0){
    PORTB |= ((1 << PB1) | (1 << PB3)); // both on
  } 
  else{
    PORTB &= ~(1 << PB3); // up LED off
    PORTB |= (1 << PB1); // down LED on
  }
    
}
int main() {


  initialize(); // initialize GPIO
  timer0_init_overflow(); // start millisecond time
  adc_init(); //initialize ADC

  bool button_stat = false; // current mode flag

  while(1){
    // wait for button press to enter "all on" mode
    if(button_debounce()){
      button_stat = true;

      //stay in "all on" mode until button pressed again
      while(button_stat){
        all_on();
        if(button_debounce()){
          button_stat = false;
        }
      }
    } 
    else{
      // Default behavior: "all off" mode
      all_off();
    }
  }
  
 return 1;

}
