# esp32-s-car
A ESP32-S Controlling Car

## Authorship
The code is originally from this Github Repo https://github.com/ISSI-Lab/esp32-accelerometer

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

### How to use?
- In Arduino IDE, create a new project (sketch) called "esp32s-car"
    - This new sketch will have a new entry file (.ino file) call "esp32s-car.ino"
    - You need to create a new sketch and then save as "esp32s-car"
    - The Arduino IDE has default folder for all the project created under IDE
        - MacOS: /Users/{username}/Documents/Arduino/
- Download the repo file in zip format
- Unzip the downloaded file
- Inside the folder, copy all the files to Arduino's esp32s-car folder to replace everythings in the folder
- Reopen the esp32s-car project by clicking the esp32s-car.ino file you can see all the files loaded in the Arduino IDE
- You can compile and upload to a ESP32s NodeMCU


 
