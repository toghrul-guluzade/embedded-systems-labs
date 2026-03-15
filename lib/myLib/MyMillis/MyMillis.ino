#include <avr/io.h>
#include <avr/interrupt.h>


volatile uint32_t elapsed_time = 0;
volatile uint16_t fraction = 0;

static void timer0_init_overflow(void){
  TCCR0A = 0;
  TCCR0B = 0;

  TCCR0B |= (1 << CS01) | (1 << CS00); // prescaler 64
  TIMSK0 |= (1 << TOIE0);              // overflow interrupt enable

  sei();
}



ISR(TIMER0_OVF_vect){
  elapsed_time ++;
  fraction += 24;

  if(fraction >= 1000){
    elapsed_time++;
    fraction -= 1000;
  }
}

uint32_t myMillis(){
  uint32_t t;
  cli();
  t = elapsed_time;
  sei();
  return t;
}


int main() {
  timer0_init_overflow();

  DDRB |= (1 << PB5);
  // put your main code here, to run repeatedly:
  int second = 1000;
  uint32_t last = myMillis();

  while(1){
    uint32_t now =myMillis();
    if(now - last >= second){
      last += 1000;
      PORTB ^= (1 << PB5);
    }
  
  }

  return 0;
}
