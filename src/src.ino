// #define WIFI_SSID
// #define WIFI_PASS

#include "secrets.h"
#include "application.h"

Application app;

void setup() {
  Serial.begin(115200);
  delay(500);
  app.setup();
}

void loop() {
  app.loop();
}
