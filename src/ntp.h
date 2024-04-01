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

#include <cstdint>
#include <cstring>
#include <arduino.h>

// #define byte unsigned char
#define YEAR1970 2208988800

class NTP {
  public:
/**
  * Forge un packet NTP V3 client vid.
  */
    static NTP makeNTP();

/**
 * Retourne un pointeur sur le paquet NTP interne.
 * @return adresse du paquet NTP.
 */
    const uint8_t* packetAddr() const;

/**
 * Retourne la taille d'un paquet NTP.
 * @return la taille en octets.
 */
    static byte packetSize();

/**
 * Retourne un chaîne de caractères décrivant l'enregistrement Header du paquet NTP.
 * @return un pointeur sur la chaîne.
 */
    const char* getHeader() const;
    byte getMode() const;
    byte getVersion() const;

/**
 * Retourne le temps entre 2 demandes acceptable par le serveur.
 * @return Un temps en secondes.
 */
    unsigned getPolling() const;
    double getPrecision() const;
    const char* getId() const;
    const char* getIP() const;
    uint64_t getT0() const;
    uint64_t getT1() const;
    uint64_t getT2() const;
    uint64_t getT3() const;

/**
 * Retourne l'offset en microsecondes.
 * @return Ecart signé en microsecondes.
 */
    int64_t getOffset() const;

/**
 * Round-trip delay time.
 */
    unsigned long getRTT() const;
    void setPacket(const uint8_t buffer[]);
    void setT0(const uint64_t& tx);
    void setT3(const uint64_t& rx);

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

    } packet;                 // Total: 384 bits or 48 bytes.

    uint64_t t3;

/**
 * Private constructor.
 */
    NTP();

};

inline void NTP::setPacket(const uint8_t buffer[]) {
  memcpy(&packet, buffer, sizeof(ntp_packet));
}

/**
 * Set Transmit Timestamp before send.
 * @param tx Transmit Timestamp in µsec since 1/1/1900.
 * @warning T0 must be write in Transmit Timestamp before sending to server.
 */
inline void NTP::setT0(const uint64_t& tx) {
  const uint32_t d = tx / 1000000;      // div
  const uint64_t m = tx - d * 1000000;  // mod

  uint32_t sec = d;
  packet.txTm_s[3] = sec & 0xFF;
  sec >>= 8;
  packet.txTm_s[2] = sec & 0xFF;
  sec >>= 8;
  packet.txTm_s[1] = sec & 0xFF;
  packet.txTm_s[0] = sec >> 8;

  uint32_t micros = (m << 32) / 1000000;
  packet.txTm_f[3] = micros & 0xFF;
  micros >>= 8;
  packet.txTm_f[2] = micros & 0xFF;
  micros >>= 8;
  packet.txTm_f[1] = micros & 0xFF;
  packet.txTm_f[0] = micros >> 8;
}

inline void NTP::setT3(const uint64_t& rx) {
  t3 = rx;
}

