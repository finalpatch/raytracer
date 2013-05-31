import std.math;
import std.algorithm;
import std.parallelism;
import std.range;
import std.datetime;
import std.stdio;
import derelict.sdl.sdl;

immutable width     = 1280;
immutable height    = 720;
immutable fov       = 45;
immutable max_depth = 6;

struct Vec3
{
    this(float x)
    {
        v[0] = x;
        v[1] = x;
        v[2] = x;
    }
    this(float x, float y, float z)
    {
        v[0] = x;
        v[1] = y;
        v[2] = z;
    }
    Vec3 opUnary(string op)() const
        if( op =="-" )
    {
        Vec3 t;
        t.v[0] = v[0] * (-1);
        t.v[1] = v[1] * (-1);
        t.v[2] = v[2] * (-1);
        return t;
    }
    Vec3 opBinary(string op)(float rhs) const
        if( op == "+" || op =="-" || op=="*" || op=="/" )
    {
        Vec3 t;
        t.v[0] = mixin("v[0]"~op~"rhs");
        t.v[1] = mixin("v[1]"~op~"rhs");
        t.v[2] = mixin("v[2]"~op~"rhs");
        return t;
    }
    Vec3 opBinary(string op)(Vec3 rhs) const
        if( op == "+" || op =="-" || op=="*" || op=="/" )
    {
       Vec3 t;
       t.v[0] = mixin("v[0] "~op~" rhs.v[0]");
       t.v[1] = mixin("v[1] "~op~" rhs.v[1]");
       t.v[2] = mixin("v[2] "~op~" rhs.v[2]");
       return t;
    }
    ref Vec3 opOpAssign(string op)(Vec3 rhs)
        if( op == "+" || op =="-" || op=="*" || op=="/" )
    {
        mixin("v[0]"~op~"=rhs.v[0];");
        mixin("v[1]"~op~"=rhs.v[1];");
        mixin("v[2]"~op~"=rhs.v[2];");
        return this;
    }
    float[] opSlice()
    {
        return v;
    }
    float[3] v;
}

float dot(Vec3 v1, Vec3 v2)
{
    return v1.v[0]*v2.v[0]+v1.v[1]*v2.v[1]+v1.v[2]*v2.v[2];
}
float magnitude(Vec3 v)
{
    return sqrt(dot(v, v));
}
Vec3 normalize(Vec3 v)
{
    float[3] t = v.v[] / magnitude(v);
    return Vec3(t[0], t[1], t[2]);
}

struct Ray
{
    Vec3 start;
    Vec3 dir;

    this(Vec3 _start, Vec3 _dir)
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
    final Vec3 normal(Vec3 pos) const
    {
        return normalize(pos - m_center);
    }
    final bool intersect(Ray ray, float* distance = null) const
    {
        auto l = m_center - ray.start;
        auto a = dot(l, ray.dir);
        if (a < 0)              // opposite direction
            return false;
        auto b2 = dot(l, l) - a * a;
        auto r2 = m_radius * m_radius;
        if (b2 > r2)            // perpendicular > r
            return false;
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
    final Vec3 color() const
    {
        return m_color;
    }
    final float reflection_ratio() const
    {
        return m_reflection;
    }
    final float transparency() const
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

    final Vec3 position() const { return m_position; }
    final Vec3 color()    const { return m_color;  }
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
    auto reflection_ratio = obj.reflection_ratio();

    foreach(l; scene.lights)
    {
        auto light_direction = normalize(l.position() - point_of_hit);
        immutable r = Ray(point_of_hit + normal * 1e-5, light_direction);

        // go through the scene check whether we're blocked from the lights
        bool blocked = any!(o => o.intersect(r))(scene.objects);

        if (!blocked)
            color += l.color()
                * max(0, dot(normal,light_direction))
                * obj.color()
                * (1.0f - reflection_ratio);
    }

    float facing = max(0, -ray.dir.dot(normal));
    float fresneleffect = reflection_ratio + (1 - reflection_ratio) * pow((1 - facing), 5);

    // compute reflection
    if (depth < max_depth && reflection_ratio > 0)
    {
        auto reflection_direction = ray.dir + normal * 2 * ray.dir.dot(normal) * (-1.0);
        auto reflection = trace(Ray(point_of_hit + normal * 1e-5, reflection_direction), scene, depth + 1);
        color += reflection * fresneleffect;
    }

    // compute refraction
    if (depth < max_depth && (obj.transparency() > 0))
    {
        auto ior = 1.5f;
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
            color += refraction * (1 - fresneleffect) * obj.transparency();
        }
    }
    return color;
}

void render (Scene scene, SDL_Surface* surface)
{
    SDL_LockSurface(surface);

    immutable eye = Vec3(0);
    float h = tan(cast(float)fov / 360 * 2 * PI / 2) * 2;
    float w = h * width / height;

    foreach (y; (iota(height)))
    {
        uint* row = cast(uint*)(surface.pixels + surface.pitch * y);
        foreach (x; 0..width)
        {
            float xx = x, yy = y, ww = width, hh = height;
            Vec3 dir = normalize(Vec3((xx - ww / 2.0f) / ww  * w,
                                       (hh/2.0f - yy) / hh * h,
                                       -1.0f));
            auto pixel = trace(Ray(eye, dir), scene, 0);
            auto rgb = map!("cast(ubyte)min(255, a*255+0.5)")(pixel[]);
            row[x] = SDL_MapRGB(surface.format, rgb[0], rgb[1], rgb[2]);
        }
    }
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
    scene.objects = [new Sphere(Vec3(0.0f, -10002.0f, -20.0f), 10000, Vec3(.8, .8, .8)),
                     new Sphere(Vec3(0.0f, 2.0f, -20.0f), 4         , Vec3(.8, .5, .5), 0.5),
                     new Sphere(Vec3(5.0f, 0.0f, -15.0f), 2         , Vec3(.3, .8, .8), 0.2),
                     new Sphere(Vec3(-5.0f, 0.0f, -15.0f),2         , Vec3(.3, .5, .8), 0.2),
                     new Sphere(Vec3(-2.0f, -1.0f, -10.0f),1        , Vec3(.1, .1, .1), 0.1, 0.8)];
    scene.lights = [ new Light(Vec3(-10, 20, 30), Vec3(2, 2, 2)) ];

    auto start = Clock.currTime();
    render(scene, screen);
    auto elapsed = Clock.currTime() - start;
    writefln("%s", elapsed);

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
