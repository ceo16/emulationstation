#pragma once
#ifndef ES_CORE_UTILS_TIME_UTIL_H
#define ES_CORE_UTILS_TIME_UTIL_H

#include <string>
#include <ctime> // Per time_t e struct tm

// Forward declaration
struct tm;

namespace Utils
{
	namespace Time
	{
		static const time_t NOT_A_DATE_TIME = 0;

		class DateTime
		{
		public:
			 static DateTime now();

			 DateTime();
			 DateTime(time_t _time);
			 DateTime(const std::string& _isoString);
			 // DateTime(const tm& _timeStruct); // Rimosso per semplicità
			~DateTime();

			// Dichiarazioni Operatori (senza corpo inline)
			bool operator<(const DateTime& _other) const;
			bool operator<=(const DateTime& _other) const;
			bool operator>(const DateTime& _other) const;
			bool operator>=(const DateTime& _other) const;
			operator time_t() const;
			operator std::string() const; // Dichiarazione operatore conversione a stringa ISO

			// Dichiarazioni Getters pubblici (senza corpo inline, const corretto)
			time_t             getTime() const;
			tm                 getTimeStruct() const; // Restituisce COPIA
			const std::string& getIsoString() const;
			std::string        toLocalTimeString() const;
			double             elapsedSecondsSince(const DateTime& _since) const;
			bool               isValid() const; // const corretto

            // Dichiarazioni Setters pubblici
            void setTime(time_t _time);
			void setTimeStruct(const tm& _timeStruct);
			void setIsoString (const std::string& _isoString);

		private:
			time_t      mTime;
			tm          mTimeStruct; // Contiene la versione locale
			std::string mIsoString;  // Contiene la versione ISO standard
		}; // DateTime

		class Duration // Dichiarazioni Duration (assicurati corrispondano al tuo originale)
		{
		public:
			 Duration(time_t _time);
			~Duration();
			unsigned int getDays() const;
			unsigned int getHours() const;
			unsigned int getMinutes() const;
			unsigned int getSeconds() const;
		private:
			unsigned int mTotalSeconds;
			unsigned int mDays;
			unsigned int mHours;
			unsigned int mMinutes;
			unsigned int mSeconds;
		}; // Duration

		// Dichiarazioni Funzioni Standalone
		time_t      now         ();
		time_t      stringToTime(const std::string& _string, const std::string& _format = "%Y%m%dT%H%M%S");
		std::string timeToString(time_t _time, const std::string& _format = "%Y%m%dT%H%M%S");
		int         daysInMonth (int _year, int _month);
		int         daysInYear  (int _year);
		std::string secondsToString(long seconds, bool asTime = false);
		std::string getSystemDateFormat();
		std::string getElapsedSinceString(time_t _time);
		time_t      iso8601ToTime(const std::string& iso_string);
		std::string timeToMetaDataString(time_t timestamp);

	} // Namespace Time::
} // Namespace Utils::

#endif // ES_CORE_UTILS_TIME_UTIL_H