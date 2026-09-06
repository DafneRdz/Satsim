#pragma once

#include <cstdint>
#include <string>

namespace satsim {

// Represents an instant in time as a Julian Date (days since noon, Jan 1, 4713 BC).
// Orbital mechanics conventionally works in Julian Date / centuries, so we
// standardize on this instead of calendar dates internally.
class JulianDate {
public:
    explicit JulianDate(double jd = 0.0) : jd_(jd) {}

    // Construct from a UTC calendar date/time (Gregorian calendar).
    static JulianDate fromCalendar(int year, int month, int day,
                                    int hour = 0, int minute = 0, double second = 0.0);

    double value() const { return jd_; }

    // Days since the J2000.0 epoch (Jan 1, 2000, 12:00 TT), commonly used
    // as the reference epoch for orbital element sets.
    double daysSinceJ2000() const { return jd_ - 2451545.0; }

    // Julian centuries since J2000.0 - used in many astronomical formulas.
    double centuriesSinceJ2000() const { return daysSinceJ2000() / 36525.0; }

    JulianDate addSeconds(double seconds) const { return JulianDate(jd_ + seconds / 86400.0); }
    JulianDate addDays(double days) const { return JulianDate(jd_ + days); }

    double secondsSince(const JulianDate& other) const { return (jd_ - other.jd_) * 86400.0; }

    bool operator<(const JulianDate& rhs) const { return jd_ < rhs.jd_; }
    bool operator>(const JulianDate& rhs) const { return jd_ > rhs.jd_; }
    bool operator<=(const JulianDate& rhs) const { return jd_ <= rhs.jd_; }
    bool operator>=(const JulianDate& rhs) const { return jd_ >= rhs.jd_; }
    bool operator==(const JulianDate& rhs) const { return jd_ == rhs.jd_; }

    // Greenwich Mean Sidereal Time in radians - needed to rotate between
    // the Earth-Centered Inertial (ECI) and Earth-Centered Earth-Fixed (ECEF)
    // frames, i.e. to know where a satellite is relative to the ground.
    double gmstRadians() const;

    std::string toIsoString() const;

private:
    double jd_;
};

} // namespace satsim
