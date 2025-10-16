#ifndef LEXGINE_CORE_MISC_DATE_TIME_H

#include <cstdint>
#include <string>
#include <chrono>
#include <optional>

namespace lexgine::core::misc {

//! Time period represented by tuple of year, month, day, hour, minute and fractional second. Time spans can be negative.
//! Time span is merely a tuple, it does not define whether certain year is a leap year or a regular year nor does it define the number of days in a certain month.
//! Therefore, time spans do not support order relations. When two time spans are added their fields are added independently.
class TimeSpan
{
public:
    TimeSpan(); //! Initializes zero-length time span

    //! Initializes time span with given length of years, months, days, hours, minutes, and fractional seconds
    TimeSpan(int years, int months, int days, int hours, int minutes, int seconds);

    int years() const; //! Returns number of years in the time span
    int months() const;    //! Returns number of months in the time span
    int days() const;  //! Returns number of days in the time span
    int hours() const; //! Returns number of hours in the time span
    int minutes() const;   //! Returns number of minutes in the time span
    int seconds() const; //! Returns number of seconds in the time span

    TimeSpan& operator=(TimeSpan const& other);    //! Assigns this time span to the @param other
    TimeSpan operator+(TimeSpan const& other) const;    //! Adds @param other time span to this time span
    TimeSpan operator-(TimeSpan const& other) const;    //! Subtracts @param other time span from this time span
    TimeSpan operator-() const;  //! unary minus, reverses the sign (direction) of the time span

private:
    int m_years;
    int m_months;
    int m_days;
    int m_hours;
    int m_minutes;
    int m_seconds;
};

enum class Weekday
{
    sunday,
    monday,
    tuesday,
    wednesday,
    thursday,
    friday,
    saturday
};

enum class Month
{
    january,
    february,
    march,
    april,
    may,
    june,
    july, 
    august,
    september,
    october,
    november,
    december
};

//! Implements primitive Gregorian style date and time wrapper with the origin at January 1, 1970
//! Note that this calendar can only represent the dates after Christ
class DateTime
{
public:
    struct DateOutputMask
    {
        static unsigned char const year = 0x1;
        static unsigned char const month = 0x2;
        static unsigned char const day = 0x4;
        static unsigned char const hour = 0x8;
        static unsigned char const minute = 0x10;
        static unsigned char const second = 0x20;
    };

    enum class DateOutputStyle
    {
        european, american
    };

public:
    //! Initializes the date-time object to January 1, 1970 UTC.
    DateTime();

    //! Initializes the date-time object to the given moment in time.
    //! Here year is the year represented by the date-time object
    //! month is the month of the year encapsulated by the date-time object. The value is clamped to 12 during initialization to enforce validity of the date.
    //! day is the current day of the month. The value is clamped to the last day of month in order to enforce validity of the date.
    //! hour is the current hour of the day. The value is clamped to 23 to enforce validity of the date.
    //! minute is the current minute of the hour. The value is clamped to 59 to enforce validity of the date.
    //! second is the current second of minute. The value is clamped to 59 to enforce validity of the date.
    //! time_zone is the abbreviated name of the time zone as specified in IANA time zone database. When not provided defaults to the current time zone used by the host system.
    DateTime(int year, Month month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, std::optional<std::string> const& time_zone = std::nullopt);

    //! Constructs date-time object using @param nanoseconds passed since January 1, 1970, 00:00:00
    DateTime(unsigned long long nanoseconds);

    //! Calculates date that is later than this date by the time span determined by span.
    //! Note that opposite to that of the time span, the date time object cannot store values that are larger than 255 in its
    //! month, day, hour, and minute fields, and all of them (including the year second fields) must be non-negative. If the time span added
    //! to the date is such that this requirement is violated the results are undefined
    DateTime operator+(TimeSpan const& span) const;

    //! Calculates date that is earlier than this date by the time span determined by span.
    //! Note that opposite to that of the time span, the date time object cannot store values that are larger than 255 in its
    //! month, day, hour, and minute fields, and all of them (including the year and second fields) must be non-negative. If the time span subtracted from
    //! the date is such that this requirement is violated the results are undefined
    DateTime operator-(TimeSpan const& span) const;

