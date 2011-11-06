#ifndef SHAREDSTRUCTS_H
#define SHAREDSTRUCTS_H
#include <limits.h>
#include "SharedDefines.h"
#include "Vector2d.h"

struct Camera : public Vector2df
{
    Camera() : Vector2df() {}
    Camera(int32 x_, int32 y_) : Vector2df((float)x_, (float)y_) {}
    Camera(float x_, float y_) : Vector2df(x_, y_) {}
    Camera(const Vector2df& p): Vector2df(p) {}
    inline void TranslatePoints(int32& ax, int32& ay) const
    {
        ax -= x;
        ay -= y;
    }
    inline void TranslatePoints(int16& ax, int16& ay) const
    {
        ax -= (int16)x;
        ay -= (int16)y;
    }
    inline void TranslatePoints(float& ax, float& ay) const
    {
        ax -= x;
        ay -= y;
    }
    inline void TranslateVector(Vector2df& v) const
    {
        v.x -= x;
        v.y -= y;
    }
};


// basic rectangle class, provides interfaces, but does not have any object properties
// methods are inline and non-virtual intentionally.
class BaseRect
{
public:

    Vector2df pos;
    Vector2df size;

    float &x, &y, &h, &w; // convenience accessors

    BaseRect() : x(pos.x), y(pos.y), w(size.x), h(size.y) {}
    BaseRect(float ax, float ay, float aw, float ah) : pos(ax, ay), size(aw, ah), x(pos.x), y(pos.y), w(size.x), h(size.y) {}
    BaseRect(const BaseRect& r) : pos(r.pos), size(r.size), x(pos.x), y(pos.y), w(size.x), h(size.y) {}
    BaseRect(const Vector2df& p, const Vector2df& sz) : pos(p), size(sz), x(pos.x), y(pos.y), w(size.x), h(size.y) {}

    const BaseRect& operator= (const BaseRect& r)
    {
        pos = r.pos;
        size = r.size;
        return *this;
    }

    // Method to calculate the second X corner
    inline float x2(void) const { return x + w; }
    // Method to calculate the second Y corner
    inline float y2(void) const { return y + h; }

    inline BaseRect cloneRect(void) const
    {
        BaseRect r(*this);
        return r;
    }

    inline void SetBBox(float x_, float y_, float w_, float h_)
    {
        this->x = x_;
        this->y = y_;
        this->w = w_;
        this->h = h_;
    }

    inline void SetPos(float x_, float y_)
    {
        this->x = x_;
        this->y = y_;
    }

    inline void SetPos(const Vector2df& v)
    {
        pos = v;
    }

    inline void Move(float xr, float yr)
    {
        this->x += xr;
        this->y += yr;
    }

    inline void Move(const Vector2df& v)
    {
        pos += v;
    }

    inline bool operator==(BaseRect& other)
    {
        return other.pos == pos && other.size == size;
    }

    inline bool operator!=(BaseRect& other)
    {
        return !(*this == other);
    }

    inline BaseRect overlapRect(const BaseRect& r)
    {
        float nx = x;
        float ny = y;
        float tx2 = x2();
        float ty2 = y2();
        float rx2 = r.x2();
        float ry2 = r.y2();
        if (x < r.x) nx = r.x;
        if (x < r.y) ny = r.y;
        if (tx2 > rx2) tx2 = rx2;
        if (ty2 > ry2) ty2 = ry2;
        tx2 -= x;
        ty2 -= y;
        return BaseRect(nx, ny, tx2, ty2);
    }

    // original code from SDL
    inline BaseRect unionRect(const BaseRect& r)
    {
        float Amin, Amax, Bmin, Bmax;
        BaseRect uni;

        Amin = x;
        Amax = Amin + w;
        Bmin = r.x;
        Bmax = Bmin + r.w;
        if (Bmin < Amin)
            Amin = Bmin;
        uni.x = Amin;
        if (Bmax > Amax)
            Amax = Bmax;
        uni.w = Amax - Amin;

        Amin = y;
        Amax = Amin + h;
        Bmin = r.y;
        Bmax = Bmin + r.h;
        if (Bmin < Amin)
            Amin = Bmin;
        uni.y = Amin;
        if (Bmax > Amax)
            Amax = Bmax;
        uni.h = Amax - Amin;

        return uni;
    }

    inline bool CollisionWith(BaseRect *other)
    {
        return !(y2() < other->y || y > other->y2() || x2() < other->x || x > other->x2());
    }

    inline float area(void)
    {
        return w * h;
    }
};

// from MaNGOS
class IntervalTimer
{
public:
    IntervalTimer() : _interval(0), _current(0) {}

    void Update(time_t diff)
    {
        _current += diff;
        if (_current < 0)
            _current = 0;
    }
    bool Passed() const { return _current >= _interval; }
    void Reset()
    {
        if (_current >= _interval)
            _current -= _interval;
    }

    void SetCurrent(time_t current) { _current = current; }
    void SetInterval(time_t interval) { _interval = interval; }
    time_t GetInterval() const { return _interval; }
    time_t GetCurrent() const { return _current; }

private:
    time_t _interval;
    time_t _current;
};




#endif
