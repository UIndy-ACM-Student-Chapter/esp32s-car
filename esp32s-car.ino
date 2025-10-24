//Add the Adafruit_ADXL345_U library so we can communicate with the ADXL345 sensor
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <WiFi.h>
#include "time.h"
#include "sntp.h"
#include <WebServer.h>
#include <EEPROM.h>
#include <base64.h>

#include <Arduino.h> //adding this because we remove Utility.h
//#include "SD_Op.h"
#include "Constant.h"
#include "Utility.h"
#include "Server_Page.h"
#include "Device.h"
#include "Pin.h"
#include "ADXL345_Display.h"
#include "NTP_Time.h"

//Variable Declarations
WebServer server(80);
String version = "1.00.00.00";
String st;
String responseJson;
String content;
String deviceUuidStr;
int espMode; 
int sdOnlyMode = 0; //SD only in AP only
byte mac[6];
Device device;

//About Hard Reset Setting
int hardsetPressFlag  = 0;
ulong buttonPressTime = 0;

//Socket Related Varibles
char host[] = "rtiot.zygozu.com";

//Sensor Related
//This buffer will hold values read from the ADXL345 registers.
unsigned char values[10];
char output[20];
//These variables will be used to hold the x,y and z axis accelerometer values.
int x,y,z;
double xg, yg, zg;
int levelState = LEVEL_OFF; 

//Time
float startLoopTime = 0.0;
float lastSampleTime = 0.0;
int prevTimestamp = 0;
float currTimestamp = 0;
float startServerTimestamp = 0;
float startLocalTimestamp = 0;
float offsetStartServerTimestamp = 0;
int sampleRate = 100; //milli-second
int isFirstWrite = 0;

//Bufffer
String bufferMeasure;
String bufferMeasureCSV;
int bufferCounter = 0;
int bufferCountSize = 10;

//File dataFile;  //@NOTE: possible remove
char SDFilename[] = "/acc_data.csv";

//Default hardcoded hmac key
String defaultHmacKey = "rTi0T#Kv%";

uint32_t chipId = 0;

void setup() {
  Device::begin();
  log("Startup-"+ String(millis()), SERIAL_LOG, PRODUCTION);

  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  Serial.print("ESP32 Chip ID: "); Serial.println(chipId);
  
  initialized();  

  String macStr = getMacString(0);
  deviceUuidStr = Device::uuid("", "-" + macStr);
  (Device::readWebHost()).toCharArray(host, 64);
  log ("host1" + Device::readWebHost(), SERIAL_LOG, PRODUCTION);
  //host = (Device::readWebHost()).c_str();
  
  log("uuid:" + deviceUuidStr, SERIAL_LOG, PRODUCTION);
  log("host:" + String(host), SERIAL_LOG, PRODUCTION);

  // if (Device::readSdOnly().toInt() == 1) {
  //     log("save ...sd only mode:" + sdOnlyMode, SERIAL_LOG, PRODUCTION);
  //     sdOnlyMode = 1;
  //     Device::saveSdOnly(String(sdOnlyMode));
  // }
  //read buffer and sample rate
  if (Device::readBufferSize().toInt() > 10) {
      bufferCountSize = 10;
      Device::saveBufferSize("10");
  } else {
      bufferCountSize = Device::readBufferSize().toInt();
  }
  sampleRate = Device::readSampleRate().toInt();

  //@NOTE: temp defined for tesitng
  // read eeprom for ssid and pass
  //String esid  = Device::readSSID();
  //String epass = Device::readPwd();

  String esid  = "ardlink";
  String epass = "2139851899";

  log(Device::readSSID(), SERIAL_LOG, DEBUG);
  log(Device::readPwd(), SERIAL_LOG, DEBUG);

  //Launch Web Based on STA or AP mode
  if ( esid.length() > 1 ) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(esid.c_str(), epass.c_str());
      if (tryConnect()) {
          espMode = ESP_ON_STA_MODE;
          Serial.println("Before launch Web Server start connection...");
          //get the start server timestamp
          delay(10);
          startServerTimestamp = getServerTimestamp();
          startLocalTimestamp = millis();
          log("SERVER Start TS:" + String(startServerTimestamp), SERIAL_LOG, PRODUCTION);
          log("LOCAL Start TS:" + String(startLocalTimestamp), SERIAL_LOG, PRODUCTION);
          
          if (startServerTimestamp > 0) {
        	    offsetStartServerTimestamp = startServerTimestamp*1.0 - startLocalTimestamp*1.0;
              log("Offset TS:" + String(offsetStartServerTimestamp), SERIAL_LOG, PRODUCTION);
          }
          launchWeb(espMode);
          return;
      }
  }
  //Setup AP mode (not return above)
  espMode = ESP_ON_AP_MODE;
  setupAP();

}

