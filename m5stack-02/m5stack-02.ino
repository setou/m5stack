#include <M5Stack.h>
#include <WiFi.h>
#include <TinyGPS++.h>

const int wifiConnectTimeout = 30;
HardwareSerial GPSRaw(2);
TinyGPSPlus gps;

#define LCD_CLEAR(s) do { M5.Lcd.clear(s); M5.Lcd.setCursor(0, 0); } while(0);
#define LCD_WAIT_C(s, t) do { M5.Lcd.print(s); delay(t); } while(0);

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
  }
  else {
    M5.Lcd.println(F("Cannot connect to Wi-fi."));
  }

  M5.Speaker.end();

  GPSRaw.begin(9600);
  M5.Lcd.print(F("GPS is being receiving."));
  while (!GPSRaw.available()) {
    LCD_WAIT_C(".", 1000);
  }
  delay(3000);
  LCD_CLEAR(BLACK);
}

void loop() {
  if (GPSRaw.available()) {
    gps.encode(GPSRaw.read());
    if (gps.time.isUpdated()) {
      M5.Lcd.setCursor(0, 0);
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
