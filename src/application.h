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

#define TOUCH_CS 0xFF
#include <TFT_eSPI.h>
// #include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Time.h>
#include "ntp.h"
#include "timezone.h"

#include "ftntp_client.h"
// #include "DSEG14_Classic_Bold_Italic_25.h"


#define POOL_NTP "fr.pool.ntp.org"
#define PORT_NTP 123

TimeChangeRule frSTD = {"CET", Last, Sun, Mar, 2, +120};   // UTC +2 hours
TimeChangeRule frDST = {"CEST", Last, Sun, Oct, 3, +60};  // UTC +1 hours
Timezone frParis(frSTD, frDST);

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
    Application() : tft(TFT_eSPI()), time(0), udp(), timezone(frParis), servers()
    {
      tft.init();
      tft.setRotation(3);
    }

/**
 * Method called once at startup.
 */
    void setup() {
      splash_screen();

      if (!initWiFi()) {
        tft.setTextColor(TFT_RED);
        tft.println("Error setting up WiFi");
        exit(-1);
      }

      setFirstTime();
    }

/**
 * Method called in loop until the end of the world.
 */
    void loop() {
      return;

      static unsigned long last = 0;  // time.getEpoch()
      static unsigned poll = 1;

      const auto epoch = time.getEpoch();
      if (epoch != last) {
        if (!last) tft.fillScreen(TFT_BLACK); // First loop
        displayTime(epoch);

        if (!(epoch % poll)) {
//          Serial.printf("%d + %d >= %d \n", last, poll, epoch);
          NTP ntp = NTP::makeNTP();
          sendNTP(ntp, POOL_NTP, PORT_NTP);
        }

        last = epoch;
        return;
      }

      NTP ntp = NTP::makeNTP();
      const bool received = waitForNTP(ntp, PORT_NTP, 0);
      if (received && ntp.getT1() && ntp.getT2() && ntp.getVersion() == 3 && ntp.getMode() == 4) {
        const auto offset = ntp.getOffset();
        addServer(ntp.getId(), ntp.getPolling(), epoch);
        poll = (ntp.getPolling() > 30 ? 30 : ntp.getPolling() );
        const auto rtt = ntp.getRTT() / 1000000.0;
        const auto headers = ntp.getHeader();

        Serial.printf("\"%s\", %f, %f, %d, \"%s\"\n", ntp.getIP(), offset / 1000.0, rtt, poll, headers);
      }







    }

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
 * @todo
 */
    void splash_screen() {  
      tft.fillScreen(TFT_BLACK);
      tft.setSwapBytes(true);
      tft.pushImage(0,0,135,135,ftntp_client);
      delay(1000);

   //   tft.setFreeFont(&DSEG14_Classic_Bold_Italic_25);

      tft.setTextColor(TFT_YELLOW);
//      tft.setTextFont(4);
      tft.println("ESP32 NTP Timer v0");
      tft.setTextFont(2);
      tft.print("Copyright (c) ");  tft.print(__DATE__ + 7); tft.println(" M. SIBERT");

      tft.setTextColor(TFT_WHITE);
      tft.print("Se connecte au WiFi puis a l'aide de requetes NTP regulieres maintient sa base temps a moins d'une milliseconde de precision.");
      delay(2000);
    }

/**
 * Display current time (local clock) to the display.
 * @param epoch The current time.
 */
    void displayTime(const unsigned long& epoch) {
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

/**
 * Setup local time for the first time until time offset is lower than 1ms (MAX_OFFSET).
 */
    void setFirstTime() {
      NTP ntp = NTP::makeNTP();
      while (true) {
//        Serial.println(time.getDateTime());
        sendNTP(ntp, POOL_NTP, PORT_NTP);
        const bool received = waitForNTP(ntp, PORT_NTP, 100);
        if (received) {
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

    bool waitForNTP(NTP& ntp, const unsigned port, const unsigned timeout = 100) {
      const auto start = millis();
      while ((udp.parsePacket() < ntp.packetSize()) && (millis() < (start + timeout))) {
        yield();
      }
      
      uint64_t rx = time.getMicros();
      rx += (time.getEpoch() + YEAR1970) * 1000000ULL;

      if (millis() > (start + timeout)) return false;

      const auto size = NTP::packetSize();
      uint8_t buffer[size];
      udp.read(buffer, size);
      ntp.setPacket(buffer);
      ntp.setT3(rx);

      if (ntp.getT3() < ntp.getT0()) {
        Serial.println("WARNING ! T3 < T0 ");
        return false;
      }
      if (ntp.getT2() < ntp.getT1()) {
        Serial.println("WARNING ! T2 < T1 ");
        return false;
      }

      if (!ntp.getT1() || !ntp.getT2() || ntp.getVersion() != 3 || ntp.getMode() != 4) return false;

      return true;
    }
    
/**
 * Setup WiFi using Application's template WIFI_SSID & WIFI_PASS.
 * @return True if ok or else False.
 */
    bool initWiFi() {
      tft.setCursor(0,0);
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextFont(4);
      tft.println("Setup WiFi");
      tft.setTextFont(2);

      const auto xPos = tft.getCursorX();
      const auto yPos = tft.getCursorY();
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      while (true) {
        delay(500);
        tft.setCursor(xPos, yPos);
        tft.print("SSID: "); tft.println(WIFI_SSID);

        tft.print("Stat: ");
        switch (WiFi.status()) {
          case WL_NO_SHIELD:
            tft.println("hardware missing!   ");
            return false;
          case WL_IDLE_STATUS:
            tft.println("idle...             ");
            break;
          case WL_NO_SSID_AVAIL:
            tft.println("SSID unavailable!   ");
            break;
          case WL_CONNECTED:
            tft.println("connected.          ");
            tft.print("IP: "); tft.println(WiFi.localIP());
            tft.print("RSSI [dB]: "); tft.println(WiFi.RSSI());
            return true;
          case WL_CONNECT_FAILED:
            tft.println("connect failed!     ");
            break;
          case WL_CONNECTION_LOST:
            tft.println("connection lost...  ");
            break;
          case WL_DISCONNECTED:
            tft.println("disconnected...     ");
            break;
          default:
            break;

        }
      }
    }

  private:
    TFT_eSPI tft;
    ESP32Time time;
    WiFiUDP udp;
    Timezone timezone;
    NTPServer servers[10];
};