void initialized() {
   levelState = LEVEL_OFF;
   
   //@NOTE: initADXL() is from ADXL345_Display.h
   initADXL();

   //@NOTE: initSD() is from SD_Op.h
   //initSD();
   //initSDFile();

   //Set NTP server time
   sntp_set_time_sync_notification_cb( timeavailable );
   configTime(0, 0, ntpServer1, ntpServer2);
   //configTzTime(time_zone, ntpServer1, ntpServer2);
}

// void initSDFile() {
//    deleteFile(SD, "/acc_data.bak");
//    renameFile(SD, SDFilename, "/acc_data.bak");
//    appendFile(SD, SDFilename, "xg, yg, zg");
   
// }

bool tryConnect(void) {
  int c = 0;
  log("Waiting for Wifi to connect", SERIAL_LOG, DEBUG);
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    delay(500);
    log("WIFI Status:" + String(WiFi.status()), SERIAL_LOG, DEBUG);
    c++;
  }
  log("Connect timed out, opening AP", SERIAL_LOG, DEBUG);
  return false;
}

void launchWeb(int espMode) {
  SetWebPages(espMode);
  server.begin();
  log("Web Server started - IP Address: " + getIpAddress(espMode), REMOTE_LOG, PRODUCTION);
}

void setupAP(void) {
  scanNetwork();
  String postMac = getMacString(espMode);
  String ssid = "LAWIFI-" + postMac;
  //String password = "SDWIFI-" + postMac;

  delay(100);
  //WiFi.softAP(ssid, passphrase, 6);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid.c_str());
  WiFi.disconnect();
  log("softap", SERIAL_LOG, DEBUG);
  launchWeb(espMode);
}

void SetWebPages(int webtype) {
  if ( webtype == 1 ) {
      OnNetworkConfigWeb();
  } else if (webtype == 0) {
      OnRunWeb();
  }
}

