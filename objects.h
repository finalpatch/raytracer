#pragma once

#include "vecmat.h"
#include "material.h"

template <typename T>
struct Ray
{
    Vec3<T> start;
    Vec3<T> dir;

    Ray(const Vec3<T>& _start, const Vec3<T>& _dir) :
        start(_start), dir(_dir)
    {}
};

template <typename T>
class Sphere
{
public:
    Sphere(const Vec3<T> &c, const T &r, const Material<T>& m) :
		m_center(c), m_radius(r), m_material(m)
	{}

    Vec3<T> normal(const Vec3<T>& pos) const
    {
        return (pos - m_center).normalized();
    }
    
	bool intersect(const Ray<T>& ray, T* distance = NULL) const
	{
		auto l = m_center - ray.start;
		auto a = l.dot(ray.dir);
		if (a < 0)              // opposite direction
            return false;
        auto b2 = l.dot(l) - a * a;
        auto r2 = m_radius * m_radius;
		if (b2 > r2)            // perpendicular > r
            return false;
        auto c = sqrt(r2 - b2);
        if (distance)
        {
            T near = a - c;
            T far  = a + c;
            // near < 0 means ray starts inside
            *distance = (near < 0) ? far : near;
        }
		return true;
	}

    const Material<T>& material() const
    {
        return m_material;
    }

protected:
    Vec3<T>            m_center;
    T                  m_radius;
    const Material<T>& m_material;
};

template <typename T>
class Light
{
public:
    Light(const Vec3<T> &p, const Vec3<T> &clr) : 
		m_position(p), m_color(clr)
	{}

    Vec3<T> position() const { return m_position; }
    Vec3<T> color()    const { return m_color;  }
protected:
    Vec3<T> m_position;
    Vec3<T> m_color;
};
