//Define All the Pins
#define PIN_FLASH      0  //use for Hard Reset
//#define LED1           5  // D1 GPIO5 another pin for 3.3v
#define LED1           36

//Sensor Part
//D5 (14) SCL  SCL
//D6 (12) MISO SDIO SDO
//D7 (13) MOSI SDA
//D8 (10) SD3   CS
//#define ADXL345_CS    10  // SD3 GPIO10
//#define ADXL345_SDO   12  // D6  GPIO12 #This is not used in the code but for records
//#define ADXL345_SDA   13  // D7  GPIO13 #This is not used in the code but for records
//#define ADXL345_SCL   14  // D5  GPIO14 #This is not used in the code but for records

//ESP32S NODEMCU
//SPI   MOSI    MISO    SCLK    CS
//VSPI  GPIO 23 GPIO 19 GPIO 18 GPIO 5
//HSPI  GPIO 13 GPIO 12 GPIO 14 GPIO 15

//SD card reader for VSPI (VSPI is the default for SD.h)
#define SD_CS    5   // GPIO5
#define SD_DO    19  // GPIO19  #This is not used in the code but for records
#define SD_DI    23  // GPIO23 #This is not used in the code but for records
#define SD_CLK   18  // GPIO18 #This is not used in the code but for records

/*
 * CS = Chip select
 * DI = Data In (MOSI)
 * VCC
 * SCK = Serial Clock
 * GND
 * DO = Data Out (MISO)
 * CD (not sure what it stands for, but this pin isn't used when connecting to an Arduino. I think it's used for faster transfers.)
 */

//SPI.begin(SCK, MISO, MOSI, SS);

//The circuit:
// SD card attached to SPI bus as follows:
// MOSI - pin 11 on Arduino Nodemcu D7
// MISO - pin 12 on Arduino Nodemcu D6
// SCK -  pin 13 on Arduino Nodemcu D5
// CS - depends on your SD card shield or module. Pin 4 Nodemcu D2



//#define SD_CS   4 //Arduino Ethernet shield: pin 4