void OnRunWeb() {
  //Root
  server.on("/", []() {
      int lts = getServerTimestamp();
      if (Device::isLogin(lts)) {
          content = webHomePage();
          server.send(200, "text/html", content);
      } else {
          content = loginPage();
          server.send(200, "text/html", content);
      }
  });

  //Server related variables
  server.on("/conf", []() {
      int lts = getServerTimestamp();
      if (Device::isLogin(lts)) {
          String webUserId = server.arg("wuid");
          String webApiKey = server.arg("wkey");
          String webSecret = server.arg("wsec");
          String hostUrl = server.arg("wurl");

          String retMsg = "";
          //save variables
          if (webUserId.length() > 0) {
              Device::saveWebUid(webUserId);
              retMsg = retMsg + "Cloud Accound is saved. ";
          } else {
              retMsg = retMsg + "Cloud Accound is NOT saved. ";
          }

          if (webApiKey.length() > 0) {
              Device::saveApiKey(webApiKey);
              retMsg = retMsg + "Cloud API Key is saved. ";
          } else {
              retMsg = retMsg + "Cloud API Key is NOT saved. ";
          }

          if (webSecret.length() > 0) {
              Device::saveEncryptSecret(webSecret);
              retMsg = retMsg + "Cloud Secret Key is saved. ";
          } else {
        	  Device::saveEncryptSecret("");
              retMsg = retMsg + "Cloud Secret Key is NOT saved. ";
          }

          if (hostUrl.length() > 0) {
              Device::saveWebHost(hostUrl);
              retMsg = retMsg + "API Host URL is saved. ";
          } else {
              retMsg = retMsg + "API Host URL is NOT saved. ";
          }

          //response
          if (retMsg != "") {
              serverJsonSend("success", retMsg, "");
          } else {
              serverJsonSend("error", "Nothing config", "");
          }
      } else {
          serverJsonSend("success", "Not Login - Variables are not saved", "");
      }

  });

    //Server related variables
  server.on("/dset", []() {
      int lts = getServerTimestamp();
      if (Device::isLogin(lts)) {
          String deviceUid = server.arg("duid");
          String devicePwd = server.arg("dpwd");
          String devicePin = server.arg("dpin");

          String retMsg = "";
          //save variables
          if (deviceUid.length() > 0) {
              Device::saveDeviceUid(deviceUid);
              retMsg = retMsg + "Device User ID is saved. ";
          } else {
              retMsg = retMsg + "Device User ID is NOT saved. ";
          }

          if (devicePwd.length() > 0) {
              Device::saveDevicePwd(devicePwd);
              retMsg = retMsg + "User Password is saved. ";
          } else {
              retMsg = retMsg + "User Pasword is NOT saved. ";
          }

          if (devicePin.length() > 0) {
              String md5PinStr = toMD5(devicePin);
              Device::savePinNumber(md5PinStr);
              retMsg = retMsg + "Device Pin Number is saved. ";
          } else {
              retMsg = retMsg + "Device Pin Number is NOT saved. ";
          }

          //response
          if (retMsg != "") {
              serverJsonSend("success", retMsg, "");
          } else {
              serverJsonSend("error", "Nothing config", "");
          }
      } else {
          serverJsonSend("success", "Not Login - Variables are not saved", "");
      }

  });

  //for ajax login check
  server.on("/login", []() {
      String userId = server.arg("uid");
      String userPwd = server.arg("pwd");
      String loginTime = String(getServerTimestamp());

      bool isLogined = Device::isAuth(userId, userPwd, loginTime);
      if (isLogined) {
          serverJsonSend("success", "t", "t");
      } else {
          serverJsonSend("success", "f", "f");
      }
  });

   //for ajax logout
  server.on("/logout", []() {
      Device::logout();
      serverJsonSend("success", "Login Out", getIpAddress(0));
  });

   //get sample rate
  server.on("/sr", []() {
      String savedSampleRate = Device::readSampleRate();
      if (!Device::isValidNumbers(savedSampleRate)) {
          serverJsonSend("success", "Use default sample rate", "100");
      } else {
          serverJsonSend("success", "Use sample rate", savedSampleRate);
      }
  });

   //set sample rate
  server.on("/setsr", []() {
      String srdata = server.arg("srdata");
      if (srdata.toInt() < 0 || srdata.toInt() > 9999) {
          serverJsonSend("error", "ERROR! Greater Than  9999 or Less Than 0", srdata);
      } else if (!Device::isValidNumbers(srdata)){
          serverJsonSend("error", "ERROR! Wrong Data Type", srdata);
      } else {
          Device::saveSampleRate(srdata);
          sampleRate = srdata.toInt();
          serverJsonSend("success", "Sample rate saved!", srdata);
      }
  });

     //get buffer size
  server.on("/buf", []() {
      String savedBufferSize = Device::readBufferSize();
      if (!Device::isValidNumbers(savedBufferSize)) {
          serverJsonSend("success", "Use default buffer size", "10");
      } else {    
          serverJsonSend("success", "Get buffer size", savedBufferSize);
      }
  });

   //set buffer size
  server.on("/setbuf", []() {
      String bufdata = server.arg("bufdata");
      if (bufdata.toInt() < 0 || bufdata.toInt() > 10) {
          serverJsonSend("error", "ERROR! Greater Than 10 or Less Than 0", bufdata);
      } else if (!Device::isValidNumbers(bufdata)){
          serverJsonSend("error", "ERROR! Wrong Data Type", bufdata);
      } else {
          Device::saveBufferSize(bufdata);
          bufferCountSize = bufdata.toInt();
          serverJsonSend("success", "Buffer size saved!", bufdata);
      }
  });

  server.on("/lvl", []() {
      int lts = getServerTimestamp();
      if (Device::isLogin(lts)) {
          levelState = LEVEL_ON;
          accelerator();
          String mMeasure = "";
          //mMeasure = toBufferMeasure (xg, yg, zg, String(millis()), 0, ""); //remove Utility so removing this line
          
          serverDataObjSend("success", "Turn on manually fetching x, y, z acceleration. Turn off automatic fetching.", mMeasure);
          log("Fetching x, y, z acceleration", SERIAL_LOG, PRODUCTION);
          log(mMeasure, SERIAL_LOG, PRODUCTION);
      } else {
          content = needLoginPage();
          server.send(200, "text/html", content);
      }
  });

  server.on("/lvloff", []() {
      levelState = LEVEL_OFF;
      serverJsonSend("success", "Turn off manual fetching x, y, z acceleration. Turn on automatic fetching.", "");
      log("Turn off manual fetch x, y, z acceleration", SERIAL_LOG, PRODUCTION);
  });
  

  server.on("/savedata", []() {
      int lts = getServerTimestamp();
      if (Device::isLogin(lts)) {
          //read cloud account id, api key, device account
          String cloudUid = Device::readWebUid();
          String apkStr = Device::readApiKey();
          String dvUid = Device::readDeviceUid();
          String hUrl = Device::readWebHost();
          String secKey = Device::readEncryptSecret();
          String respJson = "{\"status\":\"success\", \"data\":{\"c_uid\":\"" + cloudUid + "\", \"apk\":\"" + apkStr + "\", \"dv_uid\":\"" + dvUid +"\", \"web_url\":\"" + hUrl +"\", \"secret\":\"" + secKey +"\"}}";
          log(respJson, SERIAL_LOG, DEBUG);
          server.send(200, "application/json", respJson);
      } else {
          content = needLoginPage();
          server.send(200, "text/html", content);
      }
  });
  
  //get ip address
  server.on("/ip", []() {
      int lts = getServerTimestamp();
      if (Device::isLogin(lts)) {
          serverJsonSend("success", "Get IP Address", getIpAddress(0));
      } else {
          content = needLoginPage();
          server.send(200, "text/html", content);
      }
  });
  //get device uuid
  server.on("/d_id", deviceIdResponse);

  //actions
  server.on("/reboot", rebootResponse);
  server.on("/reset", reset);

  server.on("/clear", []() {
      int lts = getServerTimestamp();
      if (Device::isLogin(lts)) {
          clearResponse();
      } else {
          content = needLoginPage();
          server.send(200, "text/html", content);
      }
   });

   server.on("/ver", []() {
      serverJsonSend("success", "Version:" + version , version);
      log("Version:" + version, SERIAL_LOG, PRODUCTION);
  });
}

