#pragma once

namespace modlib {

template<typename T=int>
struct Vec2D
{
public:
    T x;
    T y;

    constexpr Vec2D (T x_=0, T y_=0)      : x (x_), y(y_) {}

    Vec2D operator+ (const Vec2D<T>& rhs) const
    {
        return Vec2D (
            x + rhs.x,
            y + rhs.y
        );
    }
    Vec2D operator- (const Vec2D<T>& rhs) const
    {
        return Vec2D (
            x - rhs.x,
            y - rhs.y
        );
    }
    Vec2D& operator+= (const Vec2D<T>& rhs) const
    {
        x += rhs.x;
        y += rhs.y;

        return *this;
    }
    Vec2D& operator-= (const Vec2D<T>& rhs) const
    {
        x -= rhs.x;
        y -= rhs.y;

        return *this;
    }

    Vec2D operator* (const T& rhs) const
    {
        return Vec2D (
            x * rhs,
            y * rhs
        );
    }
    Vec2D& operator*= (const T& rhs) const
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
