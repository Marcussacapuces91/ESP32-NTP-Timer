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

#include "timezone.h"
// #include <Arduino.h>

Timezone::Timezone(const TimeChangeRule& aStd, const TimeChangeRule& aDst) :
  std(aStd),
  dst(aDst)
{}

time_t Timezone::localtime(const time_t& utc) const {
  struct tm tmUTC;
  gmtime_r(&utc, &tmUTC);
  auto stdEpoch = getChange(std, tmUTC.tm_year);
  auto dstEpoch = getChange(dst, tmUTC.tm_year);

  if ((stdEpoch < utc) && (dstEpoch < utc)) {
    return utc + (stdEpoch < dstEpoch ? dst.offset : std.offset) * 60;
  }

  if ((stdEpoch < utc) && (dstEpoch > utc))
    return utc + std.offset * 60;
  if ((stdEpoch > utc) && (dstEpoch < utc))
    return utc + dst.offset * 60;

  stdEpoch = getChange(std, tmUTC.tm_year - 1);
  dstEpoch = getChange(dst, tmUTC.tm_year - 1);
  return utc + (stdEpoch < dstEpoch ? dst.offset : std.offset) * 60;
}

time_t Timezone::getChange(const TimeChangeRule rule, const int year) {
  struct tm tm;
  tm.tm_hour = rule.hour;
  tm.tm_year = year;

//  Serial.println(asctime(&tm));

  if (rule.week == Last) {
    tm.tm_mday = 0;
    tm.tm_mon = rule.month + 1;
    
    auto epoch = mktime(&tm); // Normalize to last day of the month.
    while (tm.tm_wday != rule.dow) {
      --tm.tm_mday;
      epoch = mktime(&tm);
    }
    return epoch;
  }

  tm.tm_mday = 1;
  tm.tm_mon = rule.month;

  auto epoch = mktime(&tm); // Normalize to last day of the month.
  while (tm.tm_wday != rule.dow) {
    ++tm.tm_mday;
    epoch = mktime(&tm);
  }
  switch (rule.week) {
    case First:
      return epoch;
    case Second:
      tm.tm_mday += 7;
      return mktime(&tm);
    case Third:
      tm.tm_mday += 14;
      return mktime(&tm);
    case Fourth:
      tm.tm_mday += 21;
      return mktime(&tm);
  }
}
