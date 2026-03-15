

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




void setup() {
  Serial.begin(9600);
 adc_init();
 
}

void loop() {
  Serial.println(analogRead(A0_A));
}
