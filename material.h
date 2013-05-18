#pragma once

template<typename T>
struct Material
{
    virtual Vec3<T> diffuse   (const Vec3<T>& pos) const = 0;
    virtual T       reflection()   const  { return T(0); }
    virtual T       transparency() const  { return T(0); }
    virtual T       ior()          const  { return T(1); }
};

template<typename T>
struct Shiny : Material<T>
{
    Vec3<T> diffuse (const Vec3<T>& pos) const { return Vec3<T>{.6,.6,.6}; }
    T       reflection() const { return T(0.5); }
};

template<typename T>
struct CheckerBoard : Material<T>
{
    Vec3<T> diffuse (const Vec3<T>& pos) const
    {
        if ((int(pos[2]*.5) + int(pos[0]*.5)) % 2)
            return { 1, 1, 1 };
        else
            return { 0, 0, 0 };
    }
    T reflection() const { return T(0.6); }
};

template<typename T>
struct Glass : Material<T>
{
    Vec3<T> diffuse   (const Vec3<T>& pos) const { return Vec3<T>{ .1,.2,.1 }; }
    T       reflection()   const { return T(0.3); }
    T       transparency() const { return T(.7); }
    T       ior()          const { return T(1.4); }
};
