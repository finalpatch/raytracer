import std.math;
import std.algorithm;
import std.parallelism;
import std.range;
import std.datetime;
import std.stdio;
import std.conv;
import std.string;
import derelict.sdl2.sdl;

immutable width     = 1280;
immutable height    = 720;
immutable fov       = 45;
immutable max_depth = 6;

template Unroll(alias CODE, alias N, alias SEP="")
{
    enum t = replace(CODE, "%", "%1$d");
    enum Unroll = iota(N).map!(i => format(t, i)).join(SEP);
}

struct Vec(T, alias N)
{
    enum size = N;
    alias T valtype;
    
    this (T...) (T args)
    {
        static if(args.length == 1)
            mixin(Unroll!("v[%]=args[0];", N));
        else static if(args.length == N)
            mixin(Unroll!("v[%]=args[%];", N));
        else
            static assert("wrong number of arguments");
    }
    Vec opUnary(string op)() const
        if( op =="-" )
    {
        Vec t;
        mixin(Unroll!("t.v[%]=v[%] * (-1);", N));
        return t;
    }
    Vec opBinary(string op)(T rhs) const
        if( op == "+" || op =="-" || op=="*" || op=="/" )
    {
        Vec t;
        mixin(Unroll!("t.v[%]=v[%]"~op~"rhs;", N));
        return t;
    }
    Vec opBinary(string op)(Vec rhs) const
        if( op == "+" || op =="-" || op=="*" || op=="/" )
    {
        Vec t;
        mixin(Unroll!("t.v[%]=v[%]"~op~"rhs.v[%];", N));
        return t;
    }
    ref Vec opOpAssign(string op)(Vec rhs)
        if( op == "+" || op =="-" || op=="*" || op=="/" )
    {
        mixin(Unroll!("v[%]"~op~"=rhs.v[%];", N));
        return this;
    }
    T[] opSlice()
    {
        return v;
    }
    T[N] v;
}

V.valtype dot(V)(V v1, V v2)
{
    return mixin(Unroll!("v1.v[%]*v2.v[%]", V.size, "+"));
}
V.valtype magnitude(V)(V v)
{
    return sqrt(dot(v, v));
}
auto normalize(V)(V v)
{
    v.v[] /= magnitude(v);
    return v;
}

alias Vec!(float,3) Vec3;

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
    this(Vec3 c, float r, Vec3 clr, float refl = 0, float trans = 0, bool chkbrd = false)
    {
        m_center = c;
        m_radius = r;
        m_color = clr;
        m_reflection = refl;
        m_transparency = trans;
        m_checkerBoard = chkbrd;
    }
    final Vec3 normal(Vec3 pos) const
    {
        return (pos - m_center).normalize();
    }
    final bool intersect(Ray ray, float* distance = null) const
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
        if (distance != null)
        {
            auto near = a - c;
            auto far  = a + c;
            *distance = (near < 0) ? far : near;
        }
        // near < 0 means ray starts inside
        return true;
    }
    final Vec3 color(Vec3 pos) const
    {
        static immutable black = Vec3(0);
        static immutable white = Vec3(1);
        if (m_checkerBoard)
            return ((to!int(pos.v[2]) + to!int(pos.v[0])) % 2) ? black : white;
        else
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
    Vec3  m_center;
    float m_radius;
    Vec3  m_color;
    float m_reflection;
    float m_transparency;
    bool  m_checkerBoard;
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

    if (normal.dot(ray.dir) > 0)
    {
        inside = true;
        normal = -normal;
    }

    Vec3 color = Vec3(0.0f);
    auto reflection_ratio = obj.reflection_ratio();

    foreach(l; scene.lights)
    {
        auto light_direction = (l.position() - point_of_hit).normalize();
        immutable r = Ray(point_of_hit + normal * 1e-5, light_direction);

        // go through the scene check whether we're blocked from the lights
        bool blocked = any!(o => o.intersect(r))(scene.objects);

        if (!blocked)
        {
            color += l.color()
                * max(0, normal.dot(light_direction))
                * obj.color(point_of_hit)
                * (1.0f - reflection_ratio);
            auto half_angle = (-ray.dir + light_direction).normalize();
            auto specular = pow(max(0, normal.dot(half_angle)), 30);
            color += l.color() * obj.color(point_of_hit) * specular * .5;
        }
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
            auto refraction = trace(Ray(point_of_hit - normal * 1e-5, refraction_direction),
                                    scene, depth + 1);
            color += refraction * (1 - fresneleffect) * obj.transparency();
        }
    }
    return color;
}

uint[] render (Scene scene, int width, int height)
{
    uint[] buffer = new uint[width*height];
    immutable eye = Vec3(0);
    float h = tan(cast(float)fov / 360 * 2 * PI / 2) * 2;
    float w = h * width / height;
    foreach (y; parallel(iota(height))) // line
    {
        uint* row = buffer.ptr + width * y;
        foreach (x; 0..width)   // pixel
        {
            Vec3 pixel = 0.0f;
            foreach (suby; 0..2) // subpixel
            {
                foreach (subx; 0..2)
                {
                    float xx = x, yy = y, ww = width, hh = height;
                    xx += 0.5f*subx; yy += 0.5f*suby;
                    Vec3 dir = (Vec3((xx - ww / 2.0f) / ww  * w,
                                     (hh/2.0f - yy) / hh * h,
                                     -1.0f)).normalize();
                    pixel += trace(Ray(eye, dir), scene, 0);
                }
            }
            auto rgb = map!(a => cast(ubyte)min(255, a*0.25*255+0.5))(pixel[]);
            row[x] = rgb[2] | (rgb[1] << 8) | (rgb[0] << 16);
        }
    }
    return buffer;
}

int main()
{
    DerelictSDL2.load();
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window*   win;
    SDL_Renderer* ren;
    SDL_Texture*  tex;

    scope(exit)
        SDL_Quit();

    if (SDL_CreateWindowAndRenderer(width, height, 0, &win, &ren) < 0)
    {
        writefln("%s", SDL_GetError());
        return -1;
    }
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!tex)
    {
        writefln("%s", SDL_GetError());
        return -1;
    }

    Scene scene;
    scene.objects = [new Sphere(Vec3(0.0f, -10002.0f, -20.0f), 10000, Vec3(.8, .8, .8), 0.0, 0.0, true),
                     new Sphere(Vec3(0.0f, 2.0f, -20.0f), 4         , Vec3(.8, .5, .5), 0.2),
                     new Sphere(Vec3(5.0f, 0.0f, -15.0f), 2         , Vec3(.3, .8, .8), 0.1),
                     new Sphere(Vec3(-5.0f, 0.0f, -15.0f),2         , Vec3(.3, .5, .8), 0.1),
                     new Sphere(Vec3(-2.0f, -1.0f, -10.0f),1        , Vec3(.1, .1, .1), 0.1, 0.8)];
    scene.lights = [ new Light(Vec3(-10, 20, 30), Vec3(2, 2, 2)) ];

    auto start = Clock.currTime();
    uint[] buffer = render(scene, width, height);
    auto elapsed = Clock.currTime() - start;
    writefln("%s", elapsed);

    SDL_UpdateTexture(tex, null, buffer, width * uint.sizeof);
    SDL_RenderCopy(ren, tex, null, null);
    SDL_RenderPresent(ren);

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
