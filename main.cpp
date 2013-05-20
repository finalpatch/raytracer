#include "vecmat.h"
#include "objects.h"
#include "SDL/SDL.h"
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
    std::list<Object<T>*> objects;
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
	const Object<T>* obj = NULL;

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
    const Material<T>& material = obj->material();
	Vec3<T> diffuse_color = material.diffuse(point_of_hit);
    T       reflection_ratio = material.reflection();

    // compute diffuse light
    // add up incoming light from all light sources
    for(auto& l: scene.lights)
    {
        auto light_direction = (l->position() - point_of_hit).normalized();

        // go through the scene check whether we're blocked from the lights
        bool blocked = std::any_of(scene.objects.begin(), scene.objects.end(), [=] (const Object<T>* o) {
                return o->intersect({point_of_hit + normal * 1e-5, light_direction}); });
        if (!blocked)
            color += l->color()
                * std::max(T(0), normal.dot(light_direction))
                * diffuse_color
                * (T(1) - reflection_ratio);
    }

    T facing = std::max(T(0), -ray.dir.dot(normal));
    T fresneleffect = reflection_ratio + (1 - reflection_ratio) * pow((1 - facing), 5);

    // compute reflection
    if (depth < max_depth && reflection_ratio > 0)
    {
        auto reflection_direction = ray.dir + normal * 2 * ray.dir.dot(normal) * T(-1);
        auto reflection = trace(Ray<T>(point_of_hit + normal * 1e-5, reflection_direction),
                                scene, depth + 1);
        color += reflection * fresneleffect;
    }

    // compute refraction
    if (depth < max_depth && (material.transparency() > 0))
    {
		auto CE = ray.dir.dot(normal) * T(-1);
        auto ior = inside ? T(1) / material.ior() : material.ior();
        auto eta = T(1) / ior;
        auto GF = (ray.dir + normal * CE) * eta;
        auto sin_t1_2 = 1 - CE * CE;
        auto sin_t2_2 = sin_t1_2 * (eta * eta);
        if (sin_t2_2 < T(1))
        {
            auto GC = normal * sqrt(1 - sin_t2_2);
            auto refraction_direction = GF - GC;
            auto refraction = trace(Ray<T>(point_of_hit - normal * 1e-5, refraction_direction),
                                    scene, depth + 1);
            color += refraction * (1 - fresneleffect) * material.transparency();
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
            Vec3<int> rgb;
            std::transform(pixel.begin(), pixel.end(), rgb.begin(), [] (T x) {
                    return std::min(255, int(pow(x, 1/2.2) * 255 + 0.5)); });
            // *p++ = SDL_MapRGB(surface->format, rgb[0], rgb[1], rgb[2]);
            *p++ = rgb[2] | (rgb[1] << 8) | (rgb[0] << 16);
            
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

    CheckerBoard<float> checker_board;
    Shiny<float> shiny;
    Glass<float> glass;

    Scene<float> scene;

    // add objects
    scene.objects = { new Sphere<float>({0, -10002, -20}, 10000, checker_board),
                      new Sphere<float>({0, 2, -20},      4,     shiny),
                      new Sphere<float>({5, 0, -15},      2,     shiny),
                      new Sphere<float>({-5, 0, -15},     2,     shiny),
                      new Sphere<float>({-2, -1, -10},    1,     glass) };
    // add lights
    scene.lights = { new Light<float>({-10, 20, 30},  {2, 2, 2}) };

	render(scene, screen);

#ifndef EMSCRIPTEN
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
#endif
    return 0;
}
