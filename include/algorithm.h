#ifndef	ALGORITHM
#define ALGORITHM
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
 * @param albedo    [in] The diffuse coefficient, or the surface color.
 * @param intensity [in] The intensity of the light source.
 * @param d         [in] The direction of light.
 * @param n         [in] The surface normal.
 * 
 * @return the pixel color.
 */
ry::vec4 LambertianShading(const ry::vec4& albedo, float intensity, const ry::vec3& d, const ry::vec3& n);
}
#endif