    bool operator>(DateTime const& other) const;    //! returns 'true' if this date is later than the other date
    bool operator>=(DateTime const& other) const;    //! returns 'true' if this date is later or same than the other date
    bool operator<=(DateTime const& other) const;    //! returns 'true' if this date is earlier or same than the other date
    bool operator<(DateTime const& other) const;    //! returns 'true' if this date is earlier than the other date
    bool operator==(DateTime const& other) const;    //! returns 'true' if this and other dates refer to the same points in time

    int year() const { return static_cast<int>(m_ymd.year()); }
    Month month() const { return static_cast<Month>(static_cast<unsigned int>(m_ymd.month()) - 1); }
    uint8_t day() const { return static_cast<uint16_t>(static_cast<unsigned int>(m_ymd.day())); }
    Weekday weekday() const;
    uint8_t hour() const { return m_hour; }
    uint8_t minute() const { return m_minute; }
    uint8_t second() const { return m_second; }
    bool isLeapYear() const { return m_ymd.year().is_leap(); }

    int getTimeZoneOffset() const;	//! returns time shift in hours from the UNC time, without accounting for the daylight saving shift
    bool isDTS() const { return m_is_dts; };	//! returns 'true' if the time represented by the date time object is in daylight saving mode
    DateTime getUTC() const;	//! returns date and time equivalent to the date and time represented by this object converted to the UNC time zone
    DateTime getLocalTime(std::string const& time_zone) const;	//! returns date and time equivalent to the date and time represented by this object converted to requested time zone

    //! Calculates time passed since "this" date till the "other" date.
    TimeSpan timeTill(DateTime const& other) const;

    static DateTime now(std::optional<std::string> const& time_zone = std::nullopt);	//! Returns date-time object encapsulating current date and time in the given time zone. If time zone is not provided, returns current local time
    static DateTime convertNanosecondsToDate(unsigned long long nanoseconds, std::optional<std::string> const& time_zone = std::nullopt);	//! Converts amount of nanoseconds passed from January 1, 1970, 00:00:00 into date-time representation

    std::string toString(unsigned char mask = 0x3F, DateOutputStyle style = DateOutputStyle::european) const;

    //! Retrieves compilation timestamp of the calling translation unit
    static DateTime buildTime()
    {
        static std::string month_name_as_encoded_in__DATE__[] = { "Jan", "Feb", "Mar", "Apr", "May",
        "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

        uint8_t month{}, day{};
        uint16_t year{};

        uint8_t hour{}, minute{}, second{};

        int8_t utc_bias{};
        bool is_dts{};


        // TODO: __DATE__ and __TIME__ are not part of C language standard. Especially __TIME__ seems to return GMT time
        // in most cases, which is OK, but this behavior cannot be relied upon. Consider establishing more robust way to 
        // support time stamps perhaps by the means of a custom macros definition created by an external script.

        std::string __date__{ __DATE__ };
        {
            auto p = std::find_if(month_name_as_encoded_in__DATE__, month_name_as_encoded_in__DATE__ + 12,
                [&__date__](std::string const& month_name)
            {
                return month_name == __date__.substr(0, 3);
            });

            month = static_cast<uint8_t>(p - month_name_as_encoded_in__DATE__);
            day = static_cast<uint8_t>(std::atoi(__date__.substr(4, 2).c_str()));
            year = static_cast<uint16_t>(std::atoi(__date__.substr(7).c_str()));
        }

        std::string __time__{ __TIME__ };
        DateTime utc_build_time;
        {
            hour = static_cast<uint8_t>(std::atoi(__time__.substr(0, 2).c_str()));
            minute = static_cast<uint8_t>(std::atoi(__time__.substr(3, 2).c_str()));
            second = static_cast<uint8_t>(std::atoi(__time__.substr(6, 2).c_str()));
        }

        return DateTime{ year, static_cast<Month>(month), day, hour, minute, second };
    }

private:
	std::chrono::year_month_day m_ymd;
	uint8_t m_hour;
	uint8_t m_minute;
	uint8_t m_second;
    std::chrono::zoned_time<std::chrono::system_clock::duration> m_ztime;
    std::chrono::sys_info m_time_info;
    bool m_is_dts;	//!< 'true' if the time is a daylight saving time
};

}

#define LEXGINE_CORE_MISC_DATE_TIME_H
#endif
