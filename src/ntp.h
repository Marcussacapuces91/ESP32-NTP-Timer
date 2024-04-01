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

/**
 * Définit l'écart entre le 1er janvier 1900 (zéro NTP) et le 1er janvier 1970 (zéro Unix).
 */
 #define YEAR1970 2208988800

/**
 * Listes des modes NTP 
 *  0 reserved
 *  1 symmetric active
 *  2 symmetric passive
 *  3 client
 *  4 server
 *  5 broadcast
 *  6 NTP control message
 *  7 reserved for private use
 */
 enum NtpMode { NTPMODE_RESERVED = 0, NTPMODE_SYMMETRIC_ACTIVE = 1, NTPMODE_SYMMETRIC_PASSIVE = 2, NTPMODE_CLIENT = 3, NTPMODE_SERVER = 4, NTPMODE_BROADCAST = 5, NTPMODE_CONTROL_MESSAGE = 6, NTPMODE_PRIVATE_USE = 7 }; 

/**
 * La classe NTP encapsule les fonctionnalités liées à la communication avec le serveur NTP.
 * @see https://fr.wikipedia.org/wiki/Network_Time_Protocol
 * @see https://www.ntp.org/reflib/reports/ntp4/ntp4.pdf
 */
class NTP {
  public:
/**
  * Forge un packet NTP V3 client vide.
  * @param mode Valeur du mode.
  * @param version Valeur de la version du protocole, par défaut 4.
  * @return Une instance NTP.
  */
    static NTP makeNTP(const NtpMode mode, const byte version = 4);


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

/**
 * Retourne le mode utilisé par le protocole NTP.
 * @return Un entier indiquant le protocole selon les valeurs de type NtpMode.
 */
    NtpMode getMode() const;

/**
 * Retourne la version du protocole NTP utilisé.
 * @return Un entier indiquant la version définie.
 */
    byte getVersion() const;

/**
 * Retourne le temps entre 2 demandes acceptable par le serveur.
 * @return Un temps en secondes.
 */
    unsigned getPolling() const;

/**
 * Retourne la précision indiquée par le serveur.
 * @return Une valeur décimale en secondes (fractions).
 */
    double getPrecision() const;

/**
 * Retourne l'identifiant du serveur.
 * @return Une chaine de caractères indiquant l'identifiant.
 */    
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
 * Round-trip delay time (RTT).
 * @return Le temps d'aller/retour du packet NTP en microsecondes.
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

