#pragma once

template<typename T>
class Material
{
public:
    virtual Vec3<T> diffuse   (const Vec3<T>& pos) const = 0;
    virtual T       reflection(const Vec3<T>& pos) const = 0;
};

template<typename T>
class CheckerBoard : public Material<T>
{
public:
    virtual Vec3<T> diffuse   (const Vec3<T>& pos) const
    {
        if ((int(pos[2]*.5) + int(pos[0]*.5)) % 2)
            return { 1, 1, 1 };
        else
            return { 0, 0, 0 };
    }
    virtual T       reflection(const Vec3<T>& pos) const { return T(0.6); }
};

template<typename T>
class Shiny : public Material<T>
{
public:
    virtual Vec3<T> diffuse   (const Vec3<T>& pos) const { return Vec3<T>{.6,.8,1}; }
    virtual T       reflection(const Vec3<T>& pos) const { return T(0.5); }
};