void OnNetworkConfigWeb() {
  Serial.println("Startup Network Config Web Server");
  server.on("/", []() {
        content = ssidRootPage(st);
        server.send(200, "text/html", content);
  });

  server.on("/set", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
          Device::saveSSID(qsid);
          Device::savePwd(qpass);
          WiFi.begin(qsid.c_str(), qpass.c_str());
          content = ssidSetPage();
          server.send(200, "text/html", content);
      } else {
          content = ssidSetErrorPage(getIpAddress(1));
          server.send(404, "text/html", content);
      }
  });

  server.on("/ip", []() {
      if (WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
          serverJsonSend("success", "Get IP Address", getIpAddress(0));
          log("Local IP Address" + getIpAddress(0), SERIAL_LOG, DEBUG);
      }
  });

  server.on("/sdonlyon", []() {
      Device::saveSdOnly("1");
      sdOnlyMode = 1;
      serverJsonSend("success", "Turn on SD only mode", String(sdOnlyMode));
  });

  server.on("/sdonlyoff", []() {
       Device::saveSdOnly("0");
       sdOnlyMode = 0;
       serverJsonSend("success", "Turn off SD only mode", String(sdOnlyMode));
  });

  server.on("/sdonlystatus", []() {
       serverJsonSend("success", "Get SD only Status", String(sdOnlyMode));
  });

  server.on("/reboot", []() {
      delay(5000);
      Device::reboot();
  });

  server.on("/ver", []() {
      serverJsonSend("success", "Version:" + version , version);
      log("Version:" + version, SERIAL_LOG, PRODUCTION);
  });

     //get sample rate
  server.on("/sr", []() {
      String savedSampleRate = Device::readSampleRate();
      if (!Device::isValidNumbers(savedSampleRate)) {
          serverJsonSend("success", "Use default sample rate", "100");
      }
      serverJsonSend("success", "Get sample rate", savedSampleRate);
  });

   //set sample rate
  server.on("/setsr", []() {
      String srdata = server.arg("srdata");
      if (srdata.toInt() < 0 || srdata.toInt() > 9999) {
          serverJsonSend("error", "ERROR! Greater Than  9999 or Less Than 0", srdata);
      } else {
          Device::saveSampleRate(srdata);
          sampleRate = srdata.toInt();
          serverJsonSend("success", "Sample rate saved!", srdata);
      }
  });

   //get buffer size
  server.on("/buf", []() {
      String savedBufferSize = Device::readBufferSize();
      if (!Device::isValidNumbers(savedBufferSize)) {
          serverJsonSend("success", "Use default buffer size", "10");
      }
      serverJsonSend("success", "Get buffer size", savedBufferSize);
  });

   //set buffer size
  server.on("/setbuf", []() {
      String bufdata = server.arg("bufdata");
      if (bufdata.toInt() < 0 || bufdata.toInt() > 10) {
          serverJsonSend("error", "ERROR! Greater Than 10 or Less Than 0", bufdata);
      } else {
          Device::saveBufferSize(bufdata);
          bufferCountSize = bufdata.toInt();
          serverJsonSend("success", "Buffer size saved!", bufdata);
      }
  });

}

