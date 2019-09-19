#include <M5Stack.h>
#include <WiFi.h>
#include <TinyGPS++.h>

#define LCD_CLEAR(s) do { M5.Lcd.clear(s); M5.Lcd.setCursor(0, 0); } while(0);
#define LCD_WAIT_C(s, t) do { M5.Lcd.print(s); delay(t); } while(0);

const int wifiConnectTimeout = 30;
const int wifiServerPort = 2323;
WiFiServer wifiServer(wifiServerPort);
WiFiClient wifiClient;
volatile boolean wifiClientConnected = false;

HardwareSerial GPSRaw(2);
TinyGPSPlus gps;
const int gpsModuleTimeout = 30;

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
    wifiServer.begin();
    xTaskCreatePinnedToCore(uartServer, "uartServer", 8192, NULL, 1, NULL, 1);
  }
  else {
    M5.Lcd.println(F("Cannot connect to Wi-fi."));
  }

  M5.Speaker.end();

  GPSRaw.begin(9600);
  M5.Lcd.print(F("GPS is being receiving."));
  for (int i = 0; !GPSRaw.available() && i < gpsModuleTimeout; i++) {
    LCD_WAIT_C(".", 1000);
  }
  delay(3000);
  LCD_CLEAR(BLACK);
  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.println(F("Debug Mode Enabled."));
    M5.Lcd.print(F("Local IPADDR: "));
    M5.Lcd.println(WiFi.localIP());
    M5.Lcd.print(F("Debug Port: "));
    M5.Lcd.println(wifiServerPort);
  }
}

void loop() {
  if (wifiClientConnected) {
    M5.Lcd.fillRect(0, 80, 320, 160, BLACK);
    M5.Lcd.setCursor(0, 80);
    M5.Lcd.println("Client Connected.");
    delay(1000);
    return;
  }
  if (GPSRaw.available()) {
    gps.encode(GPSRaw.read());
    if (gps.time.isUpdated()) {
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
    }
  }
}
