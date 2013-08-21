#include "../vecmat.h"
#include "SDL/SDL.h"
#include <list>
#include <limits>

const static unsigned width     = 1280;
const static unsigned height    = 720;
const static float    fov       = 45;
const static float    pi        = 3.1415926536;

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
    Sphere(const Vec3<T> &c, const T &r) : m_center(c), m_radius(r)
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
protected:
    Vec3<T>            m_center;
    T                  m_radius;
};

template <typename T>
using Scene = std::list<Sphere<T>*> ;

template<typename T>
Vec3<T> trace(const Ray<T>& ray, const Scene<T>& scene, int depth)
{
	T nearest = std::numeric_limits<T>::max();
	const Sphere<T>* obj = NULL;

    // search the scene for nearest intersection
    for(auto& o: scene)
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
    return Vec3<T>(obj ? 1 : 0);
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

    auto row = reinterpret_cast<unsigned char*>(surface->pixels);
    for (unsigned y = 0; y < height; ++y)
    {
        auto p = reinterpret_cast<Uint32*>(row);
        for (unsigned x = 0; x < width; ++x)
        {
            Vec3<T> direction = {(T(x) - width / 2) / width  * w,
                                 (T(height)/2 - y) / height * h,
                                 -1.0f };
            direction.normalize();
            auto pixel = trace(Ray<T>(eye, direction), scene, 0);
            // convert from [0, 1] to [0, 255] and write to frame buffer
            Vec3<int> rgb = pixel * 255 + 0.5;
            rgb.transform([] (int x) { return std::min(x, 255); });
            *p++ = SDL_MapRGB(surface->format, rgb[0], rgb[1], rgb[2]);
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
    Scene<float> scene = { new Sphere<float>({0, -10002, -20}, 10000),
                           new Sphere<float>({0, 2, -20},      4    ),
                           new Sphere<float>({5, 0, -15},      2    ),
                           new Sphere<float>({-5, 0, -15},     2    ),
                           new Sphere<float>({-2, -1, -10},    1    ) };

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
