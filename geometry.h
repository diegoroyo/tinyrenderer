#pragma once

#include <cmath>
#include <ostream>

template <class T>
struct Vec2 {
    union {
        struct {
            T u, v;
        };
        struct {
            T x, y;
        };
        T raw[2];
    };

    Vec2() : u(0), v(0) {}
    Vec2(T _u, T _v) : u(_u), v(_v) {}
    inline Vec2<T> operator+(const Vec2<T> &other) const {
        return Vec2<T>(u + other.u, v + other.v);
    }
    inline Vec2<T> operator-(const Vec2<T> &other) const {
        return Vec2<T>(u - other.u, v - other.v);
    }
    inline Vec2<T> operator*(const float f) const {
        return Vec2<T>(u * f, v * f);
    }
    template <class>
    friend std::ostream &operator<<(std::ostream &s, Vec2<T> &vector);
};

template <class T>
struct Vec3 {
    union {
        struct {
            T x, y, z;
        };
        struct {
            T ivert, iuv, inorm;
        };
        T raw[3];
    };

    Vec3() : x(0), y(0), z(0) {}
    Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
    inline Vec3<T> operator+(const Vec3<T> &other) const {
        return Vec3<T>(x + other.x, y + other.y, z + other.z);
    }
    inline Vec3<T> operator-(const Vec3<T> &other) const {
        return Vec3<T>(x - other.x, y - other.y, z - other.z);
    }
    inline Vec3<T> operator*(const float f) const {
        return Vec3<T>(x * f, y * f, z * f);
    }
    // dot product
    inline T operator*(const Vec3<T> &other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    // cross product
    inline Vec3<T> operator^(const Vec3<T> &other) const {
        return Vec3<T>(y * other.z - z * other.y,   // uyvz-uzvy
                       z * other.x - x * other.z,   // uzvx-uxvz
                       x * other.y - y * other.x);  // uxvy-uyvx
    }
    float norm() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3<T> &normalize(T l = 1) {
        *this = (*this) * (l / norm());
        return *this;
    }
    template <class>
    friend std::ostream &operator<<(std::ostream &s, Vec3<T> &vector);
};

typedef Vec2<float> Vec2f;
typedef Vec2<int> Vec2i;
typedef Vec3<float> Vec3f;
typedef Vec3<int> Vec3i;

template <class T>
std::ostream &operator<<(std::ostream &s, Vec2<T> &vector) {
    s << "(" << vector.x << ", " << vector.y << ")";
    return s;
}

template <class T>
std::ostream &operator<<(std::ostream &s, Vec3<T> &vector) {
    s << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")";
    return s;
}