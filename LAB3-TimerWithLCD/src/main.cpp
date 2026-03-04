#include <Arduino.h>
#include <util/delay.h>



//------ Global Variables ------


volatile uint32_t elapsed_time = 0; //Milliseconds since startup
volatile uint16_t fraction = 0; //fractional microseconds













//-----------------------------LCD

#define RS  PB4
#define EN  PB3

#define D4  PD2
#define D5  PD3
#define D6  PD4
#define D7  PD5

void LCD_INIT(){
  DDRB |= (1 << RS) | (1 << EN);
  DDRD |= (1 << D4) | (1 << D5) | (1 << D6) | (1 << D7);
  _delay_ms(15);

  send_nibble(0x3);
  _delay_ms(5);
  send_nibble(0x3);
  _delay_us(100);
  send_nibble(0x3);
  _delay_us(100);
  send_nibble(0x2);
  _delay_us(100);

    // REQUIRED configuration commands (now in 4-bit mode)
  lcd_cmd(0x28);          // 4-bit, 2-line
  lcd_cmd(0x0C);          // display on
  lcd_cmd(0x01);          // clear
  _delay_ms(2);           // <-- important!
  lcd_cmd(0x06);          // entry mode
}

void lcd_write_en(){
  
  PORTB |= (1 << EN);
  _delay_us(1);
  PORTB &= ~(1 << EN);
  _delay_us(1);
  

}


void write_nibble(uint8_t nibble){
  nibble &= 0x0F;

  PORTD &= ~((1 << D4) | (1<< D5) | (1<<D6) | (1<<D7));
  PORTD |= (nibble << 2);
}

void send_nibble(uint8_t nibble){
  nibble &= 0x0F;

  write_nibble(nibble);
  lcd_write_en();
}

void send_byte(uint8_t byte){
  uint8_t high_byte = (byte >> 4);
  uint8_t low_byte = byte & 0x0F;
  send_nibble(high_byte);
  send_nibble(low_byte);
}


void lcd_cmd(uint8_t byte){
  PORTB &= ~(1 << RS);
  send_byte(byte);
  _delay_ms(2);
}

void lcd_data(uint8_t byte){
  PORTB |= (1 << RS);
  send_byte(byte);
  _delay_ms(2);
}

void liquicDisplay();

void lcd_puts(const char *s)
{
  while (*s) {
    lcd_data(*s++);
  }
}

void setup() {
  LCD_INIT();

  lcd_cmd(0x80);
  lcd_puts("Hello,");

  lcd_cmd(0xC0);
  lcd_puts("World!");
}

void loop() {
  // nothing
}


