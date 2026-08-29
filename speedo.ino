#include <TinyGPSPlus.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>

#define TFT_BL 27

// GPS wiring
#define GPS_RX 35
#define GPS_TX 25

// SD card CS
#define SD_CS 5

TinyGPSPlus gps;
TFT_eSPI tft = TFT_eSPI();
HardwareSerial GPSserial(1);

unsigned long lastScreenUpdate = 0;
unsigned long lastGPSData = 0;
unsigned long lastSDWrite = 0;

int lastSatCount = 0;
bool gpsLocked = false;
bool sdWorking = false;

// Cached UI values
String lastRxText = "";
String lastSatText = "";
String lastLockText = "";
String lastSpeedText = "";
String lastLatText = "";
String lastLonText = "";
String lastAltText = "";
String lastHdopText = "";
String lastSDText = "";

//-----------------------------------
void drawStaticUI() {
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setCursor(10, 10);
  tft.println("GPS RX:");

  tft.setCursor(10, 35);
  tft.println("GPS TX:");

  tft.setCursor(10, 60);
  tft.println("SATS:");

  tft.setCursor(10, 85);
  tft.println("GPS LOCK:");

  tft.setCursor(10, 110);
  tft.println("SD:");

  tft.setCursor(10, 240);
  tft.println("LAT:");

  tft.setCursor(10, 265);
  tft.println("LON:");

  tft.setCursor(10, 290);
  tft.println("ALT:");

  tft.setCursor(10, 315);
  tft.println("HDOP:");

  // Static TX label
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(140, 35);
  tft.println("GPIO25");
}

//-----------------------------------
void initSDCard() {
  Serial.println("Initializing SD...");

  if (SD.begin(SD_CS)) {
    Serial.println("SD READY");
    sdWorking = true;
  } else {
    Serial.println("SD FAILED");
    sdWorking = false;
  }
}

//-----------------------------------
void logGPSData() {
  if (!sdWorking) return;

  File file = SD.open("/gpslog.txt", FILE_APPEND);

  if (!file) {
    Serial.println("SD write failed");
    sdWorking = false;
    return;
  }

  file.print("Speed:");

  if (gps.speed.isValid()) {
    file.print(gps.speed.kmph(), 1);
  } else {
    file.print("0");
  }

  file.print(", Sats:");
  file.print(lastSatCount);

  file.print(", Lat:");
  if (gps.location.isValid()) {
    file.print(gps.location.lat(), 6);
  } else {
    file.print("NOFIX");
  }

  file.print(", Lon:");
  if (gps.location.isValid()) {
    file.print(gps.location.lng(), 6);
  } else {
    file.print("NOFIX");
  }

  file.print(", Alt:");
  if (gps.altitude.isValid()) {
    file.print(gps.altitude.meters());
  } else {
    file.print("NOFIX");
  }

  file.println();
  file.close();

  Serial.println("GPS logged to SD");
}

//-----------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("GPS SPEEDO START");

  GPSserial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);

  drawStaticUI();
  initSDCard();
}

