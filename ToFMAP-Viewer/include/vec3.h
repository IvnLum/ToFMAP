#ifndef __VEC3__
#define __VEC3__
#endif /* !__VEC3__ */

#ifdef __cplusplus
extern "C"
{
#endif

#define Vec3(T)\
typedef struct Vec3_##T\
{\
	T x, y, z;\
} Vec3_##T;\
\
\
Vec3_##T get_Vec3_##T(T, T, T);\
\
void vec3_adds_##T(Vec3_##T*, T);\
void vec3_muls_##T(Vec3_##T*, T);\
\
void vec3_addv_##T(Vec3_##T*, Vec3_##T);\
void vec3_subv_##T(Vec3_##T*, Vec3_##T);\
void vec3_mulv_##T(Vec3_##T*, Vec3_##T);\
void vec3_divv_##T(Vec3_##T*, Vec3_##T);\
\
Vec3_##T get_Vec3_##T(T x, T y, T z)\
{\
	return (Vec3_##T) {\
		.x = x,\
		.y = y,\
		.z = z\
	};\
}\
void vec3_adds_##T(Vec3_##T *v, T s)\
{\
	v->x += s;\
	v->y += s;\
	v->z += s;\
}\
void vec3_muls_##T(Vec3_##T *v, T s)\
{\
	v->x *= s;\
	v->y *= s;\
	v->z *= s;\
}\
void vec3_addv_##T(Vec3_##T *v, Vec3_##T w)\
{\
	v->x += w.x;\
	v->y += w.y;\
	v->z += w.z;\
}\
void vec3_subv_##T(Vec3_##T *v, Vec3_##T w)\
{\
	v->x -= w.x;\
	v->y -= w.y;\
	v->z -= w.z;\
}\
void vec3_mulv_##T(Vec3_##T *v, Vec3_##T w)\
{\
	v->x *= w.x;\
	v->y *= w.y;\
	v->z *= w.z;\
}\
void vec3_divv_##T(Vec3_##T *v, Vec3_##T w)\
{\
	v->x = (T)v->x / w.x;\
	v->y = (T)v->y / w.y;\
	v->z = (T)v->z / w.z;\
}\
\
T vec3_get_mod_##T(Vec3_##T *v)\
{\
	return sqrt(\
			v->x * v->x\
			+ v->y * v->y\
			+ v->z * v->z\
			);\
}\
\
void vec3_normalize_##T(Vec3_##T *v)\
{\
	vec3_muls_##T(v, (T)1 / vec3_get_mod_##T(v));\
}\
void vec3_set_mod_##T(Vec3_##T *v, T target)\
{\
	vec3_muls_##T(v, (T)target / vec3_get_mod_##T(v));\
}

#ifdef __cplusplus
}
#endif
