#ifndef _CONSTANT_H
#define _CONSTANT_H

#define DEVICE_MODE     1  // 0=debug;1=production   
#define ENABLE_SOCKETIO 0  // 0=NOT;1=enabled
#define DEBUG           0  // 0=debug;1=production 
#define PRODUCTION      0  // 0=debug;1=production 
#define REMOTE_LOG      1
#define SERIAL_LOG      0
#define YES             1  //YES= true
#define NO              0  //NO = false

#define ESP_ON_STA_MODE 0 
#define ESP_ON_AP_MODE  1

// The following defines internet and server status
#define INTERNET_ALL_ON      1 //Has all the internet server accesses
#define WIFI_NO_TIME_SERVER  2 //Has wifi connection has data server but no time server
#define WIFI_NO_DATA_SERVER  3 //Has wifi connection has time server but no data server
#define WIFI_NO_INTERNET     4 //Has wifi connection but no internet (therefore no servers)
#define NO_WIFI              5 //NO wifi connection

// The following defines the SD Module status
#define HAS_SD          1
#define NO_SD           0

// Decide if level is on for manually ajusting leveling:
// 1=true and 0=false; this is used for manually leveling x, y, z and turn off loop 
// LEVEL_ON is skipping loop for leveling and LEVEL_OFF cannot skip loops
#define LEVEL_ON        1
#define LEVEL_OFF       0


#define HARD_RESET_TIMEOUT  4000  // if button is pressed for at least 5 seconds, reset



#endif  // _CONSTANT_H
