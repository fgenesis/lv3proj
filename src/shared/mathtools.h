#ifndef MATHTOOLS_H
#define MATHTOOLS_H

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif


const float PI = 3.14159265359f;
const float DEGTORAD = PI / 180.0f;
const float RADTODEG = 180.0f / PI;

inline float fastabs(float f)
{
    int i = ((*(int*)&f) & 0x7fffffff);
    return (*(float*)&i);
}

inline float fastneg(float f)
{
    int i = ((*(int*)&f) ^ 0x80000000);
    return (*(float*)&i);
}

inline int fastsgn(float f)
{
    return 1 + (((*(int*)&f) >> 31) << 1);
}

inline bool fastsgncheck(float f)
{
    return (*(int*)&f) & 0x80000000;
}

inline int32 int32r(float f)
{
    return int32(f + 0.5f);
}

// floor to next power of 2
inline uint32 flp2(uint32 x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x - (x >> 1);
}

// ceil to next power of 2
inline uint32 clp2(uint32 x)
{
    --x;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x + 1;
}

inline float radToDeg(float rad)
{
    return RADTODEG * rad;
}

inline float degToRad(float deg)
{
    return DEGTORAD * deg;
}

// returns linear interpolation of a and b with ratio t, with 0 <= t <= 1
template<class T> inline T lerp(const T& a, const T& b, const T& t)
{
    return (a * (1-t)) + (b*t);
}

// clamps a value between low and high
template <class T> inline const T clamp (const T& value, const T& low, const T& high)
{
    return std::min<T>(std::max<T>(value,low), high);
}


#endif
