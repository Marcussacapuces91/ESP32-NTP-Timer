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
#include <ctime>

enum week_t { Last, First, Second, Third, Fourth };
enum dow_t { Sun=0, Mon, Tue, Wed, Thu, Fri, Sat };
enum month_t { Jan=0, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec };

struct TimeChangeRule {
    char abbrev[6];    // five chars max
    uint8_t week;      // First, Second, Third, Fourth, or Last week of the month
    uint8_t dow;       // day of week, 1=Sun, 2=Mon, ... 7=Sat
    uint8_t month;     // 1=Jan, 2=Feb, ... 12=Dec
    uint8_t hour;      // 0-23
    int offset;        // offset from UTC in minutes
};

class Timezone {
  public:
    Timezone(const TimeChangeRule& aStd, const TimeChangeRule& aDst);
    time_t localtime(const time_t& utc) const;

  protected:
/**
 * Retourne l'heure du changement horaire.
 * @param rule Règle de changement d'heure.
 * @param year Année courante depuis 1900.
 * @return Un entier représentant le temps unix (@see mktime).
 */
    static time_t getChange(const TimeChangeRule rule, const int year);

  private:
    const TimeChangeRule& std;
    const TimeChangeRule& dst;
    
};
