# esp32-s-car
A ESP32-S Controlling Car

## Component
### Board and Sensor
This project uses an ADXL345 accelerometer and a ESP32S Nodemcu. We use the Arduino library on the ESP32S Nodemcu.

- https://techexplorations.com/guides/esp32/begin/digitalout/
- https://www.electronicwings.com/esp32/adxl345-accelerometer-interfacing-with-esp32

## Before Usage
### Arduino IDE Installation
- https://www.arduino.cc/en/software/

### Test ESP32S Nodemcu
- Connect your ESP32S to your computer
- Open your Arduino IDE
- Create a new project (sketch)
- In the setup() function, write the "Hello World" code.
- Compile and Upload
- See your Serial Monitor outputs "Hello World"

*Troubleshoot:* You might need to install USB chip driver. This depends on which USB chip the board uses. For example, CP210x USB is commonly used for Windows. 

### Required External Libraries
You need to import the library's zip files to get the support. The following two libraries are used to drive the sensors.
- https://github.com/adafruit/Adafruit_Sensor
- https://github.com/adafruit/Adafruit_ADXL345



