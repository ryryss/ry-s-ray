#ifndef	ALGORITHM_H
#define ALGORITHM_H
#include "pub.h"

namespace alg {
/**
 * @brief Computes the intersection of a ray with a triangle using the Möller–Trumbore algorithm.
 *
 * @param o      [in]  The origin of the ray in world coordinates.
 * @param d      [in]  The direction of the ray (should be normalized).
 * @param a b c  [in]  Triangle.
 * @param t      [out] Distance from ray origin to intersection point along the ray.
 * @param g_u    [out] Barycentric coordinate u at the intersection point.
 * @param g_v    [out] Barycentric coordinate v at the intersection point.
 *
 * @return true if the ray intersects the triangle, false otherwise.
 *
 * @note  - The intersection point can be computed as:
 *          P = orig + t * dir
 *        - The third barycentric coordinate w can be computed as w = 1 - u - v
 *        - The function assumes a right-handed coordinate system.
 */
bool Moller_Trumbore(const ry::vec3& o, const ry::vec3& d, const ry::vec3& a,
    const ry::vec3& b, const ry::vec3& c, float& t, float& g_u, float& g_v);
/**
 * @brief Lambertian Shading.
 *
 * @param kd        [in] The diffuse coefficient, or the surface color.
 * @param intensity [in] The intensity of the light source.
 * @param l         [in] The direction of light.
 * @param n         [in] The surface normal.
 * 
 * @return the pixel color.
 */
ry::vec3 LambertianShading(const ry::vec3& kd, float intensity, const ry::vec3& d, const ry::vec3& n);
/**
 * @brief Blinn-Phong Shading.
 *
 * @param kd        [in] The diffuse coefficient, or the surface color.
 * @param ks        [in] The specular coefficient, or the specular color of the surface.
 * @param intensity [in] The intensity of the light source.
 * @param l         [in] The direction of hitpoint to light.
 * @param n         [in] The surface normal.
 * @param v         [in] The direction of hitpoint to eye(camera).
 *
 * @return the pixel color.
 */
ry::vec3 BlinnPhongShading(const ry::vec3& kd, const ry::vec3& ks, float intensity,
    const ry::vec3& l, const ry::vec3& n, const ry::vec3& v);
}
#endif