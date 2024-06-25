#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

//SDA->21,SCL->22 
LiquidCrystal_I2C lcd(0x27,16,2);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

int startBtnInputPin = 25;
int upBtnInputPin = 27;
int resetBtnInputPin = 34;

int startBtnState = 0;
int resetBtnState = 0;
int upBtnState = 0;
int level = 0;
int workoutOn = 0;
int workoutFinish = 0;

const int redPin = 16;
const int greenPin = 17;
const int bluePin = 18;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;

float prevAngle = 0;
const int buzzer = 14; // the buzzer pin

unsigned long currentTime = 0;
unsigned long restartTime = 0;
float timeLimit = 5;

void setColor(int red, int green, int blue) {
  // For common-anode RGB LEDs, use 255 minus the color value
  ledcWrite(redChannel, red);
  ledcWrite(greenChannel, green);
  ledcWrite(blueChannel, blue);
}

void setup()
{
  Serial.begin(115200);

  if(!accel.begin())
  {
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }

  accel.setRange(ADXL345_RANGE_16_G);

  pinMode(startBtnInputPin, INPUT);
  pinMode(resetBtnInputPin, INPUT);
  pinMode(buzzer, OUTPUT);           // For buzzer

  ledcSetup(redChannel, 5000, 8);      // ledcSetup(channel of color, Frequency, Resolution)
  ledcSetup(greenChannel, 5000, 8);
  ledcSetup(blueChannel, 5000, 8);
  ledcAttachPin(redPin, redChannel);
  ledcAttachPin(greenPin, greenChannel);
  ledcAttachPin(bluePin, blueChannel);

  lcd.init();
  lcd.backlight(); 
  lcd.setCursor(1, 0);
  lcd.print("Please wait..."); 
  delay(3000);
  home();
}

void loop()
{
  Serial.println("Beginning of the loop");
  level = 0;
  workoutOn = 0;
  
  startBtnState = digitalRead(startBtnInputPin);
  resetBtnState = digitalRead(resetBtnInputPin);
  delay(100);
  
  if(resetBtnState == 1){
    beepSlow();
    reset();
  }

  if(startBtnState == 1){
    beepSlow();
    start();
    setLevel();
    
    if(workoutOn == 1){
      startMeg();
      workout();
      home();
    }
  }
}

void home(){
  lcd.setCursor(1, 0);
  lcd.print("Let's exercise!");
  lcd.setCursor(0, 1);
  lcd.print("Push S to start!"); 
}

void start(){
  delay(300);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Enter level");
  lcd.setCursor(4, 1);
  lcd.print("level:");
}

void workout(){
  Serial.println("Workout start!");
  int count = 0;
  int targetAngle = 0;

  switch (level){
    case 1:
      targetAngle = 5.0;
      break;
    case 2:
      targetAngle = 6.2;
      break;
    case 3:
      targetAngle = 7.4;
      break;
    case 4:
      targetAngle = 8.6;
      break;
    case 5:
      targetAngle = 9.8;
      break;
  }
  delay(10);

  while(count < 5){
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Let's go!!!");
    lcd.setCursor(2, 1);
    lcd.print("Completed: ");
    lcd.setCursor(13, 1);
    lcd.print(count);

    resetBtnState = digitalRead(resetBtnInputPin);
    float angle = readValueFromMPU();
    delay(10);

    if(resetBtnState == 1){
      beepSlow();
      setColor(0, 0, 0);
      reset();
      break;
    }

    if(angle >= targetAngle){
      setColor(0, 255, 0);
      beepComplete();
    }else if(angle < targetAngle && angle >= targetAngle * 0.9){
      setColor(241, 196, 15);   // YELLOWISH
      beepFast();
    }else if(angle < targetAngle * 0.9 && angle >= targetAngle * 0.7){
      setColor(245, 176, 65);   // Orange
      beepMid();
    }else if(angle < targetAngle * 0.7 && angle >= targetAngle * 0.5){
      setColor(230, 135, 51);   // Light brown
      beepMid();
    }else{
      setColor(255, 0, 0);     // Red
      beepSlow();
    }
    delay(10);
    // setColor(180, 50, 20);     // Brownish

    if(angle >= targetAngle){
      count++;
      lcd.setCursor(13, 1);
      lcd.print(count);
      delay(5000);

      while(angle >= targetAngle){
        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("Plz bring down");
        lcd.setCursor(4, 1);
        lcd.print("the bar");

        resetBtnState = digitalRead(resetBtnInputPin);
        float angle = readValueFromMPU();
        delay(10);

        if(resetBtnState == 1){
          setColor(0, 0, 0);
          reset();
          break;
         }

        if(angle < targetAngle){
          setColor(255, 0, 0);     // Red
          delay(500);
          break;
        }
      }
      
    }

  }
  
  if(count >= 5){
    finishMsg();
  }
}

float readValueFromMPU(){
  sensors_event_t event; 
  accel.getEvent(&event);

  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  "); Serial.println("");
  delay(1000);

  return event.acceleration.x;
}

void setLevel(){
  delay(100);
  lcd.setCursor(11, 1);
  level = 1;
  lcd.print(level);
  startBtnState = 0;

  while(true){
    startBtnState = digitalRead(startBtnInputPin);
    resetBtnState = digitalRead(resetBtnInputPin);
    upBtnState = digitalRead(upBtnInputPin);

    delay(300);

  	if(upBtnState == 1){
      beepSlow();
      level++;
      lcd.setCursor(11, 1);
      lcd.print(level);
    }
    
    if(level > 5){
      level = 1;
      lcd.setCursor(12, 1);
      lcd.print(" ");
      lcd.setCursor(11, 1);
      lcd.print(level);
    }
    
    if(resetBtnState == 1){
      beepSlow();
      reset();
      workoutFinish = 0;
      break;
    }
    
    if(startBtnState == 1){
      beepSlow();
      workoutOn = 1;
      break;
    }
    Serial.println("In setLevel() while loop...");
  }
  Serial.println("Level set up. Ready to work out.");
}

void reset(){
  Serial.println("Reset button has been pressed.");
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Reset...");
  delay(3000);
  home();
}

void startMeg(){
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Put the bar");
  lcd.setCursor(1, 1);
  lcd.print("on your thigh");
  delay(3000);
}

void finishMsg(){
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("It was amazing!");
  lcd.setCursor(4, 1);
  lcd.print("Try more!");
  delay(3000);
  lcd.clear();
}

void beepFast() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(buzzer, HIGH);
    delay(40);
    digitalWrite(buzzer, LOW);
    delay(40);
  }
}

void beepMid() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer, HIGH);
    delay(40);
    digitalWrite(buzzer, LOW);
    delay(30);
  }
}

void beepSlow() {
  digitalWrite(buzzer, HIGH);
  delay(50);
  digitalWrite(buzzer, LOW);
}

void beepComplete() {
  for (int i = 0; i < 1; i++) {
    digitalWrite(buzzer, HIGH);
    delay(2000);
    digitalWrite(buzzer, LOW);
  }
}

