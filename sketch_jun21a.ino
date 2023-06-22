#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Wire.h" // This library allows you to communicate with I2C devices.

int motor1 = 3;
int motor2 = 5;
int motor3 = 9;
int motor4 = 10;

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data

char tmp_str[7]; // temporary variable used in convert function

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  i = i/100;
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

RF24 radio(7,8);
const byte address[6] = "100000";
int adjust[4] = {0, 0, 0, 0};

void setup() {
  // put your setup code here, to run once:
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(motor3, OUTPUT);
  pinMode(motor4, OUTPUT);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();

  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  turnOnMotor(0, adjust);
}

void turnOnMotor(int power, int adjust[]) {
  
  analogWrite(motor1, power-adjust[0]);
  analogWrite(motor2, power-adjust[1]);
  analogWrite(motor3, power-adjust[2]);
  analogWrite(motor4, power-adjust[3]);
}


int power = 0;
char text[32] = "";

void loop() {
  if(radio.available()) {
    radio.read(&text, sizeof(text));
  }
  if(text[0] == 'U' && power < 255) {
    power++;
  } else if(text[0] == 'D' && power > 0) {
    power--;
  } else if(text[0] == '2' && power < 255-10) {
    power += 10;
  } else if(text[0] == '8' && power > 10) {
    power -= 10;
  } else if(text[0] == 'C'
  ) {
    power = 0;
  }
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers

  
  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
  
  // print out data
  Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
  Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
  Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);
  Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
  Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
  Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
  Serial.println();

  //when x is positive, motor 4 gets weaker
  //when x is negative, motor 2 gets weaker
  //when y is positive, motor 1 gets weaker
  //when y is negative, motor 3 gets weaker
  //stable y is between -6 and -8
  //stable x is between  5 and 7

  if (accelerometer_x > 10) {
    adjust[3] = (accelerometer_x/100-10)/2;
  }
  if (accelerometer_x < 2) {
    adjust[1] = (2-accelerometer_x/100)/2;
  }
  if (accelerometer_y > -2) {
    adjust[0] = (accelerometer_y/100+2)/2;
  }
  if (accelerometer_y < -12) {
    adjust[2] = (-12-accelerometer_y/100)/2;
  }
  turnOnMotor(power, adjust);
  text[0] = ' ';
}