//-----------------------------------
void loop() {

  //-----------------------------------
  // Read GPS continuously
  //-----------------------------------
  while (GPSserial.available()) {
    char c = GPSserial.read();

    Serial.write(c);
    gps.encode(c);

    lastGPSData = millis();
  }

  //-----------------------------------
  // Filter bad sat values
  //-----------------------------------
  if (gps.satellites.isValid()) {
    int sats = gps.satellites.value();

    if (sats >= 0 && sats <= 40) {
      lastSatCount = sats;
    }
  }

  //-----------------------------------
  // Better lock detection
  //-----------------------------------
  gpsLocked =
      gps.location.isValid() &&
      gps.location.age() < 5000;

  //-----------------------------------
  // SD logging every 5 sec
  //-----------------------------------
  if (millis() - lastSDWrite > 5000) {
    lastSDWrite = millis();
    logGPSData();
  }

  //-----------------------------------
  // Screen refresh
  //-----------------------------------
  if (millis() - lastScreenUpdate > 500) {
    lastScreenUpdate = millis();

    tft.setTextSize(2);

    //-----------------------------------
    // GPS RX
    //-----------------------------------
    String rxText =
      (millis() - lastGPSData < 3000) ?
      "ACTIVE" : "LOST";

    if (rxText != lastRxText) {
      tft.fillRect(140, 10, 180, 20, TFT_BLACK);
      tft.setCursor(140, 10);

      tft.setTextColor(
        rxText == "ACTIVE" ? TFT_GREEN : TFT_RED,
        TFT_BLACK
      );

      tft.println(rxText);
      lastRxText = rxText;
    }

    //-----------------------------------
    // SAT DISPLAY
    //-----------------------------------
    String satText;

    if (lastSatCount > 0) {
      satText = String(lastSatCount) + " TRACKED";
    } else {
      satText = "NONE";
    }

    if (satText != lastSatText) {
      tft.fillRect(140, 60, 180, 20, TFT_BLACK);
      tft.setCursor(140, 60);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.println(satText);
      lastSatText = satText;
    }

    //-----------------------------------
    // GPS LOCK
    //-----------------------------------
    String lockText =
      gpsLocked ? "YES" : "SEARCHING";

    if (lockText != lastLockText) {
      tft.fillRect(140, 85, 180, 20, TFT_BLACK);
      tft.setCursor(140, 85);

      tft.setTextColor(
        gpsLocked ? TFT_GREEN : TFT_YELLOW,
        TFT_BLACK
      );

      tft.println(lockText);
      lastLockText = lockText;
    }

    //-----------------------------------
    // SD STATUS
    //-----------------------------------
    String sdText =
      sdWorking ? "READY" : "FAIL";

    if (sdText != lastSDText) {
      tft.fillRect(140, 110, 180, 20, TFT_BLACK);
      tft.setCursor(140, 110);

      tft.setTextColor(
        sdWorking ? TFT_GREEN : TFT_RED,
        TFT_BLACK
      );

      tft.println(sdText);
      lastSDText = sdText;
    }

    //-----------------------------------
    // SPEED
    //-----------------------------------
    String speedText =
      gps.speed.isValid()
      ? String(gps.speed.kmph(), 1)
      : "0.0";

    if (speedText != lastSpeedText) {
      tft.fillRect(20, 140, 300, 70, TFT_BLACK);

      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextSize(5);
      tft.setCursor(20, 140);

      tft.print(speedText);
      tft.print(" km/h");

      lastSpeedText = speedText;
    }

    //-----------------------------------
    // LAT
    //-----------------------------------
    String latText =
      gps.location.isValid()
      ? String(gps.location.lat(), 6)
      : "NO FIX";

    if (latText != lastLatText) {
      tft.fillRect(80, 240, 220, 15, TFT_BLACK);
      tft.setTextSize(1);
      tft.setCursor(80, 240);
      tft.println(latText);
      lastLatText = latText;
    }

    //-----------------------------------
    // LON
    //-----------------------------------
    String lonText =
      gps.location.isValid()
      ? String(gps.location.lng(), 6)
      : "NO FIX";

    if (lonText != lastLonText) {
      tft.fillRect(80, 265, 220, 15, TFT_BLACK);
      tft.setCursor(80, 265);
      tft.println(lonText);
      lastLonText = lonText;
    }

    //-----------------------------------
    // ALT
    //-----------------------------------
    String altText =
      gps.altitude.isValid()
      ? String(gps.altitude.meters()) + " m"
      : "NO FIX";

    if (altText != lastAltText) {
      tft.fillRect(80, 290, 220, 15, TFT_BLACK);
      tft.setCursor(80, 290);
      tft.println(altText);
      lastAltText = altText;
    }

    //-----------------------------------
    // HDOP
    //-----------------------------------
    String hdopText =
      gps.hdop.isValid()
      ? String(gps.hdop.hdop())
      : "N/A";

    if (hdopText != lastHdopText) {
      tft.fillRect(80, 315, 220, 15, TFT_BLACK);
      tft.setCursor(80, 315);
      tft.println(hdopText);
      lastHdopText = hdopText;
    }
  }
}