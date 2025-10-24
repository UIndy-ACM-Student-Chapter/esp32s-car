//Simple Time
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";

unsigned long getNTPEpochTime()
{
  struct tm timeinfo;
  time_t now;
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return 0;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  
  //time(&now);
  now = mktime(&timeinfo);
  Serial.println("Seconds since the Epoch:" + String(now) + "\n");
  return now;
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  getNTPEpochTime();
}
