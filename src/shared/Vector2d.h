#ifndef VECTOR_TEMPLATE_H
#define VECTOR_TEMPLATE_H

#include "mathtools.h"

#include "FalconHelpers.h"

template <typename T> class Vector2d
{
public:
    typedef T type;

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
#ifdef _DEBUG
        if(&xa == &ya)
        {
            int a = 0;
            a += a; // breakpoint here
        }
#endif
    }

    static inline Vector2d FromRadAngle(float rad)
    {
        return Vector2d(sin(rad), cos(rad));
    }

    static inline Vector2d FromDegAngle(float deg)
    {
        return FromRadAngle(degToRad(deg));
    }

    const Vector2d& operator=(const Vector2d &v)
    {
        x = v.x;
        y = v.y;
        return *this;
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

    const Vector2d operator+(const T &s) const
    {
        return Vector2d(x + s, y + s);
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

    const Vector2d operator-(const T& s) const
    {    
        return Vector2d(x - s, y - s);
    }

    const Vector2d operator-() const
    {    
        return Vector2d(-x, -y);
    }

    inline void flip()
    {    
        x = -x;
        y = -y;
    }

    const Vector2d &operator-=(const Vector2d& vec)
    {
        x -= vec.x;
        y -= vec.y;
        return *this;
    }

    const Vector2d &operator-=(const T& s)
    {
        x -= s;
        y -= s;
        return *this;
    }

    const Vector2d &operator+=(const T& s)
    {
        x += s;
        y += s;
        return *this;
    }

    const Vector2d &operator*=(const T &s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    const Vector2d &operator/(const Vector2d &v)
    {
        x /= v.x;
        y /= v.y;
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

    const Vector2d operator/(const T& s) const
    {
        return Vector2d(x/s, y/s);
    }

    inline bool operator!() const
    {
        return isZero();
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
            this->x *= t;
            this->y *= t;
        }
    }

    const T inline dot(const Vector2d &v) const
    {
        return x*v.x + y*v.y;
    }

    // return angle between two vectors -- not passed by reference intentionally
    const T inline angle(Vector2d v) const
    {
        Vector2d m = *const_cast<Vector2d*>(this);
        v.normalize();
        m.normalize();
        return m.angleNorm(v);
    }

    const T inline angleDeg(const Vector2d& v) const
    {
        return radToDeg(angle(v));
    }

    // return angle between two vectors -- both vectors must already be normalized!
    const T inline angleNorm(const Vector2d& normal) const
    {
        return acos(dot(normal));
    }

    const inline bool isLenIn(T radius) const
    {
        return (x*x + y*y) <= (radius*radius);
    }

    const inline bool isZero(void) const
    {
        return !(x || y);
    }

    // rotate in degrees
    inline void rotateDeg(float angle)
    {
        rotate(degToRad(angle));
    }

    inline void rotate(float rad)
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

    T x, y;
};

typedef Vector2d<float> Vector2df;
typedef Vector2d<int32> Vector2di;

#endif
