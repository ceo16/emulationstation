// File: es-core/src/utils/TimeUtil.cpp (COMPLETO E CORRETTO)

#include "utils/TimeUtil.h"
#include "utils/StringUtil.h"
#include "LocaleES.h"
#include "Log.h"

#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <locale>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#ifndef timegm
    #define timegm _mkgmtime
#endif
#elif defined(__linux__)
#include <langinfo.h>
#else
#ifndef timegm
    #warning "timegm() not found, UTC conversion may be inaccurate."
    #define timegm mktime
#endif
#endif


namespace Utils::Time
{
    // --- Implementazioni Metodi DateTime ---

    DateTime DateTime::now() { return DateTime(Utils::Time::now()); }

    DateTime::DateTime() : mTime(NOT_A_DATE_TIME), mIsoString("00000000T000000") {
        std::memset(&mTimeStruct, 0, sizeof(tm));
        mTimeStruct.tm_mday = 1; mTimeStruct.tm_isdst = -1;
    }

    DateTime::DateTime(time_t _time) { setTime(_time); }
    // DateTime::DateTime(const tm& _timeStruct) { setTimeStruct(_timeStruct); } // Rimosso
    DateTime::DateTime(const std::string& _isoString) { setIsoString(_isoString); }
    DateTime::~DateTime() {}

    // Implementazioni operatori (const corretto)
    bool DateTime::operator<(const DateTime& _other) const { return (mTime <  _other.mTime); }
    bool DateTime::operator<=(const DateTime& _other) const { return (mTime <= _other.mTime); }
    bool DateTime::operator>(const DateTime& _other) const { return (mTime >  _other.mTime); }
    bool DateTime::operator>=(const DateTime& _other) const { return (mTime >= _other.mTime); }
    DateTime::operator time_t() const { return mTime; }
    DateTime::operator std::string() const { return mIsoString; } // Definizione operatore stringa

    // Implementazioni getter (const corretto, tipo ritorno corretto)
    time_t             DateTime::getTime() const { return mTime; }
    tm                 DateTime::getTimeStruct() const { return mTimeStruct; } // Restituisce copia
    const std::string& DateTime::getIsoString() const { return mIsoString; }
    bool               DateTime::isValid() const { return mTime != NOT_A_DATE_TIME; } // Definizione con const

    // Implementazione setTime
    void DateTime::setTime(time_t _time) {
        mTime = (_time < 0) ? NOT_A_DATE_TIME : _time;
        if (!isValid()) {
             std::memset(&mTimeStruct, 0, sizeof(tm));
             mTimeStruct.tm_mday = 1; mTimeStruct.tm_isdst = -1;
             mIsoString = "00000000T000000";
             return;
        }
        #ifdef _WIN32
            errno_t err = localtime_s(&mTimeStruct, &mTime);
             if (err != 0) { std::memset(&mTimeStruct, 0, sizeof(tm)); mTime = NOT_A_DATE_TIME; }
        #else
            tm* result = localtime_r(&mTime, &mTimeStruct);
            if (result == nullptr) { std::memset(&mTimeStruct, 0, sizeof(tm)); mTime = NOT_A_DATE_TIME; }
        #endif
        // Aggiorna ISO string solo se il tempo è valido dopo localtime_r/s
        mIsoString = isValid() ? Utils::Time::timeToString(mTime) : "00000000T000000";
    }

    // Implementazione setTimeStruct
    void DateTime::setTimeStruct(const tm& _timeStruct) {
         tm tempTm = _timeStruct;
         tempTm.tm_isdst = -1;
         time_t t = mktime(&tempTm);
         setTime(t); // setTime gestirà -1 o 0
    }

    // Implementazione setIsoString
    void DateTime::setIsoString (const std::string& _isoString) {
         setTime(Utils::Time::stringToTime(_isoString));
    }

    // Implementazione toLocalTimeString (const corretto)
    std::string DateTime::toLocalTimeString() const {
        if(!isValid()) return "INVALID DATE";
        struct tm clockTstruct = mTimeStruct;
        char clockBuf[256];
        if (strftime(clockBuf, sizeof(clockBuf), "%x %R", &clockTstruct) > 0) {
            return clockBuf;
        }
        return "ERROR_FORMATTING_DATE";
    }

