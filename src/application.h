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

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Time.h>
#include "ntp.h"

// #define TOUCH_CS 0xFF
// #include <TFT_eSPI.h>
// #include <SPI.h>

const char serverName[] = "fr.pool.ntp.org"; // Adresse IP du serveur UDP
const int serverPort = 123; // Port du serveur UDP

#define MS1900(A, B) ( ((((A[0] * 256UL + A[1]) * 256UL + A[2]) * 256UL + A[3]) * 1000000ULL) + (((((B[0] * 256UL + B[1]) * 256UL + B[2]) * 256UL + B[3]) * 1000000ULL) >> 32) )
#define SET_REGISTER(r, t) { (r)[0] = (t) >> 24; (r)[1] = (t >> 16) & 0x0FF; (r)[2] = (t >> 8) & 0x0FF; (r)[3] = t & 0x0FF; }
#define POOL_NTP "fr.pool.ntp.org"
#define PORT_NTP 123

// #include <string>

/**
 * Classe Application ; expose les méthodes setup et loop qui sont utilisées dans les deux fonctions homonymes du programme principal.
 *
 **/
class Application {
  public:
    Application() :
      udp()
//      tft(TFT_eSPI()),
    {}

    void setup() {
      if (!initWiFi()) {
        Serial.println("Error setting up WiFi");
        exit(-1);
      }


      int64_t offset = 1000000;
      unsigned polling = 0;
      while (abs(offset) > 500) {
        delay(polling * 1000);
        NTP ntp = NTP::makeNTP();
        sendNTP(ntp, POOL_NTP, PORT_NTP);

        const bool received = waitForNTP(ntp, PORT_NTP, 100);
        if (!received || !ntp.getT1() || !ntp.getT2()) continue;

        offset = ntp.getOffset();

        time.setTime(time.getEpoch() + offset / 1000000, (time.getMicros() + offset) % 1000000);
        polling = ntp.getPolling();



        Serial.println(ntp.getHeader());
        Serial.print("Max polling [s]: ");
        Serial.println(ntp.getPolling());

        char prec[50];
        snprintf(prec, 50, "Src prec. [s]: %e", ntp.getPrecision());
        Serial.println(prec);

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


      /*
      uint64_t t1, tp1, tp2, t2 = 0;

      getNTP("fr.pool.ntp.org", t1, tp1, tp2, t2);
      time.setTime((tp1+tp2) / 2000000 - YEAR1970, ((tp1 + tp2) / 2) % 1000000);
*/
//      tft.init();
//      tft.setRotation(3);
    }

    void loop() {
      // afficher(time.getEpoch());

      // Serial.println(millis() % 30000);

      static float moyenne = 0;

      if (!(millis() % 500)) {
//        Serial.println(time.getDateTime());

        uint64_t t1, tp1, tp2, t2 = 0;
//        getNTP("fr.pool.ntp.org", t1, tp1, tp2, t2);

/*
        Serial.print("T1:  ");
        Serial.println(t1);
        Serial.print("T'1: ");
        Serial.println(tp1);
        Serial.print("T'2: ");
        Serial.println(tp2);
        Serial.print("T2:  ");
        Serial.println(t2);
*/
        if (!tp1 || !tp2 || !t1 || !t2) {
//          Serial.println("Error in mesure!");
          return;
        }

        const unsigned long run = t2 - t1;
//        Serial.print("Run (ms): ");
//        Serial.println(run / 1000.0);
        Serial.print(run);
        Serial.print(", ");

//        Serial.print("Diff: ");
        const int64_t diff1 = tp1 - t1;
//        Serial.print(diff1);
        const int64_t diff2 = t2 - tp2;
//        Serial.print(" - ");
//        Serial.print(diff2);
        const int64_t diff = (diff1 - diff2) / 2;
//        Serial.print(" = ");
//        Serial.println(diff);
        Serial.print(diff);
        Serial.print(", ");

        const auto n = 50;
        moyenne = (moyenne * (n-1) + diff) / n;
//        Serial.print("Moyenne: ");
//        Serial.println(moyenne);
        Serial.print(moyenne);
        Serial.println("");

        if (abs(diff) < 500) {
//          Serial.println("Low diff.");
          return;
        }

        const auto epoch = time.getEpoch();
        const auto ms = time.getMicros();
        time.setTime( epoch + (int(round(moyenne/10)) / 1000000), (ms + int(trunc(moyenne/10))) % 1000000);
//        Serial.print("Local before: ");
//        Serial.println((epoch + YEAR1970) * 1000000 + ms);
//        Serial.print("After: ");
//        Serial.println((time.getEpoch() + YEAR1970) * 1000000 + time.getMicros());
      }
    }


/*
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.setTextFont(2);
      tft.setCursor(0, 0);
      tft.print("Header Horloge "); tft.println(__DATE__);
      tft.println();
      tft.setTextFont(7);
      const auto m = millis();
      tft.printf("%02d:%02d:%02d", (hours + m / 1000 / 3600) % 24, (minutes + m / 1000 / 60) % 60, (seconds + m / 1000) % 60);
*/

  protected:

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
      WiFi.begin(WIFI_SSID, WIFI_PASS)  ;
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print("Connecting to Wifi (");
        switch (WiFi.status()) {
          case WL_NO_SHIELD:
            Serial.print("hardware missing");
            break;
          case WL_IDLE_STATUS:
            Serial.print("idle");
            break;
          case WL_NO_SSID_AVAIL:
            Serial.print("SSID unavailable");
            break;
          case WL_SCAN_COMPLETED:
            Serial.print("Scan completed");
            break;
          case WL_CONNECTED:
            Serial.print("connected");
            break;
          case WL_CONNECT_FAILED:
            Serial.print("connect failed");
            break;
          case WL_CONNECTION_LOST:
            Serial.print("connection lost");
            break;
          case WL_DISCONNECTED:
            Serial.print("disconnected");
            break;
        }
        Serial.println(")...");
      }
      Serial.println("Wifi connected.");
      return true;
    }

/*
    void printPacket(const ntp_packet& packet) const {
      const auto ref = MS1900(packet.refTm_s, packet.refTm_f);
      Serial.print("Ref: ");
      Serial.println(ref / 1000.0);
      const auto orig = MS1900(packet.origTm_s, packet.origTm_f);
      Serial.print("Orig: ");
      Serial.println(orig / 1000.0);
      const auto rx = MS1900(packet.rxTm_s, packet.rxTm_f);
      Serial.print("Rx: ");
      Serial.println(rx / 1000.0);
      const auto tx = MS1900(packet.txTm_s, packet.txTm_f);
      Serial.print("Tx: ");
      Serial.println(tx / 1000.0);
    }
*/

  private:
    // TFT_eSPI tft;
    ESP32Time time;
    WiFiUDP udp;

};


