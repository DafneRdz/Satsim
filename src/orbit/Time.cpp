#include "orbit/Time.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

namespace satsim {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

double normalizeRadians(double angle) {
    double result = std::fmod(angle, kTwoPi);
    if (result < 0.0) result += kTwoPi;
    return result;
}
} // namespace

JulianDate JulianDate::fromCalendar(int year, int month, int day,
                                     int hour, int minute, double second) {
    // Standard algorithm (Fliegel & Van Flandern) for Julian Day Number,
    // extended with the fractional day from the time-of-day.
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;

    long jdn = day + (153 * m + 2) / 5 + 365L * y + y / 4 - y / 100 + y / 400 - 32045;

    double dayFraction = (hour - 12) / 24.0 + minute / 1440.0 + second / 86400.0;

    return JulianDate(static_cast<double>(jdn) + dayFraction);
}

double JulianDate::gmstRadians() const {
    // IAU 1982 GMST formula, in seconds of time, then converted to radians.
    double T = centuriesSinceJ2000();

    double gmstSeconds = 67310.54841
        + (876600.0 * 3600.0 + 8640184.812866) * T
        + 0.093104 * T * T
        - 6.2e-6 * T * T * T;

    // Convert seconds -> degrees (360 deg / 86400 s = 1/240) -> radians.
    double gmstDegrees = std::fmod(gmstSeconds / 240.0, 360.0);
    if (gmstDegrees < 0.0) gmstDegrees += 360.0;

    return normalizeRadians(gmstDegrees * kPi / 180.0);
}

std::string JulianDate::toIsoString() const {
    // Convert back to a Gregorian calendar date for human-readable output.
    double jdShifted = jd_ + 0.5;
    long Z = static_cast<long>(jdShifted);
    double F = jdShifted - Z;

    long A = Z;
    if (Z >= 2299161) {
        long alpha = static_cast<long>((Z - 1867216.25) / 36524.25);
        A = Z + 1 + alpha - alpha / 4;
    }
    long B = A + 1524;
    long C = static_cast<long>((B - 122.1) / 365.25);
    long D = static_cast<long>(365.25 * C);
    long E = static_cast<long>((B - D) / 30.6001);

    double dayWithFraction = B - D - static_cast<long>(30.6001 * E) + F;
    int day = static_cast<int>(dayWithFraction);
    int month = (E < 14) ? E - 1 : E - 13;
    int year = (month > 2) ? C - 4716 : C - 4715;

    double fracDay = dayWithFraction - day;
    int totalSeconds = static_cast<int>(std::round(fracDay * 86400.0));
    int hour = totalSeconds / 3600;
    int minute = (totalSeconds % 3600) / 60;
    int second = totalSeconds % 60;

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << year << "-"
        << std::setw(2) << month << "-"
        << std::setw(2) << day << "T"
        << std::setw(2) << hour << ":"
        << std::setw(2) << minute << ":"
        << std::setw(2) << second << "Z";
    return oss.str();
}

} // namespace satsim