    // Implementazione elapsedSecondsSince (const corretto)
    double DateTime::elapsedSecondsSince(const DateTime& _since) const {
        if (!this->isValid() || !_since.isValid()) return 0.0;
        return difftime(mTime, _since.getTime());
    }


    // --- Implementazioni Metodi Duration ---
    Duration::Duration(time_t _time) {
        mTotalSeconds = (unsigned int)((_time < 0) ? 0 : _time);
        mDays         = (mTotalSeconds / 86400);
        mHours        = (mTotalSeconds / 3600) % 24;
        mMinutes      = (mTotalSeconds / 60) % 60;
        mSeconds      = mTotalSeconds % 60;
    }
    Duration::~Duration() {}
    unsigned int Duration::getDays() const { return mDays; }
    unsigned int Duration::getHours() const { return mHours; }
    unsigned int Duration::getMinutes() const { return mMinutes; }
    unsigned int Duration::getSeconds() const { return mSeconds; }


    // --- Implementazioni Funzioni Standalone ---
    time_t now() { time_t t; ::time(&t); return t; }

    time_t stringToTime(const std::string& _string, const std::string& _format) {
        std::tm timeStruct = {};
        std::memset(&timeStruct, 0, sizeof(tm));
        timeStruct.tm_isdst = -1;
        if (_string.empty() || _string == "not-a-date-time") return NOT_A_DATE_TIME;
        std::istringstream ss(_string);
        ss.imbue(std::locale::classic());
        ss >> std::get_time(&timeStruct, _format.c_str());
        if (ss.fail() || timeStruct.tm_year < 70) { return NOT_A_DATE_TIME; }
        time_t result = mktime(&timeStruct);
        return (result == (time_t)-1) ? NOT_A_DATE_TIME : result;
    }

    std::string timeToString(time_t _time, const std::string& _format) {
        if (_time == NOT_A_DATE_TIME) return "00000000T000000";
        tm timeStruct;
        #ifdef _WIN32
            errno_t err = localtime_s(&timeStruct, &_time);
            if (err != 0) return "ERROR";
        #else
            tm* result = localtime_r(&_time, &timeStruct);
            if (result == nullptr) return "ERROR";
        #endif
        char buf[256];
        if (strftime(buf, sizeof(buf), _format.c_str(), &timeStruct) == 0) {
             return "ERROR_FORMATTING";
        }
        return std::string(buf);
    }

    int daysInMonth (int _year, int _month) {
        if (_month < 0 || _month > 11) return 0;
        const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (_month == 1) { return (daysInYear(_year) == 366) ? 29 : 28; }
        return days[_month];
    }

    int daysInYear  (int _year) {
         return (((_year % 4 == 0) && (_year % 100 != 0)) || (_year % 400 == 0)) ? 366 : 365;
    }

    std::string secondsToString(const long seconds, bool asTime) {
         // Implementazione esistente (assicurati sia quella corretta dal tuo codice)
        if (seconds <= 0) return _("never");
        char buf[256];
        if (asTime) {
            int d = seconds / 86400; int h = (seconds / 3600) % 24;
            int m = (seconds / 60) % 60; int s = seconds % 60;
            if (d > 0) sprintf(buf, "%d d %02d:%02d:%02d", d, h, m, s);
            else if (h > 0) sprintf(buf, "%02d:%02d:%02d", h, m, s);
            else sprintf(buf, "%02d:%02d", m, s);
        } else {
            int d = seconds / 86400; int h = (seconds / 3600) % 24;
            int m = (seconds / 60) % 60; int s = seconds % 60;
            std::string res = "";
            if (d > 0) res += Utils::String::format(ngettext("%d day", "%d days", d), d) + " ";
            if (h > 0) res += Utils::String::format(ngettext("%d hour", "%d hours", h), h) + " ";
            if (m > 0) res += Utils::String::format(ngettext("%d minute", "%d minutes", m), m) + " ";
            if (s > 0 || res.empty()) res += Utils::String::format(ngettext("%d second", "%d seconds", s), s);
            return Utils::String::trim(res);
        }
        return std::string(buf);
    }

