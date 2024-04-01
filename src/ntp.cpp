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

#include "ntp.h"

#include <cstdio>
#include <cmath>

#define MS1900(A, B) ( ((((A[0] * 256UL + A[1]) * 256UL + A[2]) * 256UL + A[3]) * 1000000ULL) + (((((B[0] * 256UL + B[1]) * 256UL + B[2]) * 256UL + B[3]) * 1000000ULL) >> 32) )
#define MAXSTRAT 16

NTP::NTP() : packet( (ntp_packet){ 0, 0, 0, 0, 0, 0, "", 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 } ) {}

NTP NTP::makeNTP(const NtpMode mode, const byte version) {
  NTP result;
  // result.packet.li_vn_mode = (version % 8) << 3 + (mode % 8);
  result.packet.li_vn_mode = 0b00011011;
  result.packet.stratum = MAXSTRAT;
  result.packet.precision = -10;
  return result;
}

const uint8_t* NTP::packetAddr() const {
  return (const uint8_t*)&packet;
}

byte NTP::packetSize() {
  return sizeof(ntp_packet);
}

const char* NTP::getHeader() const {
  static char buffer[100];
  snprintf(buffer, 100, "Mode %d ; Vers. %d ; LI %d ; Stratum %d", 
    packet.li_vn_mode & 0b0111, 
    (packet.li_vn_mode >> 3) & 0b0111, 
    (packet.li_vn_mode >> 6) & 0b011, 
    packet.stratum);
  return buffer;
}

NtpMode NTP::getMode() const {
  return NtpMode((packet.li_vn_mode) % 8);
}

byte NTP::getVersion() const {
  return (packet.li_vn_mode >> 3) % 8;
}

unsigned NTP::getPolling() const {
  return 1UL << packet.poll;
}

double NTP::getPrecision() const {
  return pow(double(2), double(packet.precision));
}

const char* NTP::getId() const {
  static char id[30];
  return packet.refId;
}

const char* NTP::getIP() const {
  static char id[30];
  snprintf(id, 30, "%d.%d.%d.%d", packet.refId[0], packet.refId[1], packet.refId[2], packet.refId[3]);
  return id;
}

uint64_t NTP::getT0() const {
  return MS1900(packet.origTm_s, packet.origTm_f);
}

uint64_t NTP::getT1() const {
  return MS1900(packet.rxTm_s, packet.rxTm_f);
} 

uint64_t NTP::getT2() const {
  return MS1900(packet.txTm_s, packet.txTm_f);
}

uint64_t NTP::getT3() const {
  return t3;
}

int64_t NTP::getOffset() const {
//  Serial.printf("T0: %llu - T1: %llu - T2: %llu - T3: %llu\n", getT0(), getT1(), getT2(), getT3());
  const int64_t diff1 = getT1() - getT0();
  const int64_t diff2 = getT2() - getT3();
//  Serial.printf("T1-T0: %lld - T2-T3: %lld - Off: %lld\n", diff1, diff2, (diff1 + diff2)/2);
  return (diff1 + diff2) / 2;
}      

unsigned long NTP::getRTT() const {
//  Serial.printf("T0: %llu - T1: %llu - T2: %llu - T3: %llu\n", getT0(), getT1(), getT2(), getT3());
  const uint64_t diff1 = getT3() - getT0();
  const uint64_t diff2 = getT2() - getT1();
//  Serial.printf("T3-T0: %llu - T2-T1: %llu - RTT: %llu\n", diff1, diff2, diff1 - diff2);
  return diff1 - diff2; // must be > 0
}
