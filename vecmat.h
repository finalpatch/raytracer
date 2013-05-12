#pragma once

#include <iostream>
#include <algorithm>

template<typename T, int N>
class Vec
{
public:
    Vec() { std::fill_n(m_vec, N, T()); }
    Vec(const T& v) { std::fill_n(m_vec, N, v); }
    Vec(const Vec& other) { std::copy_n(other.m_vec, N, m_vec); }
    Vec(std::initializer_list<T> l) { std::copy_n(l.begin(), N, m_vec); }
    
    T& operator[] (const int i) {return m_vec[i];}
    const T& operator[] (const int i) const {return m_vec[i];}

    // ***********
    Vec& operator *= (const T& x)
    {
        for(int i = 0; i < N; ++i)
            m_vec[i] *= x;
        return *this;
    }
    Vec operator * (const T& x)
    {
        Vec<T, N> t(*this);
        t *= x;
        return t;
    }
    // ************
    Vec& operator *= (const Vec& v)
    {
        for(int i = 0; i < N; ++i)
            m_vec[i] *= v.m_vec[i];
        return *this;
    }    
    Vec operator * (const Vec& v) const
    {
        Vec<T, N> t(*this);
        t *= v;
        return t;
    }
    // ************
    Vec& operator += (const Vec& v)
    {
        for(int i = 0; i < N; ++i)
            m_vec[i] += v.m_vec[i];
        return *this;
    }    
    Vec operator + (const Vec& v) const
    {
        Vec<T, N> t(*this);
        t += v;
        return t;
    }
    // ************
    Vec& operator -= (const Vec& v)
    {
        for(int i = 0; i < N; ++i)
            m_vec[i] -= v.m_vec[i];
        return *this;
    }
    Vec operator - (const Vec& v) const
    {
        Vec<T, N> t(*this);
        t -= v;
        return t;
    }
    // ************
    Vec operator - () const
    {
        Vec<T, N> t(*this);
        t *= -1;
        return t;
    }
    // ***********
    T dot(const Vec& v) const
    {
        T acc = T();
        for(int i = 0; i < N; ++i)
            acc += m_vec[i] * v[i];
        return acc;
    }
    // **********
    T magnitude() const
    {
        return sqrt(dot(*this));
    }
    // **********
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
protected:
    T m_vec[N];
};

template <typename T, int N>
class Mat
{
public:
    Mat() {}
    
    Mat(std::initializer_list<T> l)
    {
        auto p = l.begin();
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                m_rows[i][j] = *p++;
    }
    
    Vec<T,N>& operator[] (const int i) {return m_rows[i];}
    const Vec<T,N>& operator[] (const int i) const {return m_rows[i];}
    
    void transpose()
    {
        for (int i = 1; i < (N - 1); ++i)
            for (int j = i + 1; j < N; ++j)
                std::swap(m_rows[i][j], m_rows[j][i]);
    }
    
    Mat& operator *= (Mat right)
    {
        right.transpose(); 
        for(int i = 0; i < N; ++i)
        {
            Vec<T, N> t = m_rows[i];
            for(int j = 0; j < N; ++j)
                m_rows[i][j] = t.dot(right[j]);
        }
        return *this;
    }

    Mat operator * (const Mat& right) const
    {
        Mat t(*this);
        t *= right;
        return t;
    }
    
private:
    Vec<T, N> m_rows[N];
};

template<typename T, int N>
Vec<T, N> operator * (Vec<T, N> v, Mat<T, N> m)
{
    Vec<T, N> t;
    m.transpose();
    for(int i = 0; i < N; ++i)
        t[i] = v.dot(m[i]);
    return t;
}

template<typename T, int N>
Vec<T, N>& operator *= (Vec<T, N>& v, Mat<T, N> m)
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
