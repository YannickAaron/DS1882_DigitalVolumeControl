#include <Wire.h> // Library for I2C communication

// DS1882 I2C address
#define ds1882Addr      (0x28) // default 7-bit address, A0..A2 are LOW

// DS1882 registers addresses and bit masks and values
#define bmReg           (0xC0) // Register address bits 
#define regPot0         (0x00) // Potentiometer 0 register
#define regPot1         (0x40) // Potentiometer 1 register
#define regConfig       (0x80) // Configuration register
#define bmValue         (0x3F) // Register value bits 
#define bvVolatile      (0x04) // Non-volatile memory disable
#define bvZeroCrossing  (0x02) // Zero crossing detection enable
#define bvCompatibility (0x01) // Enable DS1808 compatibility mode (33 positions + mute)
#define ds1882Conf      (bvZeroCrossing) // non-volatile, zero-crossing enabled, 64 positions

// define max and min volume limits
#define minPosition (0)  // 0dB - no attenuation
#define maxPosition (63) // mute


int8_t currentPosition;
byte error;

void setup() {
  Serial.begin(9600);
  Serial.println("Start Test");
  
  // initialize the DS1882
  Wire.begin();
  Wire.beginTransmission(ds1882Addr);
  //Wire.write(ds1882Conf | regConfig);
  error = Wire.endTransmission();

  Serial.println("Set Pot");

  ds1882write(0, 0);
}

void loop() {
  // put your main code here, to run repeatedly
  Serial.println(error);
  delay(3000); // wait 5 seconds for the next I2C scan

  //ds1882write(0, 0);
}

int8_t ds1882init() {
  int8_t dsState[3];
  
  // begin I2C transaction
  Wire.begin();
  
  // request three status bytes from DS1882
  Wire.requestFrom(ds1882Addr, 3);

  for(int i = 0; Wire.available() && i < 3; i++) dsState[i] = Wire.read() & bmValue;
  // dsState[0] = Pot0 value, dsState[1] = Pot1 value, dsState[2] = configuration

  if (dsState[2] != ds1882Conf) {
    // the configuration is not what it should be, so we send the right one
    Wire.beginTransmission(ds1882Addr);
    Wire.write(ds1882Conf | regConfig);
    Wire.endTransmission();
  }
  return dsState[0];
}

void ds1882write(int8_t ch0, int8_t ch1) {
  // update potentiometers
  Wire.beginTransmission(ds1882Addr);
  Wire.write((ch0 & bmValue) | regPot0); // Pot 0
  Wire.write((ch1 & bmValue) | regPot1); // Pot 1
  error = Wire.endTransmission();
}
