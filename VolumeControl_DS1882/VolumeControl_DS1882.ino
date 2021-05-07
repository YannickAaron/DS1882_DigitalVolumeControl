
/*
 * Digital Volume Control with DS1882 
 *
 * DS1882 board connections:
 *   V+     - to +8..15V (regulated to +7V onboard)
 *   V-     - to -8..15V (regulated to -7V onboard)
 *   I2C C  - to Arduino's SCL (A5 on Duemilanove)
 *   I2C D  - to Arduino's SDA (A4 on Duemilanove)
 *   I2C +  - to Ardunio's 5V
 *   I2C 0  - to Arduino GND
 *   I,O,C  - to analog Input, Output, Common for each channel
 *   COM2GND jumpers on the bottom of the board MUST be shorted
 *   (with e.g. a drop of solder) if the analog Common is not 
 *   connected to Arduino's GND elswhere.
 *
 */

// LiquidCrystal_I2C.h: https://github.com/johnrickman/LiquidCrystal_I2C
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD

// --------------------- LCD --------------------- 
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2); // Change to (0x27,20,4) for 20x4 LCD.

// Create Pixels
byte zero[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
byte one[] = {
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000
};
byte two[] = {
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000
};
byte three[] = {
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100
};
byte four[] = {
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110
};
byte five[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

// --------------------- END OF LCD --------------------- 

// --------------------- FOR DS1882 --------------------- 

// DS1882 I2C address
#define ds1882Addr      (0x28) // default 7-bit address, A0..A2 are LOW

// DS1882 registers addresses and bit masks and values

#define bmReg           (0xC0) // Register address bits 
#define regPot0         (0x00) // Potentiometer 0 register -> 000000
#define regPot1         (0x40) // Potentiometer 1 register -> 01000000
#define regConfig       (0x80) // Configuration register -> 10000000

#define bmValue         (0x3F) // Register value bits -> 00111111
#define bvVolatile      (0x04) // Non-volatile memory disable -> 00000100
#define bvZeroCrossing  (0x02) // Zero crossing detection enable -> 00000010
#define bvCompatibility (0x01) // Enable DS1808 compatibility mode (33 positions + mute) -> 00000001

#define ds1882Conf      (bvZeroCrossing) // non-volatile, zero-crossing enabled, 64 positions

// define max and min volume limits
#define minPosition (0)  // 0dB - no attenuation
#define maxPosition (63) // mute

int8_t currentPosition;

int8_t wireState;

// --------------------- END OF FOR DS1882 --------------------- 

// --------------------- Control ---------------------

const int upButtonPin =  2; 
const int downButtonPin =  3; 
const int controlButtonPin =  4; 

bool control_notKeepedPressed = true;


int controlType = 0;
const char controlTypes[] = "vb"; // 'v'=-> Volume, 'b'-> Basse
char lastControlState = controlTypes[0];

int bassVolume = 8; //maximum 16 *5 für display

int volume = 40; //maximum 80 //16*5 = 80 steps in total
int lineToPrintVolumeOn = 1;

const int initalPressedDelay = 200;
const int minDelay = 10;
const int delayReducer = 15;
int pressedDelayUp = initalPressedDelay;
int pressedDelayDown = initalPressedDelay;


// --------------------- END OF Control ---------------------

// --------------------- DS1882 Functions ---------------------

void ds1882write(int8_t ch0, int8_t ch1) {
  // update potentiometers
  Wire.beginTransmission(ds1882Addr);
  Wire.write((ch0 & bmValue) | regPot0); // Pot 0
  Wire.write((ch1 & bmValue) | regPot1); // Pot 1
  wireState = Wire.endTransmission();
  Serial.print("Transfered State: ");
  Serial.println(wireState);
}

int8_t ds1882init() {
  Serial.println("\n---> Initialize DS1882");
  
  int8_t dsState[3];
  
  // begin I2C transaction
  Wire.begin();
  
  // request three status bytes from DS1882
  Wire.requestFrom(ds1882Addr, 3);

  for(int i = 0; Wire.available() && i < 3; i++) dsState[i] = Wire.read() & bmValue;
  // dsState[0] = Pot0 value, dsState[1] = Pot1 value, dsState[2] = configuration

  Serial.print("Current Config: ");
  Serial.println(dsState[2], HEX);

  Serial.print("Config to use: ");
  Serial.println(ds1882Conf, HEX);

  Serial.print("Code to set Config: ");
  Serial.println(ds1882Conf | regConfig, HEX);
  
  if (dsState[2] != ds1882Conf) {
    // the configuration is not what it should be, so we send the right one
    Wire.beginTransmission(ds1882Addr);
    Wire.write(ds1882Conf | regConfig);
    wireState = Wire.endTransmission();
  }
  Serial.println(" ... DONE");
  return dsState[0];
}

// --------------------- END OF DS1882 Functions  ---------------------

// --------------------- LCD Functions ---------------------

void initLCD(){
    // Initiate the LCD:
  lcd.init();
  lcd.createChar(0, zero);
  lcd.createChar(1, one);
  lcd.createChar(2, two);
  lcd.createChar(3, three);
  lcd.createChar(4, four);
  lcd.createChar(5, five);
  lcd.backlight();

  lcd.setCursor(3,0);
  lcd.print("  Volume     ");
  setInitialValueProgressBar(volume);
}

// --------------------- END OF LCD Functions  ---------------------

// ----------------------------------------------------------------------------------------------------------------------
// ---                                        Arduino SETUP AND LOOP                                                  ---
// ----------------------------------------------------------------------------------------------------------------------

void setup() {
  //Wire.setClock(400000);
  Serial.begin(9600); // The baudrate of Serial monitor is set in 9600
  Serial.println("\nVolume Control DS1882");

  // initialize the DS1882
  currentPosition = ds1882init();
  Serial.print("Setup State:");
  Serial.println(currentPosition);
  
  //initlialize Buttons
  pinMode(upButtonPin, INPUT);
  pinMode(downButtonPin, INPUT);
  pinMode(controlButtonPin, INPUT);

  //initlialize LCD
  initLCD();
}

void loop() {
  
  int8_t newPosition = 50;
  
  if (newPosition != currentPosition) {
    currentPosition = constrain(newPosition, minPosition, maxPosition);
    ds1882write(currentPosition, currentPosition);
  }

  int upIsPressed = digitalRead(upButtonPin);
  int downIsPressed = digitalRead(downButtonPin);
  int controlIsPressed = digitalRead(controlButtonPin);

  //check if control button is pressed (only once, keep pressing will be ignored)
  if(controlIsPressed){
    if(control_notKeepedPressed){
      control_notKeepedPressed = false;
      //change controlType
      controlType++;
      if ((sizeof(controlTypes)-1)<=controlType){
        controlType=0;
      }
    }
  }else{
    control_notKeepedPressed = true;
  }

  bool justchanged = lastControlState!=controlTypes[controlType];

  //check for what to control
  switch (controlTypes[controlType]) {
    case 'v':
      if(justchanged){
        lcd.setCursor(3,0);
        lcd.print("  Volume     ");
        setInitialValueProgressBar(volume);
      }
      
      if(downIsPressed & !upIsPressed){
        decreaseVolume();
        delay(pressedDelayDown);
        if(pressedDelayDown>(minDelay)){
          pressedDelayDown -= delayReducer;
        }
      }else{
        pressedDelayDown = initalPressedDelay;
      }

      if(upIsPressed & !downIsPressed){
        increasVolume();
        delay(pressedDelayUp);
        if(pressedDelayUp>(minDelay+30)){
          pressedDelayUp -= delayReducer;
        }
      }else{
        pressedDelayUp = initalPressedDelay;
      }

      break;
    case 'b':
      if(justchanged){
        lcd.setCursor(3,0);
        lcd.print("Bass Boost    ");
        setInitialValueProgressBar(bassVolume*5);
      }
      if(downIsPressed & !upIsPressed){
        decreaseBassVolume();
        delay(200);
      }

      if(upIsPressed & !downIsPressed){
        increasBassVolume();
        delay(200);
      }

      break;
    default:
      lcd.setCursor(0,0);
      lcd.print("Error: controltype    ");
    break;
  }
  lastControlState = controlTypes[controlType];
}

// ----------------------------------------------------------------------------------------------------------------------
// ---                                        END OF Arduino SETUP AND LOOP                                                  ---
// ----------------------------------------------------------------------------------------------------------------------

// --------------------- Control Functions ---------------------
void decreaseBassVolume(){
  if(bassVolume > 0){
  
    bassVolume -= 1;
    if(bassVolume <= 0){
      bassVolume = 0;
    }
    decreseValueProgressBar(5,bassVolume+1,0);
  }
}

void increasBassVolume(){
  if(bassVolume < 16){
  
    bassVolume += 1;
    if(bassVolume >= 16){
      bassVolume = 16;
    }
  increaseValueProgressBar(5,bassVolume-1,0);
  }
}

void decreaseVolume(){
  if(volume > 0){
    int currentCursor = (int)volume/5; //= 8 -> 7 in array
    int singleSteps = volume%5;
  
    volume -= 1;
    if(volume <= 0){
      volume = 0;
    }
    decreseValueProgressBar(1,currentCursor,singleSteps);
  }
  setVolumeDigiPot();
}

void setVolumeDigiPot(){
  //write to digitpot (63 = mute 0 = Laut)
  double percentageVolume = (double)volume/80;
  int digiVolume = (int)(63 - (63*percentageVolume));
  ds1882write(digiVolume, digiVolume);
}

void increasVolume(){
  if(volume < 80){
    int currentCursor = (int)volume/5; //= 8 -> 7 in array
    int singleSteps = volume%5;
  
    volume += 1;
    if(volume >= 80){
      volume = 80;
    }
  increaseValueProgressBar(1,currentCursor,singleSteps);
  }
  setVolumeDigiPot();
}

void setInitialValueProgressBar(int steps)
{
  //clean display
  for(int i=0; i <= 15; i++){
    lcd.setCursor(i,lineToPrintVolumeOn);
    lcd.write(0);
  }
  
  int fullSteps = (int)steps/5;
  int singeSteps = (int)steps%5;
  lcd.createChar(0, zero);
  int currentStep = 0;
  
  //setFullSteps
  for(int i=0; i <= fullSteps; i++){
    lcd.setCursor(i,lineToPrintVolumeOn);
    lcd.write(5);
    currentStep = i;
  }

  lcd.setCursor(currentStep,lineToPrintVolumeOn);
  lcd.write(singeSteps);
  
}

void decreseValueProgressBar(int negativeSteps,int currentCursor,int singleSteps)
{ 
    //if there are single steps, move one step ahead
    if (singleSteps !=0){
      currentCursor++;
      
     if(negativeSteps<singleSteps){
     //if the negative steps are less then the singlesteps on the cursor
        int newValue = singleSteps-negativeSteps;
        lcd.setCursor(currentCursor-1,lineToPrintVolumeOn);
        lcd.write(newValue);
        //negativeSteps left = 0
        negativeSteps = 0;
      }else{
        //if there are more negativesteps than single steps, just zero the current step
        negativeSteps -= singleSteps;
        lcd.setCursor(currentCursor-1,lineToPrintVolumeOn);
        lcd.write(0);
      }
      //set the current cursor back
      currentCursor--;
    }
    
    //check how many you can fully reduce
    int fullSteps = (int)negativeSteps/5;
    if (fullSteps>0){
      for(int i=0; i < fullSteps; i++){
        lcd.setCursor(currentCursor-1,lineToPrintVolumeOn);
        lcd.write(0);
        currentCursor--;
        negativeSteps -= 5;
      }
    }
    
    //check if single steps left
    if(negativeSteps>0){
      int newValue = 5-negativeSteps;
      lcd.setCursor(currentCursor-1,lineToPrintVolumeOn);
      lcd.write(newValue);
      negativeSteps=0;
    }
}


void increaseValueProgressBar(int postiveSteps,int currentCursor,int singleSteps)
{ 
    currentCursor++;
    //if there are single steps, move one step ahead
    if (singleSteps !=0){
      
     if(postiveSteps<=(5-singleSteps)){
     //if the negative steps are less then the singlesteps on the cursor
        int newValue = singleSteps+postiveSteps;
        lcd.setCursor(currentCursor-1,lineToPrintVolumeOn);
        lcd.write(newValue);
        //postiveSteps left = 0
        postiveSteps = 0;
      }else{
        //if there are more postive steps than single steps, just fill the current step
        postiveSteps -= (5-singleSteps);
        lcd.setCursor(currentCursor-1,lineToPrintVolumeOn);
        lcd.write(5);
        currentCursor++;
      }
    }
    
    //check how many you can fully reduce
    int fullSteps = (int)postiveSteps/5;
    if (fullSteps>0){
      for(int i=0; i < fullSteps; i++){
        lcd.setCursor(currentCursor-1,lineToPrintVolumeOn);
        lcd.write(5);
        currentCursor++;
        postiveSteps -= 5;
      }
    }
    
    //check if single steps left
    if(postiveSteps>0){
      int newValue = postiveSteps;
      lcd.setCursor(currentCursor-1,lineToPrintVolumeOn);
      lcd.write(newValue);
      postiveSteps=0;
    }
}
// --------------------- END OF Control Functions  ---------------------
