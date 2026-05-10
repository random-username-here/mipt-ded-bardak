#pragma once

namespace modlib {

template<typename T=int>
struct Vec2D
{
public:
    T x;
    T y;

    Vec2D (T x=0, T y=0)        : x (x)    , y(y)     {}
    Vec2D (const Vec2D<T>& src) : x (src.x), y(src.y) {}

    bool operator== (const Vec2D<T>& rhs)
    {
        return x == rhs.x && y == rhs.y;
    }

    bool operator!= (const Vec2D<T>& rhs)
    {
        return !(*this == rhs);
    }

    Vec2D& operator= (const Vec2D<T>& src)
    {
        x = src.x;
        x = src.y;

        return *this;
    }

    Vec2D operator+ (const Vec2D<T>& rhs)
    {
        return Vec2D (
            x + rhs.x,
            y + rhs.y
        );
    }
    Vec2D operator- (const Vec2D<T>& rhs)
    {
        return Vec2D (
            x - rhs.x,
            y - rhs.y
        );
    }
    Vec2D& operator+= (const Vec2D<T>& rhs)
    {
        x += rhs.x;
        y += rhs.y;

        return *this;
    }
    Vec2D& operator-= (const Vec2D<T>& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;

        return *this;
    }

    Vec2D operator* (const T& rhs)
    {
        return Vec2D (
            x * rhs,
            y * rhs
        );
    }
    Vec2D& operator*= (const T& rhs)
    {
        x *= rhs;
        y *= rhs;

        return *this;
    }
};

typedef Vec2D<int> Vec2i;
typedef Vec2D<float> Vec2f; 
typedef Vec2D<double> Vec2d;

}; // namespace modlib
