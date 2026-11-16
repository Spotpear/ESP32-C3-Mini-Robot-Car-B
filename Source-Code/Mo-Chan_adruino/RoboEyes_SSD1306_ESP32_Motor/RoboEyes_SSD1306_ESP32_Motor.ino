#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET  -1

#define LF  0
#define LB  1
#define RB  2
#define RF  3

#define Stop       0
#define BackWard   1
#define ForWard    2
#define FullLeft   3
#define FullRight  4
#define BackLeft   5
#define BackRight  6
#define ForLeft    7
#define ForRight   8

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display); 

byte mood = 0;

void setup() {
  pinMode(LF, OUTPUT);
  pinMode(LB, OUTPUT);
  pinMode(RF, OUTPUT);
  pinMode(RB, OUTPUT);
  delay(3000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);   // screen-width, screen-height, max framerate
  roboEyes.setAutoblinker(ON, 3, 2);                  // Start auto blinker animation cycle -> bool active, int interval, int variation -> turn on/off, set interval between each blink in full seconds, set range for random interval variation in full seconds
  roboEyes.setIdleMode(ON, 2, 2);                     // Start idle animation cycle (eyes looking in random directions) -> turn on/off, set interval between each eye repositioning in full seconds, set range for random time interval variation in full seconds
  roboEyes.setMood(DEFAULT);
}


void loop() {
  roboEyes.update();
  long i = random(100000);
  if (i==1){
    MOTOR(random(9),random(5,50),random(10,100),random(20));
  }
}

void MOTOR(byte c,int t1,int t2,int Time){
  for (int i=0;i<Time;i++){
    switch (c) {
      case 0:
        digitalWrite(LF, LOW);
        digitalWrite(LB, LOW);
        digitalWrite(RF, LOW);
        digitalWrite(RB, LOW);
      break;
      case 1:
        digitalWrite(LF, LOW);
        digitalWrite(LB, HIGH);
        digitalWrite(RF, LOW);
        digitalWrite(RB, HIGH);
      break;
      case 2:
        digitalWrite(LF, HIGH);
        digitalWrite(LB, LOW);
        digitalWrite(RF, HIGH);
        digitalWrite(RB, LOW);
      break;
      case 3:
        digitalWrite(LF, LOW);
        digitalWrite(LB, HIGH);
        digitalWrite(RF, HIGH);
        digitalWrite(RB, LOW);
      break;
      case 4:
        digitalWrite(LF, HIGH);
        digitalWrite(LB, LOW);
        digitalWrite(RF, LOW);
        digitalWrite(RB, HIGH);
      break;
      case 5:
        digitalWrite(LF, LOW);
        digitalWrite(LB, HIGH);
        digitalWrite(RF, LOW);
        digitalWrite(RB, LOW);
      break; 
      case 6:
        digitalWrite(LF, LOW);
        digitalWrite(LB, LOW);
        digitalWrite(RF, LOW);
        digitalWrite(RB, HIGH);
      break; 
      case 7:
        digitalWrite(LF, HIGH);
        digitalWrite(LB, LOW);
        digitalWrite(RF, LOW);
        digitalWrite(RB, LOW);
      break; 
      case 8:
        digitalWrite(LF, LOW);
        digitalWrite(LB, LOW);
        digitalWrite(RF, HIGH);
        digitalWrite(RB, LOW);
      break;     
    }
    delay(t1);
    digitalWrite(LF, LOW);
    digitalWrite(LB, LOW);
    digitalWrite(RF, LOW);
    digitalWrite(RB, LOW);
    delay(t2);
  }
}
