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

NTP::NTP() : packet( (ntp_packet){ 0b00011011, 0, 0, -10, 0, 0, "", 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 } ) {}

NTP NTP::makeNTP() {
  NTP result;
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

byte NTP::getMode() const {
  return (packet.li_vn_mode) % 8;
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
  const int64_t diff1 = getT1() - getT0();
  const int64_t diff2 = getT2() - getT3();
  return (diff1 + diff2) / 2;
}      
