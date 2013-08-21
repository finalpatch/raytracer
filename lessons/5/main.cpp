#include <list>
#include <limits>
#include <array>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <numeric>
#include "SDL/SDL.h"

const static unsigned width     = 1280;
const static unsigned height    = 720;
const static float    fov       = 45;
const static float    pi        = 3.1415926536;

enum { max_depth = 6 };

template<typename T, std::size_t N>
class Vec : public std::array<T, N>
{
public:
    Vec() { std::fill_n(this->begin(), N, T()); }
    template <typename U>
    Vec(const Vec<U, N>& other) { std::copy_n(other.begin(), N, this->begin()); }
    Vec(const T& v) { std::fill_n(this->begin(), N, v); }
    Vec(std::initializer_list<T> l) { std::copy_n(l.begin(), N, this->begin()); }

    template <typename OP>
    Vec& transform(OP op)
    {
        std::transform(this->begin(), this->end(), this->begin(), op);
        return *this;
    }
    template <typename OP, typename U>
    Vec& transform(const Vec<U, N>& u, OP op)
    {
        std::transform(this->begin(), this->end(), u.begin(), this->begin(), op);
        return *this;
    }
    
    // *** multiply scalar
    Vec& operator *= (const T& x) {return transform([&] (T v) { return v * x; } );}
    Vec operator * (const T& x) {Vec<T, N> t(*this); t *= x; return t;}
    // *** multiply vector
    Vec& operator *= (const Vec& v) {return transform(v, std::multiplies<T>());}
    Vec operator * (const Vec& v) const {Vec<T, N> t(*this); t *= v; return t;}
    // *** add vector
    Vec& operator += (const Vec& v) {return transform(v, std::plus<T>());}
    Vec operator + (const Vec& v) const {Vec<T, N> t(*this); t += v; return t;}
    // *** subtract vectot
    Vec& operator -= (const Vec& v) {return transform(v, std::minus<T>());}
    Vec operator - (const Vec& v) const {Vec<T, N> t(*this); t -= v; return t;}
    // *** reverse
    Vec operator - () const {return (*this) * T(-1);}
    // *** dot product
    T dot(const Vec& v) const
    {
        return std::inner_product(this->begin(), this->end(), v.begin(), T(0));
    }
    // ***
    T magnitude() const
    {
        return sqrt(dot(*this));
    }
    // ***
    void normalize()
    {
        T mag = magnitude();
        if (mag)
            *this *= 1 / mag;
    }
    Vec normalized()
    {
        Vec t(*this);
        t.normalize();
        return t;
    }
};

template <typename T>
using Vec3 = Vec<T, 3>;

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
	Sphere(const Vec3<T> &c, const T &r, const Vec3<T> &clr, const T& refl = 0.0, const T& trans = 0.0, bool chkbd = false)
		: m_center(c), m_radius(r), m_color(clr), m_reflection(refl), m_transparency(trans), m_checkerBoard(chkbd)
	{}
	Vec3<T> normal(const Vec3<T>& pos) const
	{
		return (pos - m_center).normalized();
	}    
	bool intersect(const Ray<T>& ray, T* distance = nullptr) const
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
			auto near = a - c;
			auto far  = a + c;
			// near < 0 means ray starts inside
			*distance = (near < 0) ? far : near;
		}
		return true;
	}
	Vec3<T> color(const Vec3<T>& pos) const
	{
        if (m_checkerBoard)
            return ((int(pos[2]) + int(pos[0])) % 2) ? Vec3<T>{0,0,0} : Vec3<T>{1,1,1};
        else
            return m_color;
	}
	T reflection_ratio() const
	{
		return m_reflection;
	}
	T transparency() const
	{
		return m_transparency;
	}
protected:
	Vec3<T>            m_center;
	T                  m_radius;
	Vec3<T>            m_color;
	T                  m_reflection;
	T                  m_transparency;
    bool               m_checkerBoard;
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

template <typename T>
struct Scene
{
    std::list<Sphere<T>*> objects;
    std::list<Light<T>*>  lights;
};

