#include "vecmat.h"
#include "objects.h"
#include "SDL/SDL.h"
#include <stdint.h>
#include <list>
#include <limits>

const static unsigned width = 1280;
const static unsigned height = 720;
const static float fov = 45;
const static float pi = 3.1415926536;

SDL_Surface *screen;

template <typename T>
struct Ray
{
    Vec<T, 3> origin;
    Vec<T, 3> direction;

    Ray(const Vec<T, 3>& orig, const Vec<T, 3>& dir) :
        origin(orig), direction(dir)
    {}
};

template<typename T, typename C, typename L>
Vec3<T> trace(const Ray<T>& ray, const C& scene, const L& lights, const int &depth)
{
	T distance = std::numeric_limits<T>::max();
	const Sphere<T>* sphere = NULL;

    // search the scene for nearest intersection
    for(const Sphere<T>& s: scene)
    {
		T near = std::numeric_limits<T>::max();
        T far = std::numeric_limits<T>::max();
		if (s.intersect(ray.origin, ray.direction, &near, &far))
        {
			if (near < 0)       // origin is inside the object
                near = far;
			if (near < distance)
            {
				distance = near;
				sphere = &s;
			}
		}
	}

	if (!sphere)                // no hit
        return Vec3<T>(0);      // return black

	Vec3<T> point_of_hit = ray.origin + ray.direction * distance;
	Vec3<T> normal = sphere->normal(point_of_hit);
    
    // normal should always face the origin
	if (normal.dot(ray.direction) > 0)
        normal = -normal;
    
    // a tiny offset to make sure the point we start tracing is from outside of the object
	const T offset = std::numeric_limits<T>::epsilon();

	Vec3<T> color(0);           // the color of the pixel, initialise to 0,0,0

    // compute diffuse light
    // add up incoming light from all light sources
    for(const Light<T>& l: lights)
    {
        Vec3<T> light_direction = (l.position() - point_of_hit).normalized();

        // go through the scene check whether we're blocked from the lights
        bool blocked = false;
        for(const Sphere<T>& i: scene)
        {
            if (i.intersect(point_of_hit + normal * offset, light_direction))
            {
                blocked = true;
                break;
            }
        }
        if (!blocked)
            color += l.color() * std::max(normal.dot(light_direction), T(0)) * sphere->color();
    }

	return color;
}

template <typename C, typename L>
void render(const C& scene, const L& lights)
{
    SDL_LockSurface(screen);
    
    // eye at [0, 0, 0]
    // screen plane at [x, y, -1]
    Vec3<float> eye(0);
    float h = tan(fov / 360 * 2 * pi / 2) * 2;
    float w = h * width / height;

    unsigned char* row = reinterpret_cast<unsigned char*>(screen->pixels);
    for (unsigned y = 0; y < height; ++y)
    {
        unsigned char* p = row;
        for (unsigned x = 0; x < width; ++x)
        {
            Vec3<float> direction = { ((float)x - width/2)  / width  * w,
                                      ((float)height/2 - y) / height * h,
                                      -1.0f };
            direction.normalize();
            
            Vec3<float> pixel = trace(Ray<float>(eye, direction), scene, lights, 0);
            
            *p++ = std::min(255, int(pixel[2]*255+0.5)); // b
            *p++ = std::min(255, int(pixel[1]*255+0.5)); // g
            *p++ = std::min(255, int(pixel[0]*255+0.5)); // r
            *p++ = 255;                                  // a
        }
        row += screen->pitch;
    }
    SDL_UnlockSurface(screen);
    SDL_UpdateRect(screen, 0, 0, 0, 0);
}

bool event_loop()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
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

    std::list<Sphere<float> > scene;
    std::list<Light<float> > lights;
    
    // add objects
	scene.emplace_back(Sphere<float>({0, -10004, -20}, 10000, {0.2, 0.2, 0.2}));
	scene.emplace_back(Sphere<float>({0, 0, -20},      4,     {1.0, 0.3, 0.3}));
	scene.emplace_back(Sphere<float>({5, -1, -15},     2,     {0.9, 0.7, 0.5}));
	scene.emplace_back(Sphere<float>({-5, -1, -15},    2,     {0.8, 0.7, 0.9}));
    // add lights
    lights.emplace_back(Light<float>({-10, 20, 30},  {1, 1, 1}));
    
	render(scene, lights);
    
    while (event_loop())
        ;

    SDL_Quit();
    return 0;
}
