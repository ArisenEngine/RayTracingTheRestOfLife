#pragma once
#include "rt_weekend.h"
class onb {
public:
    onb() {}

    vec3 operator[](int i) const { return axis[i]; }
    vec3& operator[](int i) { return axis[i]; }

    vec3 u() const { return axis[0]; }
    vec3 v() const { return axis[1]; }
    vec3 w() const { return axis[2]; }

    vec3 local(double a, double b, double c) const {
        return a * u() + b * v() + c * w();
    }

    vec3 local(const vec3& a) const {
        return a.x() * u() + a.y() * v() + a.z() * w();
    }

    void build_from_w(const vec3& w) {
        vec3 unit_w = normalize(w);
        vec3 a = (fabs(unit_w.x()) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
        vec3 v = normalize(cross(unit_w, a));
        vec3 u = cross(unit_w, v);
        axis[0] = u;
        axis[1] = v;
        axis[2] = unit_w;
    }

public:
    vec3 axis[3];
};