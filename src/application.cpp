#include "esp32-hal.h"
//
//    Copyright 2024 Marc SIBERT
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#include "application.h"

#include <esp_wifi.h>
#include <vector>
// #include "ftntp_client.h"
#include "splash.h"

TimeChangeRule frSTD = {"CET", Last, Sun, Mar, 2, +120};   // UTC +2 hours
TimeChangeRule frDST = {"CEST", Last, Sun, Oct, 3, +60};  // UTC +1 hours
Timezone frParis(frSTD, frDST);

Application::Application() : tft(TFT_eSPI()), time(0), udp(), timezone(frParis), servers()
{
  tft.init();
  tft.setRotation(3);
}

bool Application::initWiFi() {

  tft.setCursor(0,0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.println("Setup WiFi");
  tft.setTextFont(2);
  const auto xPos = tft.getCursorX();
  const auto yPos = tft.getCursorY();

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  wifi_config_t wifi_config;
  strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
  strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
  wifi_config.sta.bssid_set = false;
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_ERROR_CHECK(esp_wifi_connect());

  wifi_ap_record_t ap_info;
  const auto start = time.getEpoch();
  while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
    yield();
    tft.setCursor(xPos, yPos);
    tft.println("Connecting...");
    if (time.getEpoch() > start + 30) { // timedout
      ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    }
  }

  tft.setCursor(xPos, yPos);
  tft.println("Connected.     ");
  tft.print("SSID: "); tft.println((char*)ap_info.ssid);
  tft.printf("RSSI: %d dB\n", ap_info.rssi);
  return true;
}

void Application::loop() {
  static unsigned long last = 0;  // time.getEpoch()
  static unsigned poll = 1;
  static std::vector<int64_t> offsets(10);

  const auto epoch = time.getEpoch();
  if (epoch != last) {
    if (!last) tft.fillScreen(TFT_BLACK); // First loop
    displayTime(epoch);

    if (!(epoch % poll)) {
//          Serial.printf("%d + %d >= %d \n", last, poll, epoch);
      NTP ntp = NTP::makeNTP(NTPMODE_CLIENT);
      sendNTP(ntp, POOL_NTP, PORT_NTP);
    }

    last = epoch;
    return;
  }

  NTP ntp = NTP::makeNTP(NTPMODE_CLIENT, 3);
  if (waitForNTP(ntp, PORT_NTP)) {
    const auto offset = ntp.getOffset();
    offsets.insert(offsets.cbegin(), offset);
            
    addServer(ntp.getId(), ntp.getPolling(), epoch);
    poll = (ntp.getPolling() > 30 ? 30 : ntp.getPolling() );
    if (!poll) poll = 1;
    const auto rtt = ntp.getRTT();
    const auto ip = ntp.getIP();
    const auto headers = ntp.getHeader();
    const double precision = ntp.getPrecision();

    static const auto P = 0.05;
    const long correction = ((rtt < 30000) || (precision < 1e-5)) ? (offset * poll) * P : 0;

    if (correction != 0) {
      const auto d = correction / 1000000;
      const auto m = correction - d * 1000000;
      time.setTime(time.getEpoch() + d, time.getMicros() + m);
    }

    Serial.printf("IP:\"%s\", Hdr:\"%s\", prec:%Lg, ", ip, headers, precision);
    Serial.printf("Err:%lld, Rtt:%lu, Poll:%d, ",  offset, rtt, poll);
    Serial.printf("Corr:%ld ", correction);
    Serial.println();
  }
}

void Application::setFirstTime() {
  Serial.println(__PRETTY_FUNCTION__);
  NTP ntp = NTP::makeNTP(NTPMODE_CLIENT, 3);
  while (true) {
//        Serial.println(time.getDateTime());
    sendNTP(ntp, POOL_NTP, PORT_NTP);
    if (waitForNTP(ntp, PORT_NTP, 100)) {
      const auto t = ntp.getT2();
      const auto d = (t - YEAR1970 * 1000000) / 1000000;
      const auto m = (t - YEAR1970 * 1000000) - d * 1000000;
      time.setTime(d, m);
      break;
    }

    Serial.println(ntp.getHeader());
    Serial.printf("IP: %s\n", ntp.getIP());
    Serial.printf("Src prec. [s]: %e\n", ntp.getPrecision());
    Serial.printf("Diff [µs]: %lld\n", ntp.getOffset());
    Serial.printf("RTT [µs]: %lu\n", ntp.getRTT());
    Serial.println(time.getDateTime());

    const auto polling = ntp.getPolling();
    delay((polling > 30 ? 30 : polling) * 1000);
  }
/*
  Serial.println(ntp.getHeader());
  Serial.printf("IP: %s\n", ntp.getIP());
  Serial.printf("Src prec. [s]: %e\n", ntp.getPrecision());
  Serial.printf("Diff [µs]: %lld\n", ntp.getOffset());
  Serial.printf("RTT [µs]: %lu\n", ntp.getRTT());
  Serial.println(time.getDateTime());
*/
}

void Application::setup() {
  splashScreen();
  initWiFi();
  setFirstTime();
}

void Application::splashScreen() {  
  char c[50];

  tft.setSwapBytes(true);
  tft.pushImage(0,0,240,135,splash_240x135x16);

  snprintf(c, sizeof(c), "ESP32 NTP Timer v0");
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_BLACK);
  for (auto i = -1; i < 2; ++i)
    for (auto j = -1; j < 2; ++j)
      tft.drawString(c, tft.width() / 2 + i, j, 4);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(c, tft.width() / 2, 0, 4);

  snprintf(c, sizeof(c), "Copyright (c) %s M. SIBERT", __DATE__ + 7);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(c, tft.width() / 2, 135, 2);
  delay(5000);
}

void Application::displayTime(const unsigned long& epoch) {
  static const char *const days[] = { "Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam" };
  static const char *const months[] = { "Janv.", "Fevr.", "Mars", "Avril", "Mai", "Juin", "Juil.", "Aout", "Sept.", "Octo.", "Nove.", "Dece." };
  struct tm tmUTC, tmLocal;

  gmtime_r( (const time_t*)(&epoch), &tmUTC );
  const auto epochLocal = timezone.localtime(epoch);
  gmtime_r( (const time_t*)(&epochLocal), &tmLocal );

  char str[100];
    
// Heure locale
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (strftime(str, sizeof(str), "%R", &tmLocal)) {
    const auto x = 25;
    const auto y = 40;
    const auto dx = tft.drawString(str, x, y, 6);
    if (strftime(str, sizeof(str), ":%S", &tmLocal))
      tft.drawString(str, x + 1 + dx, y, 4);
  }

// Date locale
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (snprintf(str, sizeof(str), "%s. %02d %s %d", days[tmLocal.tm_wday], tmLocal.tm_mday, months[tmLocal.tm_mon], tmLocal.tm_year + 1900) > 0)
    tft.drawString(str, tft.width()/2, tft.height() - 18, 4);

// Date et heure UTC.
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  if (strftime(str, sizeof(str), "%c UTC", &tmUTC))
    tft.drawString(str, tft.width()/2, tft.height(), 2);
}