    std::string getSystemDateFormat() {
        static std::string value;
        if (!value.empty()) return value;
        // Implementazione esistente (assicurati sia quella corretta dal tuo codice)
        #ifdef _WIN32
            char szBuffer[256];
            if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, szBuffer, sizeof(szBuffer))) {
                value = szBuffer;
                // Applica sostituzioni per %Y, %m, %d
                value = Utils::String::replace(value, "yyyy", "%Y");
                value = Utils::String::replace(value, "yy", "%y");
                value = Utils::String::replace(value, "MM", "%m");
                value = Utils::String::replace(value, "M", "%m"); // Potrebbe servire %#m?
                value = Utils::String::replace(value, "dd", "%d");
                value = Utils::String::replace(value, "d", "%d"); // Potrebbe servire %#d?
            } else { value = "%m/%d/%Y"; } // Fallback
        #elif defined(__linux__)
            char* date = nl_langinfo(D_FMT);
            if (date && *date) value = date;
            else value = "%m/%d/%Y"; // Fallback
        #else
            value = "%m/%d/%Y"; // Fallback generico
        #endif
        return value;
    }

    std::string getElapsedSinceString(time_t _time) {
        if (_time == NOT_A_DATE_TIME || _time == (time_t)-1) return _("never");
        time_t now_t = Utils::Time::now();
        if (_time > now_t) return _("in the future");
        long seconds = (long)difftime(now_t, _time);
        if (seconds < 0) seconds = 0;
        int d = seconds / 86400; int h = (seconds / 3600) % 24;
        int m = (seconds / 60) % 60; int s = seconds % 60;
        char buf[256];
        if (d > 365) { unsigned int y = d / 365; snprintf(buf, sizeof(buf), ngettext("%u year ago", "%u years ago", y), y); }
        else if (d > 0) { snprintf(buf, sizeof(buf), ngettext("%d day ago", "%d days ago", d), d); }
        else if (h > 0) { snprintf(buf, sizeof(buf), ngettext("%d hour ago", "%d hours ago", h), h); }
        else if (m > 0) { snprintf(buf, sizeof(buf), ngettext("%d minute ago", "%d minutes ago", m), m); }
        else { snprintf(buf, sizeof(buf), ngettext("%d second ago", "%d seconds ago", s), s); }
        return std::string(buf);
    }


    // --- NUOVE Implementazioni Funzioni ---
    time_t iso8601ToTime(const std::string& iso_string) {
        if (iso_string.empty()) { return NOT_A_DATE_TIME; }
        std::tm t = {};
        std::memset(&t, 0, sizeof(tm));
        std::istringstream ss(iso_string);
        ss.imbue(std::locale::classic());
        ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            ss.clear(); ss.str(iso_string); ss.seekg(0);
            ss >> std::get_time(&t, "%Y-%m-%d");
            if (ss.fail()) { return NOT_A_DATE_TIME; }
        }
        t.tm_isdst = 0; // Assume UTC
        time_t result = (time_t)-1;
        #ifdef _WIN32
            result = _mkgmtime(&t);
        #else
            result = timegm(&t);
        #endif
        return (result == (time_t)-1) ? NOT_A_DATE_TIME : result;
    }

    std::string timeToMetaDataString(time_t timestamp) {
        if (timestamp == NOT_A_DATE_TIME || timestamp == (time_t)-1) { return ""; }
        std::tm ptm_buf;
        #ifdef _WIN32
            errno_t err = gmtime_s(&ptm_buf, &timestamp);
            std::tm* ptm = (err == 0) ? &ptm_buf : nullptr;
        #else
            std::tm* ptm = gmtime_r(&timestamp, &ptm_buf);
        #endif
        if (!ptm) { return ""; }
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y%m%dT000000", ptm);
        return std::string(buffer);
    }

} // Namespace Time::