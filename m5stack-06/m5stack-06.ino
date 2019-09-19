#include <M5Stack.h>
#include <WiFi.h>
#include <TinyGPS++.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>

#define TINY_GSM_MODEM_UBLOX
#include <TinyGsmClient.h>

#define LCD_CLEAR(s) do { M5.Lcd.clear(s); M5.Lcd.setCursor(0, 0); } while(0);
#define LCD_WAIT_C(s, t) do { M5.Lcd.print(s); delay(t); } while(0);

const int wifiConnectTimeout = 30;
const int wifiServerPort = 2323;
WiFiServer wifiServer(wifiServerPort);
WiFiClient wifiClient;
volatile boolean wifiClientConnected = false;

HardwareSerial GPSRaw(0);
TinyGPSPlus gps;
const int gpsModuleTimeout = 30;

TinyGsm modem(Serial2); /* 3G board modem */
TinyGsmClientSecure ctx(modem);

const int httpRequestTimeCount = 60;
int gpsUpdateTimeCounter = httpRequestTimeCount;

const char* httpHost = "example.com";
String httpPath = "/index";
const int httpPort = 443;
boolean gsmConnected = false;

void otaBegin() {
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");
  ArduinoOTA.setHostname("m5stack");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    LCD_CLEAR(BLACK);
    M5.Lcd.println("Start updating " + type);
    M5.Lcd.fillRect(0, 110, 320, 20, TFT_WHITE);
    M5.Lcd.fillRect(2, 112, 316, 16, TFT_BLACK);
  })
  .onEnd([]() {
    LCD_CLEAR(BLACK);
    M5.Lcd.println("reboot");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    M5.Lcd.progressBar(2, 112, 316, 16, (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    M5.Lcd.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) M5.Lcd.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) M5.Lcd.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) M5.Lcd.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) M5.Lcd.println("Receive Failed");
    else if (error == OTA_END_ERROR) M5.Lcd.println("End Failed");
  });

  ArduinoOTA.begin();
}

void uartServer(void* arg)
{
  while (WiFi.status() == WL_CONNECTED) {
    do {
      wifiClient = wifiServer.available();
    } while (!wifiClient);
    if (wifiClient.connected()) {
      wifiClientConnected = true;
      wifiClient.print("\033[2J\033[0;0H");
      wifiClient.println("GPS is being receiving.");
      // GPSモジュールからデータを受信するまで待機
      while (!GPSRaw.available()) {
        if (!wifiClient.connected()) {
          break;
        }
        wifiClient.print(".");
        vTaskDelay(1000);
      }
      wifiClient.println("");
      while (wifiClient.connected()) {
        while (GPSRaw.available()) {
          wifiClient.write(GPSRaw.read());
        }
      }
      wifiClientConnected = false;
      wifiClient.stop();
    }
    vTaskDelay(100);
  }
}

void setup() {
  M5.begin();
  M5.Power.begin();
  M5.Speaker.begin();

  M5.Lcd.setTextFont(4);
  M5.Lcd.print("Boot");

  WiFi.begin();
  for (int i = 0; WiFi.status() != WL_CONNECTED && i < wifiConnectTimeout; i++) {
    LCD_WAIT_C(".", 1000);
  }

  LCD_CLEAR(BLACK);

  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.print(WiFi.SSID());
    M5.Lcd.println(F(" Connected!"));
    //M5.Lcd.print(WiFi.psk());
    otaBegin();
    wifiServer.begin();
    xTaskCreatePinnedToCore(uartServer, "uartServer", 8192, NULL, 1, NULL, 1);
  }
  else {
    M5.Lcd.println(F("Cannot connect to Wi-fi."));
  }

  M5.Speaker.end();

  LCD_CLEAR(BLACK);
  M5.Lcd.println(F("3G Module init!"));
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  modem.restart();
  M5.Lcd.println(F("done"));

  M5.Lcd.print(F("getModemInfo:"));
  String modemInfo = modem.getModemInfo();
  M5.Lcd.println(modemInfo);

  M5.Lcd.print("waitForNetwork. ");
  while (!modem.waitForNetwork()) {
    M5.Lcd.print("-");
  }
  M5.Lcd.println(F("> Ok!"));

  M5.Lcd.print("gprsConnect(soracom.io) => ");
  modem.gprsConnect("soracom.io", "sora", "sora");
  M5.Lcd.println(F("done"));

  M5.Lcd.print("isNetworkConnected. ");
  while (!modem.isNetworkConnected()) {
    M5.Lcd.print("-");
  }
  M5.Lcd.println(F("> Ok!"));

  delay(3000);
  M5.Lcd.clear(BLACK);
  M5.Lcd.setCursor(0, 0);

  GPSRaw.begin(9600);
  M5.Lcd.print(F("GPS is being receiving."));
  for (int i = 0; !GPSRaw.available() && i < gpsModuleTimeout; i++) {
    LCD_WAIT_C(".", 1000);
  }
  delay(3000);
  LCD_CLEAR(BLACK);
  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.println(F("OTA Enabled."));
    M5.Lcd.println(F("Debug Mode Enabled."));
    M5.Lcd.print(F("Local IPADDR: "));
    M5.Lcd.println(WiFi.localIP());
    M5.Lcd.print(F("Debug Port: "));
    M5.Lcd.println(wifiServerPort);
  }
}