String getIpAddress(int type) {
   IPAddress ip = WiFi.localIP();
   if (type != 0) {
       ip = WiFi.softAPIP();
   }
   return String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
}

String getMacString(int type) {
   if (type == 0) {
       WiFi.macAddress(mac);
   } else {
       WiFi.softAPmacAddress(mac);
   }
   String macStr = String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
   return macStr;
}


void rebootResponse() {
    serverJsonSend("success", "Start rebooting now", "");
    delay(3000);
	  Device::reboot();
}

void deviceIdResponse() {
    serverJsonSend("success", "Get Device ID", deviceUuidStr);
}

void clearResponse() {
    Device::clearAll();
    serverJsonSend("success", "Clearing all successfully", "");
}


//void fastFlashOne(int times) {
//    int i = 0;
//    while (i <= times) {
//      digitalWrite(LED1, HIGH);
//      delay(500);
//      digitalWrite(LED1, LOW);
//      delay(500);
//      i++;
//    }
//}

String scanNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if (n == 0) {
    log("No networks found", SERIAL_LOG, DEBUG);
  } else {
    st = "<ul class='dropdown-menu'>";
    for (int i = 0; i < n; i++) {
      // Print SSID and RSSI for each network found
      st = st + "<li><a href='#'>" + WiFi.SSID(i) + "</a></li>";
    }
    st += "</ul>";
  }
  return st;
}

