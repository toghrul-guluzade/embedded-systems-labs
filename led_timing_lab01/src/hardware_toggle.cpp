#include <Arduino.h>

#define LED1_Target 1
#define LED2_Target 2
#define LED3_Target 3

int led_count = 0;


void setup() {
  // put your setup code here, to run once:
  DDRB |= (1 << DDB1) | (1 << DDB2); // Set pin 9 and pin 10 as output


  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1A |= (1 << COM1A0) | (1 << COM1B0);
  TCCR1B |= (1 <<CS12); // Set prescaler to 256


 // TCCR1B |= (1 << WGM12);  // Enable CTC mode

  OCR1A = 15625; // Set compare match register for 0.5 second at 16MHz with 256 prescaler  
  OCR1B = 31250;

  sei(); // Enable global interrupts

}


// ISR(TIMER1_COMPA_vect){

//   led_count++;
//   if(led_count % LED1_Target == 0){
//     PORTB ^= (1 << PORTB5); // Toggle pin 13

//   }

//   if(led_count % LED2_Target == 0){
//     PORTB ^= (1 << PORTB4); // Toggle pin 12

//   }
//   if(led_count % LED3_Target == 0){
//     PORTB ^= (1 << PORTB3); // Toggle pin 11
//   }


//   if(led_count >= 6){
//     led_count = 0;  
//   }

// }

void loop() {
  // put your main code here, to run repeatedly:


}