#pragma once

#include <array>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <numeric>

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
    Vec& operator *= (const T& x)
    {
        return transform([&] (T v) { return v * x; } );
    }
    Vec operator * (const T& x)
    {
        Vec<T, N> t(*this); t *= x; return t;
    }
    // *** multiply vector
    Vec& operator *= (const Vec& v)
    {
        return transform(v, std::multiplies<T>());
    }    
    Vec operator * (const Vec& v) const
    {
        Vec<T, N> t(*this); t *= v; return t;
    }
    // *** add vector
    Vec& operator += (const Vec& v)
    {
        return transform(v, std::plus<T>());
    }    
    Vec operator + (const Vec& v) const
    {
        Vec<T, N> t(*this); t += v; return t;
    }
    // *** subtract vectot
    Vec& operator -= (const Vec& v)
    {
        return transform(v, std::minus<T>());
    }
    Vec operator - (const Vec& v) const
    {
        Vec<T, N> t(*this); t -= v; return t;
    }
    // *** reverse
    Vec operator - () const
    {
        return (*this) * T(-1);
    }
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

template <typename T, std::size_t N>
class Mat : public std::array<Vec<T, N>, N>
{
public:
    Mat() = default;
    Mat(const Mat& other) = default;
    Mat(std::initializer_list<T> l)
    {
        auto p = l.begin();
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                (*this)[i][j] = *p++;
    }
    void transpose()
    {
        for (int i = 1; i < (N - 1); ++i)
            for (int j = i + 1; j < N; ++j)
                std::swap((*this)[i][j], (*this)[j][i]);
    }    
    Mat& operator *= (Mat right)
    {
        right.transpose(); 
        for(int i = 0; i < N; ++i)
        {
            Vec<T, N> t = (*this)[i];
            for(int j = 0; j < N; ++j)
                (*this)[i][j] = t.dot(right[j]);
        }
        return *this;
    }
    Mat operator * (const Mat& right) const
    {
        Mat t(*this);
        t *= right;
        return t;
    }
};

// *** vector * matrix
template<typename T, int N>
Vec<T, N> operator * (const Vec<T, N>& v, Mat<T, N> m)
{
    Vec<T, N> t;
    m.transpose();
    for(int i = 0; i < N; ++i)
        t[i] = v.dot(m[i]);
    return t;
}

template<typename T, int N>
Vec<T, N>& operator *= (Vec<T, N>& v, const Mat<T, N>& m)
{
    v = v * m;
    return v;
}

template<typename T, int N>
std::ostream & operator << (std::ostream &os, const Vec<T, N>& v)
{
    os << "[" << v[0];
    for(int i = 1; i < N; ++i)
        os << " " << v[i];
    os << "]";
    return os;
}

template<typename T, int N>
std::ostream & operator << (std::ostream &os, const Mat<T, N>& m)
{
    os << "[" << m[0];
    for(int i = 1; i < N; ++i)
        os << "\n " << m[i];
    os << "]";
    return os;
}

template <typename T>
using Vec3 = Vec<T, 3>;
