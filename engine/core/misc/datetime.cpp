#include "datetime.h"

#include <chrono>
#include <cassert>
#include <algorithm>
#include <sstream>


namespace lexgine::core::misc {

namespace {

template<typename Duration>
DateTime convertSystemTimePointToDateTime(std::chrono::time_point<std::chrono::system_clock, Duration> const& tp, std::optional<std::string> const& time_zone)
{
    std::chrono::sys_days system_days = std::chrono::time_point_cast<std::chrono::days>(tp);
    std::string tz = time_zone.value_or(std::string{ std::chrono::current_zone()->name() });
    std::chrono::zoned_time ztime{ tz, system_days };

    auto local_tp = ztime.get_local_time();
    std::chrono::year_month_day ymd{ std::chrono::time_point_cast<std::chrono::days>(local_tp) };
    int year = static_cast<int>(ymd.year());
    uint8_t month = static_cast<uint8_t>(static_cast<unsigned int>(ymd.month()));
    uint8_t day = static_cast<uint8_t>(static_cast<unsigned int>(ymd.day()));

    auto hours_fraction = local_tp - std::chrono::floor<std::chrono::days>(local_tp);
    auto hours = std::chrono::floor<std::chrono::hours>(hours_fraction);

    auto minutes_fraction = hours_fraction - hours;
    auto minutes = std::chrono::floor<std::chrono::minutes>(minutes_fraction);

    auto seconds_fraction = minutes_fraction - minutes;
    auto seconds = std::chrono::floor<std::chrono::seconds>(seconds_fraction);

    return DateTime{
        year,
        static_cast<Month>(month - 1),
        day,
        static_cast<uint8_t>(hours.count()),
        static_cast<uint8_t>(minutes.count()),
        static_cast<uint8_t>(seconds.count()),
        tz
    };
}

long long floordiv(long long a, long long b)
{
    assert(b != 0);
    long long q = a / b;
    long long r = a % b;
    if (r < 0) --q;
    return q;
}

long long floormod(long long a, long long b)
{
    assert(b != 0);
    long long r = a % b;
    if (r < 0) r += b;
	return r;
}

}

TimeSpan::TimeSpan() : m_years{ 0 }, m_months{ 0 }, m_days{ 0 }, m_hours{ 0 }, m_minutes{ 0 }, m_seconds{ 0 }
{

}

TimeSpan::TimeSpan(int years, int months, int days, int hours, int minutes, int seconds) :
    m_years{ years }, m_months{ months }, m_days{ days }, m_hours{ hours }, m_minutes{ minutes }, m_seconds{ seconds }
{

}

int TimeSpan::years() const { return m_years; }

int TimeSpan::months() const { return m_months; }

int TimeSpan::days() const { return m_days; }

int TimeSpan::hours() const { return m_hours; }

int TimeSpan::minutes() const { return m_minutes; }

int TimeSpan::seconds() const { return m_seconds; }

TimeSpan TimeSpan::operator+(TimeSpan const& other) const
{
    return TimeSpan{ m_years + other.m_years, m_months + other.m_months, m_days + other.m_days,
        m_hours + other.m_hours, m_minutes + other.m_minutes, m_seconds + other.m_seconds };
}

TimeSpan TimeSpan::operator-(TimeSpan const& other) const
{
    return TimeSpan{ m_years - other.m_years, m_months - other.m_months, m_days - other.m_days,
        m_hours - other.m_hours, m_minutes - other.m_minutes, m_seconds - other.m_seconds };
}

TimeSpan TimeSpan::operator-() const
{
    return TimeSpan{ -m_years, -m_months, -m_days, -m_hours, -m_minutes, -m_seconds };
}


DateTime::DateTime() : DateTime{ 1970, Month::january, 1, 0, 0, 0, "UTC" }
{
}

DateTime::DateTime(int year, Month month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second,
    std::optional<std::string> const& time_zone /* = nullopt */)
    : m_ymd{ std::chrono::year{year} / std::chrono::month{static_cast<unsigned int>(month) + 1} / std::chrono::day{day}}
	, m_hour{ hour }
	, m_minute{ minute }
	, m_second{ second }
    , m_ztime{
        time_zone.has_value() ? std::chrono::locate_zone(time_zone.value()) : std::chrono::current_zone(),
        static_cast<std::chrono::local_days>(m_ymd)
    }
    , m_time_info{ m_ztime.get_info() }
    , m_is_dts{ m_time_info.save != std::chrono::minutes{0} } 
{

}

DateTime::DateTime(unsigned long long nanoseconds)
{
    *this = DateTime::convertNanosecondsToDate(nanoseconds);
}

int DateTime::getTimeZoneOffset() const 
{
    return std::chrono::duration_cast<std::chrono::hours>(m_time_info.offset).count() - static_cast<int>(m_is_dts); 
}

DateTime DateTime::getUTC() const
{
    return getLocalTime("UTC");
}

DateTime DateTime::getLocalTime(std::string const& time_zone) const
{
    return convertSystemTimePointToDateTime(m_ztime.get_sys_time(), time_zone);
}

DateTime DateTime::operator+(TimeSpan const& span) const
{
    int y = year();
    int m = static_cast<int>(month());
    int d = static_cast<int>(day());
    int h = static_cast<int>(hour());
    int mi = static_cast<int>(minute());
    int s = static_cast<int>(second());

    y += span.years();
    m += span.months();
    d += span.days();
    h += span.hours();
    mi += span.minutes();
    s += span.seconds();

    long long time_of_day = h * 3600LL + mi * 60LL + s;
    long long total_hours = floordiv(time_of_day, 3600LL);
    long long spillover_seconds = floormod(time_of_day, 3600LL);
	h = static_cast<int>(floormod(total_hours, 24LL));
	mi = static_cast<int>(spillover_seconds / 60LL);
	s = static_cast<int>(spillover_seconds % 60LL);

    y += floordiv(m, 12LL);
	m = static_cast<int>(floormod(m, 12LL));
    
    std::chrono::year_month_day ymd = std::chrono::year{ y } / std::chrono::month{ static_cast<unsigned int>(m + 1) } / std::chrono::day{ 1 };
    std::chrono::sys_days sdays = static_cast<std::chrono::sys_days>(ymd);
    sdays += std::chrono::days{ floordiv(total_hours, 24LL) + d - 1 };
    ymd = std::chrono::year_month_day{ sdays };
    y = static_cast<int>(ymd.year());
    m = static_cast<int>(static_cast<unsigned int>(ymd.month())) - 1;
    d = static_cast<int>(static_cast<unsigned int>(ymd.day()));
    
    return DateTime{
        y,
        static_cast<Month>(m),
        static_cast<uint8_t>(d),
        static_cast<uint8_t>(h),
        static_cast<uint8_t>(mi),
        static_cast<uint8_t>(s),
        std::string{m_ztime.get_time_zone()->name()}
    };
}

DateTime DateTime::operator-(TimeSpan const& span) const
{
    return *this + (-span);
}

bool DateTime::operator>=(DateTime const& other) const
{
    return m_ztime.get_sys_time() >= other.m_ztime.get_sys_time();
}

bool DateTime::operator<=(DateTime const& other) const
{
    return m_ztime.get_sys_time() <= other.m_ztime.get_sys_time();
}

bool DateTime::operator<(DateTime const & other) const
{
    return m_ztime.get_sys_time() < other.m_ztime.get_sys_time();
}

bool DateTime::operator>(DateTime const & other) const
{
    return m_ztime.get_sys_time() > other.m_ztime.get_sys_time();
}

bool DateTime::operator==(DateTime const& other) const
{
    return m_ztime.get_sys_time() == other.m_ztime.get_sys_time();
}

Weekday DateTime::weekday() const
{
    std::chrono::year_month_weekday ymw{ static_cast<std::chrono::sys_days>(m_ymd) };
    return static_cast<Weekday>(ymw.weekday().c_encoding());
}

TimeSpan DateTime::timeTill(DateTime const& other) const
{
    DateTime const* p_lhs{ this };
    DateTime const* p_rhs{ &other };
	DateTime utcLhs{}, utcRhs{};
    bool same_tzone = m_ztime.get_time_zone() == other.m_ztime.get_time_zone();
    if (!same_tzone)
    {
        utcLhs = this->getUTC();
        utcRhs = other.getUTC();
        p_lhs = &utcLhs;
        p_rhs = &utcRhs;
    }
    bool normal_order = *p_lhs <= *p_rhs;
	DateTime const& lhs = normal_order ? *p_lhs : *p_rhs;
	DateTime const& rhs = normal_order ? *p_rhs : *p_lhs;

    int y = rhs.year() - lhs.year();
    int m = 0;
    {
        DateTime anniversary_date{ lhs.year() + y, lhs.month(), lhs.day(), lhs.hour(), lhs.minute(), lhs.second(), std::string{lhs.m_ztime.get_time_zone()->name()} };
        if (anniversary_date > rhs)
        {
            // move one year back if the full year anniversary is past the final date and account for the remaining months from the past year
            --y;
            m += 12 - static_cast<int>(lhs.month()) + static_cast<int>(rhs.month());
        }
        else
        {
			m += static_cast<int>(rhs.month()) - static_cast<int>(lhs.month());
        }
    }

    long long diff_seconds{};
    int days_in_month[] = { 31, rhs.isLeapYear() ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    {
		int rhs_month = static_cast<int>(rhs.month());
        int anchor_day = (std::min)(static_cast<int>(lhs.day()), days_in_month[rhs_month]);
        DateTime anchor_date{ 
            rhs.year(), 
            rhs.month(), 
            static_cast<uint8_t>(anchor_day), 
            lhs.hour(), 
            lhs.minute(), 
            lhs.second(), 
            std::string{rhs.m_ztime.get_time_zone()->name()} 
        };

        int rhs_day = static_cast<int>(rhs.day());
        if (anchor_date > rhs)
        {
            --m;
            int prev_month = static_cast<int>(floormod(rhs_month - 1LL, 12LL));
            anchor_day = (std::min)(static_cast<int>(lhs.day()), days_in_month[prev_month]);
			diff_seconds += (days_in_month[prev_month] - anchor_day + rhs_day) * 86400LL;
        }
        else
        {
			diff_seconds += (rhs_day - anchor_day) * 86400LL;
        }
    }

    // Calculate difference between lhs moved forward by y years and m months and rhs as the total number of seconds
    long long lhs_tod = lhs.hour() * 3600LL + lhs.minute() * 60LL + lhs.second();
    long long rhs_tod = rhs.hour() * 3600LL + rhs.minute() * 60LL + rhs.second();
    diff_seconds = diff_seconds + rhs_tod - lhs_tod;

    int d = static_cast<int>(floordiv(diff_seconds, 86400LL));
    diff_seconds = floormod(diff_seconds, 86400LL);

    int h = static_cast<int>(floordiv(diff_seconds, 3600LL));
    diff_seconds = floormod(diff_seconds, 3600LL);

    int mi = static_cast<int>(floordiv(diff_seconds, 60LL));
    int s = static_cast<int>(floormod(diff_seconds, 60LL));

    assert(y >= 0);
    assert(m >= 0);
    assert(d >= 0);
    assert(h >= 0);
    assert(mi >= 0);
    assert(s >= 0);
    TimeSpan rv{ y, m, d, h, mi, s };
    return normal_order ? rv : -rv;
}

DateTime DateTime::now(std::optional<std::string> const& time_zone/* = std::nullopt*/)
{
    auto current_time_point = std::chrono::system_clock::now();
    return convertSystemTimePointToDateTime(current_time_point, time_zone);
}

DateTime DateTime::convertNanosecondsToDate(unsigned long long nanoseconds, std::optional<std::string> const& time_zone/* = std::nullopt*/)
{
    std::chrono::nanoseconds ns{ nanoseconds };
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> system_tp{ ns };
    return convertSystemTimePointToDateTime(system_tp, time_zone);
}

std::string DateTime::toString(unsigned char mask, DateOutputStyle style) const
{
    static std::string month_name[] = { "January", "February", 
        "March", "April", "May", 
        "June", "July", "August",
        "September", "October", "November", 
        "December" };

    std::ostringstream output_string_stream{};
    bool preceeding_date_token_exists = false;

    switch (style)
    {
    case DateOutputStyle::european:
        if (mask & DateOutputMask::day)
        {
            output_string_stream << std::to_string(day());
            preceeding_date_token_exists = true;
        }

        if (mask & DateOutputMask::month)
        {
            if (preceeding_date_token_exists) output_string_stream << "/";

            output_string_stream << month_name[static_cast<int>(month())];
            preceeding_date_token_exists = true;
        }

        break;

    case DateOutputStyle::american:
        if (mask & DateOutputMask::month)
        {
            output_string_stream << month_name[static_cast<int>(month())];
            preceeding_date_token_exists = true;
        }

        if (mask & DateOutputMask::day)
        {
            if (preceeding_date_token_exists) output_string_stream << "/";

            output_string_stream << std::to_string(day());
            preceeding_date_token_exists = true;
        }

        break;
    }

    if (mask & DateOutputMask::year)
    {
        if (preceeding_date_token_exists) output_string_stream << "/";
        output_string_stream << std::to_string(year());
        preceeding_date_token_exists = true;
    }


    bool preceding_time_token_exists = false;
    if (mask & DateOutputMask::hour)
    {
        if (preceeding_date_token_exists) output_string_stream << "/ (";
        output_string_stream << std::to_string(m_hour);
        preceding_time_token_exists = true;
    }

    if (mask & DateOutputMask::minute)
    {
        if (preceeding_date_token_exists && !preceding_time_token_exists)
            output_string_stream << "/ (";
        else if (preceding_time_token_exists)
            output_string_stream << ":";
        if (m_minute < 10) output_string_stream << "0";
        output_string_stream << std::to_string(m_minute);
        preceding_time_token_exists = true;
    }

    if (mask & DateOutputMask::second)
    {
        if (preceeding_date_token_exists && !preceding_time_token_exists)
            output_string_stream << "/ (";
        else if (preceding_time_token_exists)
            output_string_stream << ":";
        if (m_second < 10) output_string_stream << "0";
        output_string_stream << std::to_string(m_second) << ")";
    }

    return output_string_stream.str();
}


}