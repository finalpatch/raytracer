import std.math;
import std.numeric;
import std.algorithm;
import std.parallelism;
import std.range;
import std.datetime;
import std.stdio;
import derelict.sdl.sdl;
import core.simd;

immutable width     = 1280;
immutable height    = 720;
immutable fov       = 45;
immutable max_depth = 6;

version(D_SIMD)
{
    float4 loadps(in float x)
    {
        float4 f4 = __simd(XMM.LODSS, x);
        return __simd(XMM.SHUFPS, f4, f4, 0);
    }
    struct Vec3
    {
        this(in float x)
        {
            v = loadps(x);
        }
        this(in float[] _v)
        {
            v.array[] = _v[];
        }
        this(in float4 _v)
        {
            v = _v;
        }
        
        float opIndex(size_t i) const
        {
            return v.ptr[i];
        }
        void opIndexAssign(float x, size_t i )
        {
            v.ptr[i] = x;
        }
        Vec3 opUnary(string op)() const
        {
            static if (op == "-")
            {
                return Vec3(v * (-1));
            }
            else
                assert(0, "Operator "~op~" not implemented");
        }
        Vec3 opBinary(string op)(in float rhs)
        {
            static if (op == "+")
                return Vec3(v + loadps(rhs));
            else if (op == "-")
                return Vec3(v - loadps(rhs));
            else if (op == "*")
                return Vec3(v * loadps(rhs));
            else
                assert(0, "Operator "~op~" not implemented");
        }
        Vec3 opBinary(string op)(Vec3 rhs)
        {
            static if (op == "+")
                return Vec3(v + rhs.v);
            else if (op == "-")
                return Vec3(v - rhs.v);
            else if (op == "*")
                return Vec3(v * rhs.v);
            else
                assert(0, "Operator "~op~" not implemented");
        }
        float[] opSlice()
        {
            return v.array;
        }
        float[] opSlice(size_t x, size_t y)
        {
            return v.ptr[x..y];
        }

        float4 v;
    }

    float dot(in Vec3 v1, in Vec3 v2)
    {
        const float4 t = v1.v * v2.v;
        const float* p = t.ptr;
        return p[0] + p[1] + p[2];
    }
    float magnitude(in Vec3 v)
    {
        return sqrt(dot(v, v));
    }
    Vec3 normalize(Vec3 v)
    {
        return Vec3(v.v / loadps(magnitude(v)));
    }
}
else
{
    struct Vec3
    {
        this(in float x)
        {
            v = [x,x,x];
        }
        this(in float[] _v)
        {
            v[] = _v[];
        }
        float opIndex(size_t i) const
        {
            return v[i];
        }
        void opIndexAssign(float x, size_t i )
        {
            v[i] = x;
        }
        Vec3 opUnary(string op)() const
        {
            static if (op == "-")
            {
                const float[3] t = v[] * (-1);
                return Vec3(t);
            }
            else static
                     assert(0, "Operator "~op~" not implemented");		
        }
        Vec3 opBinary(string op)(float rhs)
        {
            static if (op == "+")
            {
                const float[3] t = (v[] + rhs);
                return Vec3(t);
            }
            else if (op == "-")
            {
                const float[3] t = (v[] - rhs);
                return Vec3(t);
            }
            else if (op == "*")
            {
                const float[3] t = v[] * rhs;
                return Vec3(t);
            }
            else
                assert(0, "Operator "~op~" not implemented");
        }
        Vec3 opBinary(string op)(Vec3 rhs)
        {
            static if (op == "+")
            {
                const float[3] t = v[] + rhs.v[];
                return Vec3(t);
            }
            else if (op == "-")
            {
                const float[3] t = v[] - rhs.v[];
                return Vec3(t);
            }
            else if (op == "*")
            {
                const float[3] t = v[] * rhs.v[];
                return Vec3(t);
            }
            else
                assert(0, "Operator "~op~" not implemented");
        }
        float[] opSlice()
        {
            return v;
        }
        float[] opSlice(size_t x, size_t y)
        {
            return v[x..y];
        }

        float[3] v;
    }

    float dot(in Vec3 v1, in Vec3 v2)
    {
        const float[3] t = v1.v[] * v2.v[];
        return t[0] + t[1] + t[2];
    }
    float magnitude(in Vec3 v)
    {
        return sqrt(dot(v, v));
    }
    Vec3 normalize(in Vec3 v)
    {
        const float[3] t = v.v[] / magnitude(v);
        return Vec3(t);
    }
}

struct Ray
{
	Vec3 start;
	Vec3 dir;

	this(in Vec3 _start, in Vec3 _dir)
	{
		start = _start;
		dir = _dir;
	}
}

class Sphere
{
public:
    this(Vec3 c, float r, Vec3 clr, float refl = 0, float trans = 0)
	{
		m_center = c;
		m_radius = r;
		m_color = clr;
		m_reflection = refl;
		m_transparency = trans;
	}
    Vec3 normal(Vec3 pos)
    {
        return normalize(pos - m_center);
    }
	bool intersect(Ray ray, float* distance = null)
	{
		auto l = m_center - ray.start;
		auto a = dot(l, ray.dir);
		if (a < 0)              // opposite direction
		{
            return false;
		}
        auto b2 = dot(l, l) - a * a;
        auto r2 = m_radius * m_radius;
		if (b2 > r2)            // perpendicular > r
		{
            return false;
		}
        auto c = sqrt(r2 - b2);

		if (distance != null)
		{
			auto near = a - c;
			auto far  = a + c;
			*distance = (near < 0) ? far : near;
		}		
        // near < 0 means ray starts inside
		return true;
	}
	Vec3 color()
	{
		return m_color;
	}
	float reflection_ratio()
	{
		return m_reflection;
	}
	float transparency()
	{
		return m_transparency;
	}
protected:
    Vec3            m_center;
    float           m_radius;
	Vec3            m_color;
	float           m_reflection;
	float           m_transparency;
};

