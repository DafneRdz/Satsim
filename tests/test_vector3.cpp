#include "test_framework.hpp"
#include "orbit/Vector3.hpp"

using satsim::Vector3;

TEST_CASE(vector_addition_and_subtraction) {
    Vector3 a(1.0, 2.0, 3.0);
    Vector3 b(4.0, -1.0, 0.5);

    Vector3 sum = a + b;
    CHECK_NEAR(sum.x, 5.0, 1e-9);
    CHECK_NEAR(sum.y, 1.0, 1e-9);
    CHECK_NEAR(sum.z, 3.5, 1e-9);

    Vector3 diff = a - b;
    CHECK_NEAR(diff.x, -3.0, 1e-9);
    CHECK_NEAR(diff.y, 3.0, 1e-9);
    CHECK_NEAR(diff.z, 2.5, 1e-9);
}

TEST_CASE(vector_norm_and_normalization) {
    Vector3 v(3.0, 4.0, 0.0);
    CHECK_NEAR(v.norm(), 5.0, 1e-9);

    Vector3 unit = v.normalized();
    CHECK_NEAR(unit.norm(), 1.0, 1e-9);
    CHECK_NEAR(unit.x, 0.6, 1e-9);
    CHECK_NEAR(unit.y, 0.8, 1e-9);
}

TEST_CASE(vector_dot_product) {
    Vector3 a(1.0, 0.0, 0.0);
    Vector3 b(0.0, 1.0, 0.0);
    CHECK_NEAR(a.dot(b), 0.0, 1e-9); // orthogonal vectors

    Vector3 c(2.0, 3.0, 4.0);
    Vector3 d(1.0, 1.0, 1.0);
    CHECK_NEAR(c.dot(d), 9.0, 1e-9);
}

TEST_CASE(vector_cross_product) {
    Vector3 xAxis(1.0, 0.0, 0.0);
    Vector3 yAxis(0.0, 1.0, 0.0);
    Vector3 zAxis = xAxis.cross(yAxis);

    CHECK_NEAR(zAxis.x, 0.0, 1e-9);
    CHECK_NEAR(zAxis.y, 0.0, 1e-9);
    CHECK_NEAR(zAxis.z, 1.0, 1e-9);
}

TEST_CASE(vector_angle_between) {
    Vector3 a(1.0, 0.0, 0.0);
    Vector3 b(0.0, 1.0, 0.0);
    CHECK_NEAR(a.angleTo(b), 3.14159265358979323846 / 2.0, 1e-6); // 90 degrees
}

TEST_CASE(vector_distance) {
    Vector3 a(0.0, 0.0, 0.0);
    Vector3 b(3.0, 4.0, 0.0);
    CHECK_NEAR(a.distanceTo(b), 5.0, 1e-9);
}
