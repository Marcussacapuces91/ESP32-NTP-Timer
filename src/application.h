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

#pragma once

#include "esp_log.h"

#define TOUCH_CS 0xFF
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Time.h>
#include "ntp.h"
#include "timezone.h"

#include "secrets.h"

#define POOL_NTP "fr.pool.ntp.org"
#define PORT_NTP 123


/**
 * NTP Server description.
 */
struct NTPServer {
  char          refId[4];
  unsigned      poll;
  unsigned long lastPoll;
};

/**
 * Classe Application ; expose les méthodes setup et loop qui sont utilisées dans les deux fonctions homonymes du programme principal.
 */
class Application {
  public:
/**
 * Public constructor.
 */
    Application();

/**
 * Method called once at startup.
 */
    void setup();

/**
 * Method called in loop until the end of the world.
 */
    void loop();

  protected:

/**
 * Add or replace a server in the list.
 * @param refId reference ID (IP or name).
 * @param polling [s].
 * @param lastPoll UTC time of the last poll.
 */
    void addServer(const char refId[4], const unsigned poll, const unsigned long lastPoll) {
      for (byte i = 0; i < 10; ++i) {
//        char s[50];
//        snprintf(s, 50, "addServer %d.%d.%d.%d", servers[i].refId[0], servers[i].refId[1], servers[i].refId[2], servers[i].refId[3]);
//        Serial.print(s);
        if (memcmp(servers[i].refId, "\0\0\0\0", 4) == 0) {
          memcpy(servers[i].refId, refId, 4) ;
          servers[i].poll = poll;
          servers[i].lastPoll = lastPoll;
//          Serial.println(" added");
          return;
        } 
        if (memcmp(refId, servers[i].refId, 4) == 0) {
          servers[i].poll = poll;
          servers[i].lastPoll = lastPoll;
//          Serial.println(" updated");
          return;
        }
      }
    }

/**
 * Splash screen explaining the aim of the application.
  */
    void splashScreen();

/**
 * Display current time (local clock) to the display.
 * @param epoch The current time.
 */
    void displayTime(const unsigned long& epoch);

/**
 * Setup local time for the first time until time offset is lower than 1ms (MAX_OFFSET).
 */
    void setFirstTime();

/**
 * Send a prepared NTP packet to the designed host and port using UDP.
 * @param ntp Reference to a NTP packet to be sent.
 * @param host Host's name (array of char).
 * @param port UDP port.
 */
    void sendNTP(NTP& ntp, const char host[], const unsigned port) {
      udp.begin(1024);
      udp.beginPacket(host, port);
      uint64_t tx = time.getMicros();
      tx += (time.getEpoch() + YEAR1970) * 1000000ULL;
      ntp.setT0(tx);
      udp.write(ntp.packetAddr(), ntp.packetSize());
      udp.endPacket();
    }

    bool waitForNTP(NTP& ntp, const unsigned port, const unsigned timeout = 0) {
      const auto start = millis();
      while ((udp.parsePacket() < ntp.packetSize()) && (millis() < (start + timeout))) {
        yield();
      }
      uint64_t rx = time.getMicros();
      rx += (time.getEpoch() + YEAR1970) * 1000000;

      if (millis() > (start + timeout)) return false; // timedout without packet.

      const auto size = NTP::packetSize();
      uint8_t buffer[size];
      const auto nb = udp.read(buffer, size);
      if (nb == -1) return false; // error reading!

      ntp.setPacket(buffer);
      ntp.setT3(rx);

      if (ntp.getT3() < ntp.getT0()) {
//        Serial.println("WARNING ! T3 < T0 ");
        return false;
      }
      if (ntp.getT2() < ntp.getT1()) {
//        Serial.println("WARNING ! T2 < T1 ");
        return false;
      }

      if (!ntp.getT1() || !ntp.getT2() || ntp.getVersion() != 3 || ntp.getMode() != NTPMODE_SERVER) return false;

      return true;
    }

/**
 * Setup WiFi using Application's template WIFI_SSID & WIFI_PASS.
 * @return True if ok or else False.
 */
    bool initWiFi();

  private:
    TFT_eSPI tft;
    ESP32Time time;
    WiFiUDP udp;
    Timezone timezone;
    NTPServer servers[10];
};


