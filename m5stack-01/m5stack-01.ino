#include <M5Stack.h>
#include <WiFi.h>

const int wifiConnectTimeout = 30;

void setup() {
  M5.begin();
  M5.Power.begin();

  M5.Lcd.setTextFont(4);
  M5.Lcd.print("Boot");

  WiFi.begin();
  for (int i = 0; WiFi.status() != WL_CONNECTED && i < wifiConnectTimeout; i++) {
    M5.Lcd.print(".");
    delay(1000); 
  }

  M5.Lcd.clear(BLACK);
  M5.Lcd.setCursor(0, 0);

  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.print(WiFi.SSID());
    M5.Lcd.println(F(" Connected!"));
    //M5.Lcd.print(WiFi.psk());
  }
  else {
    M5.Lcd.println(F("Cannot connect to Wi-fi."));
  }
}

void loop() {

}
