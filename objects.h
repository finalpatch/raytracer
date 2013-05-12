#pragma once

#include "vecmat.h"

template <typename T>
class Sphere
{
public:
    Sphere(const Vec3<T> &c, const T &r, const Vec3<T> &clr) : 
		m_center(c), m_radius(r), m_color(clr)
	{}

    Vec3<T> center() const { return m_center; }
    T       radius() const { return m_radius; }
    Vec3<T> color()  const { return m_color;  }

    Vec3<T> normal(const Vec3<T>& point) const
    {
        return (point - center()).normalized();
    }
    
	bool intersect(const Vec3<T>& origin, const Vec3<T>& direction, T* near = NULL, T* far = NULL) const
	{
		Vec3<T> l = center() - origin;
		T a = l.dot(direction);
		if (a < 0)              // opposite direction
            return false;
		T b2 = l.dot(l) - a * a;
        T r2 = radius() * radius();
		if (b2 > r2)            // perpendicular > r
            return false;
		T c = sqrt(r2 - b2);
		if (near)
            *near = a - c;
        if (far)
			*far = a + c;
		return true;
	}
protected:
    Vec3<T> m_center;
    T       m_radius;
    Vec3<T> m_color;
};

template <typename T>
class Light
{
public:
    Light(const Vec3<T> &p, const Vec3<T> &clr) : 
		m_position(p), m_color(clr)
	{}

    Vec3<T> position() const { return m_position; }
    Vec3<T> color()  const { return m_color;  }
protected:
    Vec3<T> m_position;
    Vec3<T> m_color;
};
