#ifndef VECTOR_TEMPLATE_H
#define VECTOR_TEMPLATE_H

#include "mathtools.h"

template <typename T> class Vector2d
{
public:
    Vector2d()
        : x(0), y(0)
    {
    }

    Vector2d(const Vector2d& v)
        : x(v.x), y(v.y)
    {
    }

    Vector2d(const T& xa, const T& ya)
        : x(xa), y(ya)
    {
    }

    const Vector2d& operator=(const Vector2d &v)
    {
        x = v.x;
        y = v.y;
        return *this;
    }

    const Vector2d& operator=(const T& s)
    {
        x = y = s;
    }

    const bool operator==(const Vector2d &v) const
    {
        return (x == v.x) && (y == v.y);
    }

    const bool operator!=(const Vector2d &v) const
    {
        return !(*this == v);
    }

    const Vector2d operator+(const Vector2d &v) const
    {
        return Vector2d(x + v.x, y + v.y);
    }

    const Vector2d& operator+=(const Vector2d& v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    const Vector2d operator-(const Vector2d& v) const
    {    
        return Vector2d(x - v.x, y - v.y);
    }

    const Vector2d operator-() const
    {    
        return Vector2d(-x, -y);
    }
    bool isZero()
    {
        return !x && !y;
    }

    const Vector2d &operator-=(const Vector2d& vec)
    {
        x -= vec.x;
        y -= vec.y;
        return *this;
    }

    const Vector2d &operator*=(const T &s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    const Vector2d &operator/=(const T &s)
    {
        x /= s;
        y /= s;
        return *this;
    }

    const Vector2d &operator/=(const Vector2d &v)
    {
        x /= v.x;
        y /= v.y;
        return *this;
    }

    const Vector2d &operator*=(const Vector2d &v)
    {
        x *= v.x;
        y *= v.y;
        return *this;
    }

    const Vector2d operator*(const T &s) const
    {
        return Vector2d(x*s, y*s);
    }

    const Vector2d operator*(const Vector2d &v) const
    {
        return Vector2d(x*v.x, y*v.y);
    }

    friend inline const Vector2d operator*(const T &s, const Vector2d &v)
    {
        return v * s;
    }

    friend inline const Vector2d operator*(const Vector2d &v, const T &s)
    {
        return Vector2d(v.x*s, v.y*s);
    }

    const Vector2d operator/(T s) const
    {
        return Vector2d(x/s, y/s);
    }

    const inline T lensq(void) const
    {
        return x*x + y*y;
    }

    const inline T len(void) const
    {
        return sqrt(x*x + y*y);
    }

    const inline T getUnit(void) const
    {
        const T l = len();
        return l ? *this / len() : Vector2d(0,0,0);
    }

    inline void normalize(void)
    {
        if(x || y)
            *this /= len();
        else
            x = y = 0;
    }

    void inline setLen(const T& newlen)
    {
        if(x || y)
        {
            T t = newlen / len();
            this->x *= t
            this->y *= t
        }
    }

    const T inline dot(const Vector2d &v) const
    {
        return x*v.x + y*v.y;
    }

    // return angle between two vectors -- not passed by reference intentionally
    const float inline angle(const Vector2d v) const
    {
        v = v.normalize();
        Vector2d m = *this;
        m.normalize();
        return angleNorm(v);
    }

    // return angle between two vectors -- both vectors must already be normalized!
    const float inline angleNorm(const Vector2d& normal) const
    {
        return acos(dot(normal));
    }

    const inline bool isLenIn(float radius) const
    {
        return (x*x + y*y) <= (radius*radius);
    }

    const inline bool isZero(void) const
    {
        return !(x || y);
    }

    // rotate in degrees
    void rotateDeg(float angle)
    {
        float a = degToRad(angle);
        float oldx = x;
        x = cos(a)*x - sin(a)*y;
        y = -(sin(a)*oldx + cos(a)*y);
    }

    void rotateRad(float rad)
    {
        T oldx = x;
        x = cos(rad)*x - sin(rad)*y;
        y = sin(rad)*oldx + cos(rad)*y;
    }

    const inline T rotation(void) const
    {
        Vector2d t = *this;
        t.normalize();
        return t.angleNorm(Vector2d(0, -1));
    }

    const inline T rotationDeg(void) const
    {
        return radToDeg(rotation());
    }


protected:
    T x, y;


};

typedef Vector2d<float> Vector2df;

#endif
