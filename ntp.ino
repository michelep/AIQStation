// AIQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

#include <NtpClientLib.h>

// ************************************
// processSyncEvent()
//
// manage NTP sync events and warn in case of error
// ************************************
void processSyncEvent(NTPSyncEvent_t ntpEvent) {
  DEBUG_PRINT("[DEBUG] processSyncEvent() ");
  if (ntpEvent) {
    DEBUG_PRINT("[NTP] Time Sync error: ");
    if (ntpEvent == noResponse)
      DEBUG_PRINT("[NTP] NTP server not reachable");
    else if (ntpEvent == invalidAddress)
      DEBUG_PRINT("[NTP] Invalid NTP server address");
  } else {
    DEBUG_PRINT("[NTP] Got NTP time: "+String(NTP.getTimeDateString(NTP.getLastNTPSync ())));
  }
}