String formatYmdhms(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  String ymdhms;
  char ymd[12], hms[13];
  sprintf(ymd, "%04d-%02d-%02d+", year, month, day);
  ymdhms += ymd;
  sprintf(hms, "%02d%%3A%02d%%3A%02d", hour, minute, second);
  ymdhms += hms;
  return ymdhms;
}

String buildUrlGetQuery(const char* host, String path,
                        uint16_t year, uint8_t month, uint8_t day,
                        uint8_t hour, uint8_t minute, uint8_t second,
                        double lat, double lng,
                        boolean useComplete
                       ) {
  String u = "";
  if (useComplete) {
    u += "https://";
    u += host;
  }
  u += path;

  char latStr[15], lngStr[15];
  dtostrf(lat, 9, 6, latStr);
  u += "?lat=";
  u += latStr;
  dtostrf(lng, 10, 6, lngStr);
  u += "&lng=";
  u += lngStr;
  String utc;
  utc = formatYmdhms(year, month, day, hour, minute, second);
  u += "&utc=";
  u += utc;
  return u;
}

void loop() {
  ArduinoOTA.handle();
  if (wifiClientConnected) {
    M5.Lcd.fillRect(0, 80, 320, 160, BLACK);
    M5.Lcd.setCursor(0, 80);
    M5.Lcd.println("Client Connected.");
    delay(1000);
    return;
  }
  if (GPSRaw.available()) {
    ArduinoOTA.handle();
    gps.encode(GPSRaw.read());
    if (gps.time.isUpdated()) {
      gpsUpdateTimeCounter--;
      M5.Lcd.setCursor(0, 80);
      M5.Lcd.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                    (uint16_t) gps.date.year(),
                    (uint8_t) gps.date.month(),
                    (uint8_t) gps.date.day(),
                    (uint8_t) gps.time.hour(),
                    (uint8_t) gps.time.minute(),
                    (uint8_t) gps.time.second()
                   );
      M5.Lcd.print(F("lat: "));
      M5.Lcd.println(gps.location.lat(), 6);
      M5.Lcd.print(F("lng: "));
      M5.Lcd.println(gps.location.lng(), 6);
      if (gpsUpdateTimeCounter <= 0) {
        gpsUpdateTimeCounter = httpRequestTimeCount;
        gsmConnected = false;
        if (ctx.connect(httpHost, httpPort, 30)) {
          String url = buildUrlGetQuery(httpHost,
                                        httpPath,
                                        gps.date.year(),
                                        gps.date.month(),
                                        gps.date.day(),
                                        gps.time.hour(),
                                        gps.time.minute(),
                                        gps.time.second(),
                                        gps.location.lat(),
                                        gps.location.lng(),
                                        false
                                       );
          ctx.print("GET ");
          ctx.print(url);
          ctx.println(" HTTP/1.0");
          ctx.print("Host: ");
          ctx.println(httpHost);
          ctx.println();
          while (ctx.connected()) {
            String line = ctx.readStringUntil('\n');
            if (line == "\r") {
              break;
            }
          }
          char buf[1 * 1024] = {0};
          ctx.readBytes(buf, sizeof(buf)); /* body */
          ctx.stop();
          gsmConnected = true;
        }
        if (!gsmConnected && WiFi.status() == WL_CONNECTED) {
          String url = buildUrlGetQuery(httpHost,
                                        httpPath,
                                        gps.date.year(),
                                        gps.date.month(),
                                        gps.date.day(),
                                        gps.time.hour(),
                                        gps.time.minute(),
                                        gps.time.second(),
                                        gps.location.lat(),
                                        gps.location.lng(),
                                        true
                                       );
          HTTPClient http;
          http.begin(url);
          int httpCode = http.GET();
          http.end();
        }
      }
    }
  }
}