//@NOTE: check
void reset() {
   //wipeout connection ssid/password in the flash for testing
   log("Wipe out connection...", SERIAL_LOG, PRODUCTION);
   WiFi.disconnect(true);
   WiFi.softAPdisconnect();
   Device::clearSsid();
   Device::clearPwd();
   delay(1000);
   Device::reboot();
}

void serverJsonSend(String status, String message, String data) {
	 responseJson = "{\"status\":\"" + status + "\", \"msg\":\"" + message + "\", \"data\":\"" + data + "\"}";
	 server.send(200, "application/json", responseJson);
}

void serverDataObjSend(String status, String message, String data) {
   responseJson = "{\"status\":\"" + status + "\", \"msg\":\"" + message + "\", \"data\":" + data + "}";
   server.send(200, "application/json", responseJson);
}

void log(String message, int isRemote, int logMode) {
   int isLog = 0;
   if (DEVICE_MODE != PRODUCTION) {
       isLog = 1;
   } else if (logMode == PRODUCTION) {
       isLog = 1;
   }
   if (isLog == 1) {
       Serial.println(message);
	     if (isRemote == 1) {
		       String msgData = "{\"log\":\"" + message + "\"}";
           delay(6000);
		       sendHttpRequest("/rtapp/log", msgData);
	     }
   }
}

String sendHttpRequest(String path, String data) {

   String jwtStr =  "";
   // Use WiFiClient class to create TCP connections
   WiFiClient client;
   const int httpPort = 80;
   if (!client.connect(host, httpPort)) {
	     Serial.println("Http connection failed");
       return "Http connection failed";
   }

   String expTimestamp = String(millis()*1.0);
   String timeType = "LTS";
   //User server time
   if (offsetStartServerTimestamp > 0) {
	     expTimestamp = String(millis()*1.0 + offsetStartServerTimestamp*1.0 + 100000.0);
	     timeType = "STS";
   }

   String payload = "{\"apk\":\"" + Device::readApiKey() + "\", \"tst\":\"" + timeType + "\", \"exp\":\"" + expTimestamp + "\", \"did\":\"" + deviceUuidStr + "\", \"uid\":\"" + Device::readWebUid() + "\"}";
   if (data != "") {
	     payload = "{\"apk\":\"" + Device::readApiKey() + "\", \"tst\":\"" + timeType + "\", \"exp\":\"" + expTimestamp + "\", \"did\":\"" + deviceUuidStr + "\", \"uid\":\"" + Device::readWebUid() + "\", \"dat\":" + data + "}";
   }
   Serial.println(payload);

   String jwtPayloadStr = base64::encode(payload);
   jwtPayloadStr.replace("\n", "");

   if (Device::readEncryptSecret() != "") {
       String b64SignStr = base64::encode(toHmacMD5(jwtStr, Device::readEncryptSecret()));
       b64SignStr.replace("\n", "");
       jwtStr = "eyJ0eXAiOiJKV1QiLCJhbGciOiJNRDUifQ==." + jwtPayloadStr + "." + b64SignStr;
   } else {
       String b64SignStr = base64::encode(toHmacMD5(jwtStr, defaultHmacKey));
       b64SignStr.replace("\n", "");
       jwtStr = "eyJ0eXAiOiJKV1QiLCJhbGciOiJNRDUifQ==." + jwtPayloadStr + "." + b64SignStr; 
   }

   //Method#1: POST with url parameters
//   String url = path + "?atok=" + urlencode(jwtStr);
//   log(url, SERIAL_LOG, PRODUCTION);
//
//   // This will send the request to the server
//   client.print("POST " + url + " HTTP/1.1\r\n" +
//                "Host: " + host + "\r\n" +
//                "Cache-Control: no-cache \r\n" +
//                "Content-Type: application/x-www-form-urlencoded \r\n" +    
//                "Connection: close\r\n\r\n");

   //Method#2: POST with POST form parameters
   String url = path;
   String postData = "atok=" + urlencode(jwtStr);

   log(postData, SERIAL_LOG, PRODUCTION);

   client.print("POST "+ url +" HTTP/1.1\r\n" +
                "Host: " + String(host) + "\r\n" +
                "Cache-Control: no-cache \r\n" +
                "Content-Type: application/x-www-form-urlencoded \r\n" +
                "Content-Length: " + postData.length() + "\r\n\r\n" +
                postData
               );

   unsigned long timeout = millis();
   while (client.available() == 0) {
          if (millis() - timeout > 5000) {
        	    log("Client Timeout !", SERIAL_LOG, PRODUCTION);
              client.stop();
              return "Client Timeout !";
          }
   }

   // Read all the lines of the reply from server and print them to Serial
   String line = "";
   while (client.available()){
          line = client.readStringUntil('\r');
   }
   //log("Web server return from " + String(host), SERIAL_LOG, PRODUCTION);
   log(line, SERIAL_LOG, PRODUCTION);
   return line;
}



