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

#include <string>
#include <math.h>

#define YEAR1970 2208988800
#define MS1900(A, B) ( ((((A[0] * 256UL + A[1]) * 256UL + A[2]) * 256UL + A[3]) * 1000000ULL) + (((((B[0] * 256UL + B[1]) * 256UL + B[2]) * 256UL + B[3]) * 1000000ULL) >> 32) )

class NTP {
  public:

    static NTP makeNTP() {
      NTP result;
      return result;
    }

    const uint8_t* packetAddr() const {
      return (const uint8_t*)&packet;
    }

    static byte packetSize() {
      return sizeof(ntp_packet);
    }

    const char* getHeader() const {
      static char buffer[100];
      snprintf(buffer, 100, "Mode %d ; Vers. %d ; LI %d ; Stratum %d", 
        packet.li_vn_mode & 0b0111, 
        (packet.li_vn_mode >> 3) & 0b0111, 
        (packet.li_vn_mode >> 6) & 0b011, 
        packet.stratum);
      return buffer;
    }

    byte getMode() const {
      return (packet.li_vn_mode) % 8;
    }

    byte getVersion() const {
      return (packet.li_vn_mode >> 3) % 8;
    }

    unsigned getPolling() const {
      return 1UL << packet.poll;
    }

    double getPrecision() const {
      return pow(double(2), double(packet.precision));
    }

    const char* getIP() const {
      static char id[30];
      snprintf(id, 30, "%d.%d.%d.%d", packet.refId[0], packet.refId[1], packet.refId[2], packet.refId[3]);
      return id;
    }

    uint64_t getT0() const {
      return MS1900(packet.origTm_s, packet.origTm_f);
    }

    uint64_t getT1() const {
      return MS1900(packet.rxTm_s, packet.rxTm_f);
    } 

    uint64_t getT2() const {
      return MS1900(packet.txTm_s, packet.txTm_f);
    }

    uint64_t getT3() const {
      return t3;
    }

    int64_t getOffset() const {
      const int64_t diff1 = getT1() - getT0();
      const int64_t diff2 = getT2() - getT3();
      return (diff1 + diff2) / 2;
    }      

    inline void setPacket(const uint8_t buffer[]) {
      memcpy(&packet, buffer, sizeof(ntp_packet));
    }

    inline void setT0(const uint64_t& tx) {
      uint32_t sec = (tx / 1000000);
      packet.txTm_s[3] = sec & 0xFF;
      sec >>= 8;
      packet.txTm_s[2] = sec & 0xFF;
      sec >>= 8;
      packet.txTm_s[1] = sec & 0xFF;
      sec >>= 8;
      packet.txTm_s[0] = sec & 0xFF;

      uint32_t micros = (tx << 32) / 1000000;
      packet.txTm_f[3] = micros & 0xFF;
      micros >>= 8;
      packet.txTm_f[2] = micros & 0xFF;
      micros >>= 8;
      packet.txTm_f[1] = micros & 0xFF;
      micros >>= 8;
      packet.txTm_f[0] = micros & 0xFF;
    }

    inline void setT3(const uint64_t& rx) {
      t3 = rx;
    }

  private:

    struct ntp_packet {
      uint8_t li_vn_mode;      // Eight bits. li, vn, and mode (Flags  MSB[LI:2 VN:3 Mode:3]LSB   LI=0, VN=3 mode=3)

      uint8_t stratum;         // Eight bits. Stratum level of the local clock.
      uint8_t poll;            // Eight bits. Maximum interval between successive messages. (2^n)
      int8_t  precision;       // Eight bits. Precision of the local clock. -10 --> 2^(-10) ~ 0.97 msec

      int32_t rootDelay;       // 32 bits. Total round trip delay time.
      int32_t rootDispersion;  // 32 bits. Max error aloud from primary clock source.
      char    refId[4];        // 32 bits. Reference clock identifier as char[4]

      uint8_t refTm_s[4];      // 32 bits. Reference time-stamp seconds.
      uint8_t refTm_f[4];      // 32 bits. Reference time-stamp fraction of a second.

      uint8_t origTm_s[4];     // 32 bits. Originate time-stamp seconds.
      uint8_t origTm_f[4];     // 32 bits. Originate time-stamp fraction of a second.

      uint8_t rxTm_s[4];       // 32 bits. Received time-stamp seconds.
      uint8_t rxTm_f[4];       // 32 bits. Received time-stamp fraction of a second.

      uint8_t txTm_s[4];      // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
      uint8_t txTm_f[4];      // 32 bits. Transmit time-stamp fraction of a second.

    } packet;              // Total: 384 bits or 48 bytes.

    uint64_t t3;

    NTP() : packet( (ntp_packet){ 0b00011011, 0, 0, -10, 0, 0, "", 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 } ) {}

};

