#pragma once
#include <cmath>

constexpr float g_dt = 0.6;
constexpr float g_G = 1;

struct Vec2 
{
    float x, y;
};

// No velocity or forces in object class because I don't give a fuck.
struct Object 
{
    float m=1; // mass
    Vec2 p;
};

inline float Dist (Object const& o1, Object const& o2)
{
    return sqrt((o1.p.x - o2.p.x) * (o1.p.x - o2.p.x) + (o1.p.y - o2.p.y) * (o1.p.y - o2.p.y));
}

inline Vec2 GetForce (Object const& o1, Object const& o2, bool div=true)
{
    // Force vector = F * normalize(o2 - o1) = (o2 - o1) * m1 * m2 / |o1-o2|^3
    float F = o1.m * o2.m;
    if(!div)
        F /= pow(Dist(o1, o2),3);
    else
        F *= Dist(o1, o2);
    return {F * (o2.p.x - o1.p.x), F * (o2.p.y - o1.p.y)};
}

// b is ball, a1-3 are attractors and are STATIONARY.
inline void Simulate (Object& b, Object const& a1, Object const& a2, Object const& a3, Object const& a4, Object const& a5, Object const& a6) 
{
    Vec2 F1 = GetForce(b, a1), F2 = GetForce(b, a2), F3 = GetForce(b, a3), F4 = GetForce(b, a4, false), F5 = GetForce(b, a5, false), F6 = GetForce(b, a6, false);
    b.p.x += g_dt * g_dt * (F1.x + F2.x + F3.x + F4.x + F5.x + F6.x);
    b.p.y += g_dt * g_dt * (F1.y + F2.y + F3.y + F4.y + F5.y + F6.y);
}