//@NOTE: check
float getServerTimestamp() {
   getNTPEpochTime();
   return 0;
}

//The method is commented for not connecting web server
//float getServerTimestamp() {
//
//   WiFiClient client;
//   const int httpPort = 80;
//   if (!client.connect(host, httpPort)) {
//       log("Connection failed", SERIAL_LOG, PRODUCTION);
//       return 0;
//   }
//
//   // We now create a URI for the request
//   String url = "/rtapp/t?t=ts";
//
//   // This will send the request to the server
//   client.print(String("GET ") + url + " HTTP/1.1\r\n" +
//                "Host: " + host + "\r\n" +
//                "Connection: close\r\n\r\n");
//
//   unsigned long timeout = millis();
//   while (client.available() == 0) {
//          if (millis() - timeout > 5000) {
//              log("Client Timeout !", SERIAL_LOG, PRODUCTION);
//              client.stop();
//              return 0;
//          }
//   }
//
//   // Read all the lines of the reply from server and print them to Serial
//   String line = "";
//   while (client.available()){
//          line = client.readStringUntil('\r');
//   }
//   line.trim();
//   float n1 = line.toFloat();
//
//   log("THIS IS SERVER TS STR RETURN:" + line, SERIAL_LOG, PRODUCTION);
//   log("THIS IS SERVER TS RETURN:" + String(n1), SERIAL_LOG, PRODUCTION);
//   return line.toFloat();
//}


//@NOTE: Need to check
void detectDoHardset() {
   //Serial.println(String(digitalRead(PIN_FLASH)) + String(hardsetPressFlag) + String(buttonPressTime));
   if (digitalRead(PIN_FLASH) == 0 && hardsetPressFlag == 0) {
       hardsetPressFlag = 1;
       buttonPressTime = millis();
   } else if (digitalRead(PIN_FLASH) == 1) {
       hardsetPressFlag = 0;
       buttonPressTime = 0;
   } else if (digitalRead(PIN_FLASH) == 0 && hardsetPressFlag == 1) {
       if ((millis() - buttonPressTime) > HARD_RESET_TIMEOUT) {
           log(String(millis()), SERIAL_LOG, PRODUCTION);
           log("Start hardset - clear all", SERIAL_LOG, PRODUCTION);
           //fastFlashOne(5);
           Device::clearAll();
           Device::reboot();
       }
   }
}

//Sensor Part
int16_t tenBitTwosComplementToDecimal(uint16_t x) {
  boolean negative = (x & (1 << 9)) != 0;
  if(negative)
    return x | ~((1 << 10) - 1);
   return (int16_t)x;
}


