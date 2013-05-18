#include "vecmat.h"
#include "objects.h"
#include "SDL/SDL.h"
#include <stdint.h>
#include <list>
#include <limits>

const static unsigned width     = 1280;
const static unsigned height    = 720;
const static unsigned max_depth = 6;
const static float    fov       = 45;
const static float    pi        = 3.1415926536;

template <typename T>
struct Scene
{
    std::list<Sphere<T>*> objects;
    std::list<Light<T>*>  lights;

    ~Scene()
    {
        for (auto& o: objects)
            delete o;
        for (auto& l: lights)
            delete l;
    }
};

template<typename T>
Vec3<T> trace(const Ray<T>& ray, const Scene<T>& scene, int depth)
{
	T nearest = std::numeric_limits<T>::max();
	const Sphere<T>* obj = NULL;

    // search the scene for nearest intersection
    for(auto& s: scene.objects)
    {
		T distance = std::numeric_limits<T>::max();
		if (s->intersect(ray, &distance))
        {
			if (distance < nearest)
            {
				nearest = distance;
				obj = s;
			}
		}
	}

	if (!obj)                   // no hit
        return Vec3<T>(0);      // return black

	auto point_of_hit = ray.start + ray.dir * nearest;
	auto normal = obj->normal(point_of_hit);
    
    // normal should always face the origin
	if (normal.dot(ray.dir) > 0)
        normal = -normal;
    
    // a tiny offset to make sure the point we start tracing is from outside of the object
	const T offset = std::numeric_limits<T>::epsilon();

    Vec3<T> color(0);
    const Material<T>& material = obj->material();
	Vec3<T> diffuse_color = material.diffuse(point_of_hit);
    T       reflection_ratio = material.reflection(point_of_hit);

    // compute diffuse light
    // add up incoming light from all light sources
    for(auto& l: scene.lights)
    {
        auto light_direction = (l->position() - point_of_hit).normalized();
            
        // go through the scene check whether we're blocked from the lights
        bool blocked = std::any_of(scene.objects.begin(), scene.objects.end(), [=] (const Sphere<T>* o) {
                return o->intersect({point_of_hit + normal * offset, light_direction}); });
        if (!blocked)
            color += l->color()
                * std::max(T(0), normal.dot(light_direction))
                * diffuse_color
                * (T(1) - reflection_ratio);
    }

    // compute reflection
    if (depth < max_depth && (obj->material().reflection(point_of_hit) > 0))
    {
        auto reflection_direction = ray.dir - normal * 2 * ray.dir.dot(normal);
        auto reflection = trace(Ray<T>(point_of_hit + normal * offset, reflection_direction),
                                scene, depth + 1);
        color += reflection * reflection_ratio;
    }
	return color;
}

static SDL_Surface* screen;

template <typename T>
void render(const Scene<T>& scene)
{
    SDL_LockSurface(screen);
    
    // eye at [0, 0, 0]
    // screen plane at [x, y, -1]
    Vec3<T> eye(0);
    T h = tan(fov / 360 * 2 * pi / 2) * 2;
    T w = h * width / height;

    auto row = reinterpret_cast<unsigned char*>(screen->pixels);
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
            Vec3<int> rgb = pixel * 255 + 0.5;
            rgb.transform([] (int x) { return std::min(x, 255); });
            *p++ = SDL_MapRGB(screen->format, rgb[0], rgb[1], rgb[2]);
        }
        row += screen->pitch;
    }
    SDL_UnlockSurface(screen);
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

bool event_loop()
{
	SDL_Event event;
	while (SDL_WaitEvent(&event))
	{
		switch (event.type) 
		{
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				return false;
			break;
		case SDL_QUIT:
			return false;
		}
	}
    return true;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);

	if (!screen)
		return 1;

    Scene<float> scene;

    CheckerBoard<float> checker_board;
    Shiny<float> shiny;
    
    // add objects
    scene.objects = { new Sphere<float>({0, -10004, -20}, 10000, checker_board),
                      new Sphere<float>({0, 0, -20},      4,     shiny),
                      new Sphere<float>({5, -1, -15},     2,     shiny),
                      new Sphere<float>({-5, -1, -15},    2,     shiny) };
    // add lights
    scene.lights = { new Light<float>({-10, 20, 30},  {2, 2, 2}) };
    
	render(scene);
    
    while (event_loop());

    SDL_Quit();
    return 0;
}
