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

#define POOL_NTP "fr.pool.ntp.org"
#define PORT_NTP 123

TimeChangeRule frSTD = {"CET", Last, Sun, Mar, 2, +60};   // UTC + 1 hours
TimeChangeRule frDST = {"CEST", Last, Sun, Oct, 3, +120};  // UTC + 2 hours
Timezone frParis(frSTD, frDST);

/**
 * Classe Application ; expose les méthodes setup et loop qui sont utilisées dans les deux fonctions homonymes du programme principal.
 *
 **/
class Application {
  public:
    Application() : tft(TFT_eSPI()), time(0), udp(), timezone(frParis)
    {
      tft.init();
      tft.setRotation(3);
    }

    void setup() {
      splash_screen();

      if (!initWiFi()) {
        tft.setTextColor(TFT_RED);
        tft.println("Error setting up WiFi");
        exit(-1);
      }

      setFirstTime();
      
//      WiFi.disconnect(true);
//      WiFi.mode(WIFI_OFF);
    }

    void loop() {
      static auto last = 0;

      const auto epoch = time.getEpoch();
      if (epoch != last) {
        last = epoch;

        struct tm tmUTC = { time.getSecond(), time.getMinute(), time.getHour(true), time.getDay(), time.getMonth(), time.getYear() - 1900 };
        const auto utc = mktime(&tmUTC);


        char str[100];
        
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        if (strftime(str, sizeof(str), "%X", &tmUTC))
          tft.drawString(str, tft.width()/2, tft.height()/2, 7);
        tft.setTextDatum(BL_DATUM);
        if (strftime(str, sizeof(str), "%a %b %e %Y - S%V - %Z", &tmUTC))
        tft.drawString(str, 0, tft.height(), 2);

      }
    }

  protected:

    void splash_screen() {  
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_YELLOW);
      tft.setTextFont(4);
      tft.println("ESP32 NTP Timer v0");
      tft.setTextFont(2);
      tft.print("Copyright (c) ");  tft.print(__DATE__ + 7); tft.println(" M. SIBERT");

      tft.setTextColor(TFT_WHITE);
      tft.print("Se connecte au WiFi puis a l'aide de requetes NTP regulieres maintient sa base temps a moins d'une milliseconde de precision.");
      delay(2000);
    }

    void setFirstTime() {
      int64_t offset = 1000000;
      unsigned polling = 0;
      while (abs(offset) > 500) {
        delay((polling > 30 ? 30 : polling) * 1000);
        NTP ntp = NTP::makeNTP();
        sendNTP(ntp, POOL_NTP, PORT_NTP);

        const bool received = waitForNTP(ntp, PORT_NTP, 100);
        if (!received || !ntp.getT1() || !ntp.getT2() || ntp.getVersion() != 3 || ntp.getMode() != 4) continue;

        offset = ntp.getOffset();

        time.setTime(time.getEpoch() + offset / 1000000, (time.getMicros() + offset) % 1000000);
        polling = ntp.getPolling();

        Serial.println(ntp.getHeader());
        Serial.print("Max polling [s]: ");
        Serial.println(polling);
        char prec[50];
        snprintf(prec, 50, "Src prec. [s]: %e", ntp.getPrecision());
        Serial.println(prec);
        Serial.print("IP: ");
        Serial.println(ntp.getIP());

        Serial.print("T0: ");
        Serial.println(ntp.getT0());
        Serial.print("T1: ");
        Serial.println(ntp.getT1());
        Serial.print("T2: ");
        Serial.println(ntp.getT2());
        Serial.print("T3: ");
        Serial.println(ntp.getT3());
        Serial.print("Diff: ");
        Serial.println(ntp.getOffset());

        Serial.println(time.getDateTime());
      }
    }

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
      return true;
    }


/*
    void allFonts() {
      for (int i = 0; i <= 8; ++i) {
        tft.setCursor(0, 0);
        tft.setTextFont(i);
        tft.fillScreen(TFT_BLACK);
        tft.print(i);
        tft.println(" Hello World!");
        delay(2000);
      }
    }
*/

/**
 * send a NTP request and return the responce.
 **/
 /*
    void getNTP(const char server[], uint64_t& t1, uint64_t& tp1, uint64_t& tp2, uint64_t& t2) {
      struct ntp_packet {
        uint8_t li_vn_mode;      // Eight bits. li, vn, and mode (Flags  MSB[LI:2 VN:3 Mode:3]LSB   LI=0, VN=3 mode=3)

        uint8_t stratum;         // Eight bits. Stratum level of the local clock.
        uint8_t poll;            // Eight bits. Maximum interval between successive messages. (2^n)
        int8_t  precision;       // Eight bits. Precision of the local clock. -10 --> 2^(-10) ~ 0.97 msec

        int32_t rootDelay;      // 32 bits. Total round trip delay time.
        int32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
        char    refId[4];       // 32 bits. Reference clock identifier as char[4]

        uint8_t refTm_s[4];      // 32 bits. Reference time-stamp seconds.
        uint8_t refTm_f[4];      // 32 bits. Reference time-stamp fraction of a second.

        uint8_t origTm_s[4];     // 32 bits. Originate time-stamp seconds.
        uint8_t origTm_f[4];     // 32 bits. Originate time-stamp fraction of a second.

        uint8_t rxTm_s[4];       // 32 bits. Received time-stamp seconds.
        uint8_t rxTm_f[4];       // 32 bits. Received time-stamp fraction of a second.

        uint8_t txTm_s[4];      // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
        uint8_t txTm_f[4];      // 32 bits. Transmit time-stamp fraction of a second.

      };              // Total: 384 bits or 48 bytes.

      ntp_packet packet = (ntp_packet){ 0b00011011, 0, 0, -10, 0, 0, "", 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };

      udp.begin(123);
      udp.beginPacket(server, 123);

      const unsigned long s = time.getEpoch();
      const uint32_t tx_s = s + YEAR1970;
      SET_REGISTER(packet.txTm_s, tx_s);
      
      const uint64_t ms = time.getMicros();
      const uint32_t tx_ms = (ms << 32) / 1000000;
      SET_REGISTER(packet.txTm_f, tx_ms);

      udp.write((const uint8_t*)(&packet), sizeof(packet));
      udp.endPacket();

      const auto start = millis();
      while ((udp.parsePacket() < sizeof(packet)) && ((millis() - start) < 50)) {
        yield();
      }
      
      t2 = time.getMicros();
      t2 += (time.getEpoch() + YEAR1970) * 1000000ULL;

      if (millis() - start > 50) {
        t1 = t2 = tp1 = tp2 = 0;
        return;
      }

      udp.read((uint8_t*)(&packet), sizeof(packet));

      const auto ref = MS1900(packet.refTm_s, packet.refTm_f);

      // t1 = MS1900(packet.origTm_s, packet.origTm_f);
      t1 = (s + YEAR1970) * 1000000UL + ms;

      tp1 = MS1900(packet.rxTm_s, packet.rxTm_f);
      tp2 = MS1900(packet.txTm_s, packet.txTm_f);
    }
*/

/**
 * Setup WiFi using Application's template WIFI_SSID & WIFI_PASS.
 * @return True if ok or else False.
 **/
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
};


