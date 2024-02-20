#include "geometry.h"

template <> template <> vec<3, int>  ::vec(const vec<3, float>& v) : x(int(v.x + .5f)), y(int(v.y + .5f)), z(int(v.z + .5f)) {}
template <> template <> vec<3, float>::vec(const vec<3, int>& v) : x(v.x), y(v.y), z(v.z) {}
template <> template <> vec<2, int>  ::vec(const vec<2, float>& v) : x(int(v.x + .5f)), y(int(v.y + .5f)) {}
template <> template <> vec<2, float>::vec(const vec<2, int>& v) : x(v.x), y(v.y) {}
template <> template <> vec<3, float>::vec(const vec<4, float>& v) : x(v[0]), y(v[1]), z(v[2]) {}
template <> template <> vec<3, int>::vec(const vec<4, float>& v) : x(int(v[0] + .5f)), y(int(v[1] + .5f)), z(int(v[2] + .5f)) {}