template<typename T>
Vec3<T> trace(const Ray<T>& ray, const Scene<T>& scene, int depth)
{
	T nearest = std::numeric_limits<T>::max();
	const Sphere<T>* obj = nullptr;

	// search the scene for nearest intersection
	for(auto& o: scene.objects)
	{
		T distance = std::numeric_limits<T>::max();
		if (o->intersect(ray, &distance))
		{
			if (distance < nearest)
			{
				nearest = distance;
				obj = o;
			}
		}
	}

	if (!obj)                   // no hit
        return Vec3<T>(0);      // return black
	
	auto point_of_hit = ray.start + ray.dir * nearest;
	auto normal = obj->normal(point_of_hit);
	bool inside = false;

    // normal should always face the origin
	if (normal.dot(ray.dir) > 0)
    {
        inside = true;
        normal = -normal;
    }
	
	Vec3<T> color(0);
    T reflection_ratio = obj->reflection_ratio();

    // compute diffuse light
    // add up incoming light from all light sources
    for(auto& l: scene.lights)
    {
        auto light_direction = (l->position() - point_of_hit).normalized();
        Ray<T> r (point_of_hit + normal * 1e-5, light_direction);

        // go through the scene check whether we're blocked from the lights
        bool blocked = std::any_of(scene.objects.begin(), scene.objects.end(), [=] (const Sphere<T>* o) {
                return o->intersect(r); });
        if (!blocked)
        {
            color += l->color()
                * std::max(T(0), normal.dot(light_direction))
                * obj->color(point_of_hit)
                * (T(1) - reflection_ratio);
            auto half_angle = (-ray.dir + light_direction).normalized();
            auto specular = pow(std::max(T(0), normal.dot(half_angle)), T(30));
            color += l->color() * obj->color(point_of_hit) * specular * .5;
        }
    }

    T facing = std::max(T(0), -ray.dir.dot(normal));
    T fresneleffect = reflection_ratio + (1 - reflection_ratio) * pow((1 - facing), 5);

    // compute reflection
    if (depth < max_depth && obj->reflection_ratio() > 0)
    {
        auto reflection_direction = ray.dir + normal * 2 * ray.dir.dot(normal) * T(-1);
        auto reflection = trace(Ray<T>(point_of_hit + normal * 1e-5, reflection_direction), scene, depth + 1);
        color += reflection * fresneleffect;
    }
	
    // compute refraction
    if (depth < max_depth && obj->transparency() > 0)
    {
		T ior = 1.5; // index of refraction
		auto CE = ray.dir.dot(normal) * T(-1);
        ior = inside ? T(1) / ior : ior;
        auto eta = T(1) / ior;
        auto GF = (ray.dir + normal * CE) * eta;
        auto sin_t1_2 = 1 - CE * CE;
        auto sin_t2_2 = sin_t1_2 * (eta * eta);
        if (sin_t2_2 < T(1))
        {
            auto GC = normal * sqrt(1 - sin_t2_2);
            auto refraction_direction = GF - GC;
            auto refraction = trace(Ray<T>(point_of_hit - normal * 1e-4, refraction_direction),
                                    scene, depth + 1);
            color += refraction * (1 - fresneleffect) * obj->transparency();
        }
    }
	return color;
}

template <typename T>
void render(const Scene<T>& scene, SDL_Surface* surface)
{
	SDL_LockSurface(surface);

	// eye at [0, 0, 0]
	// screen plane at [x, y, -1]
	Vec3<T> eye(0);
	T h = tan(fov / 360 * 2 * pi / 2) * 2;
	T w = h * width / height;

	for (unsigned y = 0; y < height; ++y)
	{
		auto row = reinterpret_cast<uint32_t*>(surface->pixels) + surface->pitch * y / sizeof(uint32_t);
		for (unsigned x = 0; x < width; ++x)
		{
            Vec3<T> pixel(0.0);
            for(unsigned suby = 0; suby < 2; ++suby)
            {
                for(unsigned subx = 0; subx < 2; ++subx)
                {
                    Vec3<T> direction = {(T(x+0.5f*subx) - width / 2) / width  * w,
                                         (T(height)/2 - (y+0.5f*suby)) / height * h,
                                         -1.0f };
                    direction.normalize();
                    pixel += trace(Ray<T>(eye, direction), scene, 0);
                }
            }
            for(int i = 0; i < 3; ++i)
                pixel[i] = pow(pixel[i] * 0.25, 0.454545);
			// convert from [0, 1] to [0, 255] and write to frame buffer
			Vec3<int> rgb = pixel * 255 + 0.5;
			rgb.transform([] (int x) { return std::min(x, 255); });
            row[x] = rgb[2] | (rgb[1] << 8) | (rgb[0] << 16);
			// row[x] = SDL_MapRGB(surface->format, rgb[0], rgb[1], rgb[2]);
		}
		row += surface->pitch;
	}
	SDL_UnlockSurface(surface);
	SDL_UpdateRect(surface, 0, 0, 0, 0);
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	atexit(SDL_Quit);
	SDL_Surface* screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);

	if (!screen)
		return 1;

	// add objects
	Scene<float> scene;
	scene.objects = { new Sphere<float>({0, -10002, -20}, 10000, {.8, .8, .8}, 0.0, 0.0, true),
					  new Sphere<float>({0, 2, -20},      4    , {.8, .5, .5}, 0.5),
					  new Sphere<float>({5, 0, -15},      2    , {.3, .8, .8}, 0.2),
					  new Sphere<float>({-5, 0, -15},     2    , {.3, .5, .8}, 0.2),
					  new Sphere<float>({-2, -1, -10},    1    , {.1, .1, .1}, 0.1, 0.8)};
	scene.lights = { new Light<float>({-10, 20, 30},  {2, 2, 2}) };

	render(scene, screen);

	SDL_Event event;
	while (SDL_WaitEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				return 0;
			break;
		case SDL_QUIT:
			return 0;
		}
	}
	return 0;
}
