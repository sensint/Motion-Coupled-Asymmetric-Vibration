#include <HCSR04.h>

float data; 

UltraSonicDistanceSensor distanceSensor(28,29); 

void setup () {
    Serial.begin(9600);  }

void loop () {
    data = distanceSensor.measureDistanceCm(); 

    if(data > 2.0) {
      Serial.println(data);
     
    }
    else{
      Serial.println(0);
    }
     delay(10);
}