void accelerator() {
  sensors_event_t event; 
  accel.getEvent(&event);

  xg = event.acceleration.x;
  yg = event.acceleration.y;
  zg = event.acceleration.z;
}


//LOOP function Here; loop interval is around 2-3 million second
void loop() {

   server.handleClient();
   int writeAcc = 0;
   if (espMode == ESP_ON_AP_MODE && sdOnlyMode == 1) {
       writeAcc = 1;
   }

   //request
   if (espMode == ESP_ON_STA_MODE && levelState != LEVEL_ON) {
       writeAcc = 1;
   }

   //@NOTE:for debugging - think about how to deal with AP Mode. print out but also write into SD?
   writeAcc = 1;

   if (writeAcc == 1) {
       if (isFirstWrite == 0) {
//           writeSD("#World_Start_Timestamp:" + String(startServerTimestamp) + " Device_Start_Timestamp:" +  String(startLocalTimestamp));
           isFirstWrite = 1;
       }

       float tempTimestamp = currTimestamp;
       currTimestamp = millis()*1.0;
         
       accelerator();
       String currTs = String(millis()*1.0 + offsetStartServerTimestamp*1.0);
       log("-->" + currTs, SERIAL_LOG, PRODUCTION);
       Serial.print((float)xg);
       Serial.print("g,");
       Serial.print((float)yg);
       Serial.print("g,");
       Serial.print((float)zg);
       Serial.println("g");

       //@NOTE: temp for testing
       //appendStrToFile(SD, SDFilename, bufferMeasureCSV);
    
        //first loop
       if (int(prevTimestamp) == 0 && int(tempTimestamp) == 0) {
           int(prevTimestamp) = int(currTimestamp);
       } 
       //after first loop
       if (tempTimestamp > 0) {
           prevTimestamp = tempTimestamp;
//           Serial.println("sample rate" + String(sampleRate));
//           Serial.println("delay: " + String(sampleRate - (currTimestamp - prevTimestamp)));
           if (sampleRate > (currTimestamp - prevTimestamp)) {
               delay(sampleRate - (currTimestamp - prevTimestamp));
           }
       }
           
       bufferCounter++;
       if (bufferCounter < bufferCountSize) {
           currTimestamp = millis();
           String saveCurrTimestamp = String(millis());
           int useServerTime = 0;
           //User server time
           if (offsetStartServerTimestamp > 0) {
        	     useServerTime = 1;
               saveCurrTimestamp = String(millis()*1.0 + offsetStartServerTimestamp*1.0);
           }

          log(">>>" + String(saveCurrTimestamp), SERIAL_LOG, PRODUCTION);
           
           if (espMode == 0) {
//               log(">>>" + String(saveCurrTimestamp), SERIAL_LOG, PRODUCTION);
               bufferMeasure = toBufferMeasure (xg, yg, zg, saveCurrTimestamp, useServerTime, bufferMeasure);
           }
           bufferMeasureCSV = toBufferMeasureCSV(xg, yg, zg, saveCurrTimestamp, useServerTime, bufferMeasureCSV);
           //get the update timestamp again
           //Serial.println(String(currTimestamp - prevTimestamp));
       } else if (bufferCounter == bufferCountSize) {
           if (espMode == 0) {
               sendHttpRequest("/rtapp/upv", "[" + bufferMeasure + "]");
               //Serial.println(bufferMeasure);
           }

           //appendStrToFile(SD, SDFilename, bufferMeasureCSV);
           
           //@NOTE: old method open the SD file and flush() the data into the file
//           dataFile.println(bufferMeasureCSV);
//           dataFile.flush();
           Serial.println(bufferMeasureCSV);

           bufferMeasure = "";
           bufferMeasureCSV = "";
           bufferCounter = 0;      
       }
   }


   //detect long press FLASH button
   detectDoHardset();

}