class Light
{
public:
    this(Vec3 p, Vec3 clr)
	{
		m_position = p;
		m_color = clr;
	}

    Vec3 position() { return m_position; }
    Vec3 color()    { return m_color;  }
protected:
    Vec3 m_position;
    Vec3 m_color;
};

struct Scene
{
	Sphere[] objects;
	Light[]  lights;
}

Vec3 trace (Ray ray, Scene scene, int depth)
{
	auto nearest = float.max;
	Sphere obj = null;

    // search the scene for nearest intersection
    foreach(o; scene.objects)
    {
		auto distance = float.max;
		if (o.intersect(ray, &distance))
        {
			if (distance < nearest)
            {
				nearest = distance;
				obj = o;
			}
		}
	}

    if (obj is null)
		return Vec3(0);

	auto point_of_hit = ray.start + ray.dir * nearest;
	auto normal = obj.normal(point_of_hit);
	bool inside = false;

	if (dot(normal, ray.dir) > 0)
    {
        inside = true;
        normal = -normal;
    }

	Vec3 color = Vec3(0.0f);

    foreach(l; scene.lights)
    {
        auto light_direction = normalize(l.position() - point_of_hit);
		Ray r = Ray(point_of_hit + normal * 1e-5, light_direction);

        // go through the scene check whether we're blocked from the lights
        bool blocked = any!((o){return o.intersect(r);})(scene.objects);

        if (!blocked)
            color = color + l.color()
			* max(0, dot(normal,light_direction))
			* obj.color();
    }

    float facing = max(0, -ray.dir.dot(normal));
	auto reflection_ratio = obj.reflection_ratio();
	auto transparency = obj.transparency();
	auto ior = 1.5f;
    float fresneleffect = reflection_ratio + (1 - reflection_ratio) * pow((1 - facing), 5);

    // compute reflection
    if (depth < max_depth && reflection_ratio > 0)
    {
        auto reflection_direction = ray.dir + normal * 2 * ray.dir.dot(normal) * (-1.0);
        auto reflection = trace(Ray(point_of_hit + normal * 1e-5, reflection_direction),
                                scene, depth + 1);
        color = color + reflection * fresneleffect;
    }

    // compute refraction
    if (depth < max_depth && (transparency > 0))
    {
		auto CE = ray.dir.dot(normal) * (-1.0f);
        ior = inside ? (1.0f) / ior : ior;
        auto eta = (1.0f) / ior;
        auto GF = (ray.dir + normal * CE) * eta;
        auto sin_t1_2 = 1 - CE * CE;
        auto sin_t2_2 = sin_t1_2 * (eta * eta);
        if (sin_t2_2 < 1)
        {
            auto GC = normal * sqrt(1 - sin_t2_2);
            auto refraction_direction = GF - GC;
            auto refraction = trace(Ray(point_of_hit - normal * 1e-4, refraction_direction),
                                    scene, depth + 1);
            color = color + refraction * (1 - fresneleffect) * transparency;
        }
    }

	return color;
}

void render (Scene scene, SDL_Surface* surface)
{
	Vec3 eye = Vec3(0);
    float h = tan(cast(float)fov / 360 * 2 * PI / 2) * 2;
    float w = h * width / height;

    SDL_LockSurface(surface);

	auto start = Clock.currTime();

	foreach (y; parallel(sequence!("n")()[0..height]))
    {
		uint[] row = (cast(uint*)(surface.pixels + surface.pitch * y))[0..width];
		foreach (x; 0..width)
		{
			float xx = x, yy = y, ww = width, hh = height;
            Vec3 dir = normalize(Vec3([(xx - ww / 2.0f) / ww  * w,
                                       (hh/2.0f - yy) / hh * h,
                                       -1.0f]));
            auto pixel = trace(Ray(eye, dir), scene, 0);
			auto rgb = map!("cast(ubyte)min(255, a*255+0.5)")(pixel[]);
            row[x] = SDL_MapRGB(surface.format, rgb[0], rgb[1], rgb[2]);
		}
    }

	auto elapsed = Clock.currTime() - start;
	writefln("%s", elapsed);

	SDL_UnlockSurface(surface);
    SDL_UpdateRect(surface, 0, 0, 0, 0);
}

int main()
{
	DerelictSDL.load();
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Surface* screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
	if (!screen)
		return 1;

	Scene scene;
	scene.objects = [new Sphere(Vec3([0.0f, -10002.0f, -20.0f]), 10000, Vec3([.8, .8, .8])),
	new Sphere(Vec3([0.0f, 2.0f, -20.0f]), 4		  , Vec3([.8, .5, .5]), 0.5),
	new Sphere(Vec3([5.0f, 0.0f, -15.0f]), 2		  , Vec3([.3, .8, .8]), 0.2),
	new Sphere(Vec3([-5.0f, 0.0f, -15.0f]),2		  , Vec3([.3, .5, .8]), 0.2),
	new Sphere(Vec3([-2.0f, -1.0f, -10.0f]),1         , Vec3([.1, .1, .1]), 0.1, 0.8)];
	scene.lights = [ new Light(Vec3([10, 20, 30]), Vec3([2, 2, 2])) ];
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
			default:
				break;
		}
	}

    return 0;
}
