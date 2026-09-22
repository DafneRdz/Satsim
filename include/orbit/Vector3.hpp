#pragma once

#include <cmath>
#include <ostream>

namespace satsim {

// A simple 3D vector class used for positions (km), velocities (km/s),
// and general math throughout the orbital mechanics module.
class Vector3 {
public:
    double x, y, z;

    constexpr Vector3() : x(0.0), y(0.0), z(0.0) {}
    constexpr Vector3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    // --- Arithmetic operators ---
    Vector3 operator+(const Vector3& rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    Vector3 operator-(const Vector3& rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    Vector3 operator-() const { return {-x, -y, -z}; }
    Vector3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vector3 operator/(double s) const { return {x / s, y / s, z / s}; }

    Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
    Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
    Vector3& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }
    Vector3& operator/=(double s) { x /= s; y /= s; z /= s; return *this; }

    bool operator==(const Vector3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
    bool operator!=(const Vector3& rhs) const { return !(*this == rhs); }

    // --- Vector operations ---
    double dot(const Vector3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }

    Vector3 cross(const Vector3& rhs) const {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        };
    }

    double normSquared() const { return dot(*this); }
    double norm() const { return std::sqrt(normSquared()); }

    Vector3 normalized() const {
        double n = norm();
        if (n < 1e-15) return {0.0, 0.0, 0.0};
        return *this / n;
    }

    double angleTo(const Vector3& rhs) const {
        double denom = norm() * rhs.norm();
        if (denom < 1e-15) return 0.0;
        double cosAngle = dot(rhs) / denom;
        // Clamp to avoid domain errors from floating point drift.
        if (cosAngle > 1.0) cosAngle = 1.0;
        if (cosAngle < -1.0) cosAngle = -1.0;
        return std::acos(cosAngle);
    }

    double distanceTo(const Vector3& rhs) const { return (*this - rhs).norm(); }

    friend Vector3 operator*(double s, const Vector3& v) { return v * s; }

    friend std::ostream& operator<<(std::ostream& os, const Vector3& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

} // namespace satsim
