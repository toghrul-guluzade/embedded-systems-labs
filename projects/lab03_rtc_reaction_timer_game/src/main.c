#include <LiquidCrystal.h>
#include <Bonezegei_DS1307.h>

Bonezegei_DS1307 rtc(0x68);

#define button 2
#define buzzer 8

//Win case sound
void buzzer_win(){
  tone(buzzer, 329);
  delay(100);
  noTone(buzzer);
  tone(buzzer, 415);
  delay(100);
  noTone(buzzer);
  tone(buzzer, 440);
  delay(100);
  noTone(buzzer);
  tone(buzzer, 493);
  delay(300);
  noTone(buzzer);

  delay(1000);
}

//Lost case sound
void buzzer_lost(){
  tone(buzzer, 329);
  delay(100);
  noTone(buzzer);
  tone(buzzer, 293);
  delay(100);
  noTone(buzzer);
  tone(buzzer, 277);
  delay(100);
  noTone(buzzer);
  tone(buzzer, 246);
  delay(500);
  noTone(buzzer);
  tone(buzzer, 220);
  delay(600);
  noTone(buzzer);

  delay(1000);
}



long debounceDelay = 50;

bool button_press(){

  static int stable_state = HIGH;
  static int last_reading = HIGH;
  static unsigned long lastChange = 0;

  int reading = digitalRead(button);

  if(reading != last_reading){
    lastChange = millis();
    last_reading = reading;
  }

  if((millis() - lastChange) > debounceDelay){ 
    if(reading != stable_state){
      stable_state = reading;

    if(stable_state == LOW) return true;
    } 
  } 
  
  return false;
}


const int rs = 12, en = 11, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

  int count = 0;



void setup() {

  lcd.begin(16, 2);
  rtc.begin();
  lcd.setCursor(0, 0);
  rtc.setTime("00:00:00");

  pinMode(buzzer, OUTPUT);
  pinMode(button, INPUT_PULLUP);


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Count: ");
}

void loop() {

  if(!rtc.getTime()) return;

  int s = rtc.getSeconds();

  if(s != last_second){
    last_second = s;
    
    lcd.setCursor(6, 0);
    lcd.print(count);
 
    count++;

  }

  bool button_state = button_press();
   if(button_state && (count < 9)){
      lcd.clear();
      lcd.home();
      lcd.print("Failed!");
      lcd.setCursor(0, 1); 
      lcd.print("Try Again!");
      buzzer_lost();
      
      count = 0;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Count: ");

    } else if(button_state && (count >= 9)){
        lcd.clear();
        lcd.home();
        lcd.print("Congrats!    ");
        Serial.print("Congrats!    ");
        buzzer_win();
      
        count = 0;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Count: ");

    } else if(count > 10){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("You missed!");
        buzzer_lost();
      
        count = 0;
      
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Count: ");
    }
}
