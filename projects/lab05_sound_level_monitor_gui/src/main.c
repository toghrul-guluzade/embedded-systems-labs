#include <LiquidCrystal.h>
#include <math.h>

#define MIC_PIN A0
#define LED_PIN 8

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const float thresholdDB = 40.0;
const float calibrationOffset = 46.0;   // tune this
void setup()
{
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);

    lcd.begin(16, 2);
    lcd.setCursor(0,0);
    lcd.print("Sound Monitor");
}

void loop()
{
    unsigned long startMillis = millis();

    int signalMax = 0;
    int signalMin = 1023;

    // sample for ~50 ms
    while (millis() - startMillis < 50)
    {
        int sample = analogRead(MIC_PIN);

        if (sample < 1023)
        {
            if (sample > signalMax)
                signalMax = sample;

            if (sample < signalMin)
                signalMin = sample;
        }
    }

    int peakToPeak = signalMax - signalMin;

    // convert amplitude to voltage
    float voltage = peakToPeak * (5.0 / 1023.0);

    /*
    dB=20log10​(V / ​Vref​)
    */

    // simple relative dB estimation
    float dB = 20.0 * log10(voltage / 0.00631);  
    // 0.00631 is a reference constant you can tune during calibration

    dB += calibrationOffset;
    if (dB < 0) dB = 0;

 

    // ---- LCD display ----
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Sound Level:");

    lcd.setCursor(0,1);
    lcd.print(dB,1);
    lcd.print(" dB");

    // ---- Serial output ----
    //Serial.print("DB:");
    Serial.println(dB,1);

    // ---- Threshold control ----
    if (dB >= thresholdDB)
        digitalWrite(LED_PIN, HIGH);
    else
        digitalWrite(LED_PIN, LOW);

    delay(50);
}