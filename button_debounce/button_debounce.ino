#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdbool.h>
#include <stdint.h>


//------ Global Variables ------


volatile uint32_t elapsed_time = 0; //Milliseconds since startup
volatile uint16_t fraction = 0; //fractional microseconds


//------ Button Debounce State

bool last_reading = 1; //last raw button reading
bool button_state = 1; // debounced button state
uint32_t last_debounceTime = 0; //time of last change



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


//-----GPIO Initilization-----
static void initialize(void){
  
  DDRB |= (1 << PB5); //PB5 output 
  DDRB &= ~(1<<PB0); //PB0 input
  PORTB |= (1 << PB0); //internal pull-up resistor
  
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




//-----Button debounce------
bool button_debounce(void){

  const uint16_t debounce = 20; //debaunce time (increase if button still bounces)


  bool reading = (PINB & (1<<PB0)) == 0; //button is active low 

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



//----- MAIN-----
int main() {
  initialize(); //GPIO CONF
  timer0_init_overflow(); //TIMER CONF
 
  while(1){
    if(button_debounce()){
      PORTB ^= (1 << PB5); //Toggle led on each button press
    }

  }
  return 0;

}
