#pragma once
#include "Utils.hpp"
//TODO: Prototype math! NOT FINAL
#include <cmath> //TODO: Remove it later
using std::sqrt;

union Vec2
{
	f32 e[2];

	struct
	{
		f32 x;
		f32 y;
	};

	Vec2 operator-() const { return Vec2{-e[0], -e[1]}; }
	f32 operator[](s32 i) const { return e[i]; }
	f32 &operator[](s32 i) { return e[i]; }

	Vec2 &operator+=(const Vec2 &v)
	{
		e[0] += v.e[0];
		e[1] += v.e[1];
		return *this;
	}

	Vec2 &operator*=(const f32 t)
	{
		e[0] *= t;
		e[1] *= t;
		return *this;
	}

	Vec2 &operator/=(const f32 t)
	{
		return *this *= 1 / t;
	}

	f32 length() const
	{
		return sqrt(length_squared());
	}

	f32 length_squared() const
	{
		return e[0] * e[0] + e[1] * e[1];
	}
};

inline Vec2 operator+(const Vec2 &u, const Vec2 &v)
{
	return Vec2{u.e[0] + v.e[0], u.e[1] + v.e[1]};
}

inline Vec2 operator-(const Vec2 &u, const Vec2 &v)
{
	return Vec2{u.e[0] - v.e[0], u.e[1] - v.e[1]};
}

inline Vec2 operator*(const Vec2 &u, const Vec2 &v)
{
	return Vec2{u.e[0] * v.e[0], u.e[1] * v.e[1]};
}

inline Vec2 operator*(f32 t, const Vec2 &v)
{
	return Vec2{t * v.e[0], t * v.e[1]};
}

inline Vec2 operator*(const Vec2 &v, f32 t)
{
	return t * v;
}

inline Vec2 operator/(Vec2 v, f32 t)
{
	return (1 / t) * v;
}

inline f32 dot(const Vec2 &u, const Vec2 &v)
{
	return u.e[0] * v.e[0] + u.e[1] * v.e[1];
}

inline f32 cross(const Vec2 &u, const Vec2 &v)
{
	return u.x * v.y - u.y * v.x;
}

inline Vec2 unit_vector(Vec2 v)
{
	return {v.x / v.length(), v.y / v.length()};
}

union Vec3
{

	f32 e[3];

	struct
	{
		f32 x;
		f32 y;
		f32 z;

		f32 r;
		f32 g;
		f32 b;
	};

	Vec3 operator-() const { return Vec3{-e[0], -e[1], -e[2]}; }
	f32 operator[](s32 i) const { return e[i]; }
	f32 &operator[](s32 i) { return e[i]; }

	Vec3 &operator+=(const Vec3 &v)
	{
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}

	Vec3 &operator*=(const f32 t)
	{
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	Vec3 &operator/=(const f32 t)
	{
		return *this *= 1 / t;
	}

	f32 length() const
	{
		return sqrt(length_squared());
	}

	f32 length_squared() const
	{
		return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
	}
};

using color3f = Vec3; // RGB color

inline Vec3 operator+(const Vec3 &u, const Vec3 &v)
{
	return Vec3{u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]};
}

inline Vec3 operator-(const Vec3 &u, const Vec3 &v)
{
	return Vec3{u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]};
}

inline Vec3 operator*(const Vec3 &u, const Vec3 &v)
{
	return Vec3{u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]};
}

inline Vec3 operator*(f32 t, const Vec3 &v)
{
	return Vec3{t * v.e[0], t * v.e[1], t * v.e[2]};
}

inline Vec3 operator*(const Vec3 &v, f32 t)
{
	return t * v;
}

inline Vec3 operator/(Vec3 v, f32 t)
{
	return (1 / t) * v;
}

inline f32 dot(const Vec3 &u, const Vec3 &v)
{
	return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

inline Vec3 cross(const Vec3 &u, const Vec3 &v)
{
	return Vec3{u.e[1] * v.e[2] - u.e[2] * v.e[1],
				u.e[2] * v.e[0] - u.e[0] * v.e[2],
				u.e[0] * v.e[1] - u.e[1] * v.e[0]};
}

inline Vec3 unit_vector(Vec3 v)
{
	return {v.x / v.length(), v.y / v.length(), v.z / v.length()};
}