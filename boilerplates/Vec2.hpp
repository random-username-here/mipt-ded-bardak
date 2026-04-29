#pragma once


template<typename T=int>
struct Vec2D
{
public:
    T m_x;
    T m_y;

    Vec2D (T x=0, T y=0)        : m_x (x),       m_y (y)       {}
    Vec2D (const Vec2D<T>& src) : m_x (src.m_x), m_y (src.m_y) {}

    Vec2D& operator= (const Vec2D<T>& src)
    {
        m_x = src.m_x;
        m_x = src.m_y;

        return *this;
    }

    Vec2D operator+ (const Vec2D<T>& rhs)
    {
        return Vec2D (
            m_x + rhs.m_x,
            m_y + rhs.m_y
        );
    }
    Vec2D operator- (const Vec2D<T>& rhs)
    {
        return Vec2D (
            m_x - rhs.m_x,
            m_y - rhs.m_y
        );
    }
    Vec2D& operator+= (const Vec2D<T>& rhs)
    {
        m_x += rhs.m_x;
        m_y += rhs.m_y;

        return *this;
    }
    Vec2D& operator-= (const Vec2D<T>& rhs)
    {
        m_x -= rhs.m_x;
        m_y -= rhs.m_y;

        return *this;
    }

    Vec2D operator* (const T& rhs)
    {
        return Vec2D (
            m_x * rhs,
            m_y * rhs
        );
    }
    Vec2D& operator*= (const T& rhs)
    {
        m_x *= rhs;
        m_y *= rhs;

        return *this;
    }
};

typedef Vec2D<int> Vec2i;
typedef Vec2D<float> Vec2f;
typedef Vec2D<double> Vec2d;