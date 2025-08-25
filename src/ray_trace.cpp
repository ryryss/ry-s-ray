#include "ray_trace.h"
#include "algorithm.h"
#include "task.h"

using namespace std;
using namespace ry;
using namespace glm;
using namespace alg;

void RayTrace::SetCamera(const ry::Camera& c)
{
    cam = c;
    up = normalize(vec3(c.m[1])); // look up
    f = normalize(vec3(c.m[2]));  // look forward
    l = vec3(c.m[3]);

    w = normalize((l - f));
    u = normalize(cross(up, f));
    v = cross(f, u);
}

vector<uint8_t> RayTrace::Excute(const Screen& s)
{
    if (s.h <= 0 && s.w <= 0) {
        return colors;
    } else {
        scr = s;
        colors.clear();
        colors.resize(s.h * s.w * 3);

        // use cam aspect ratio
        if (cam.type == "perspective") {
            cam.ymag = cam.znear * tan(cam.yfov / 2);
            cam.xmag = cam.ymag * cam.aspectRatio;
        } else {
            cam.xmag = cam.ymag * s.w / s.h;
        }
    }

    RayGeneration();
    return colors;
}

void RayTrace::RayGeneration()
{
    auto& t = Task::GetInstance();
    auto w_cnt = t.WokerCnt();
    for (auto i = 0; i < w_cnt; i ++) {
        t.Add([this, i, &t, h = scr.h / w_cnt]() {
            // use the number of pixels on the y-axis to parallel cal
            for (uint16_t y = i * h; y < (i + 1) * h; y++) {
                for (uint16_t x = 0; x < scr.w; x++) {
                    float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
                    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
                    if (RayCompute(x, y)) {
                        RayShading(x, y);
                    }
                }
            }
        });
    }
    t.AsynExcute();

    if (auto mod = scr.h % w_cnt; mod) {
        for (uint16_t y = scr.h / w_cnt * w_cnt; y < scr.h; y++) {
            for (uint16_t x = 0; x < scr.w; x++) {
                if (RayCompute(x, y)) {
                    // RayShading(x, y);
                }
            }
        }
    }
    while (!t.Free());
    // below is the serial version
    // l = -xmag  r = xmag b = -ymag t = ymag
    // u = l + (r − l)(i + 0.5)/nx
    /*for (uint16_t y = 0; y < scr.h; y++) {
        for (uint16_t x = 0; x < scr.w; x++) {
            RayCompute(x, y);
        }
    }*/
}
vec3 InterpColor(const vec3& c0, const vec3& c1, const vec3& c2,
    float b0, float b1, float b2) {
    vec3 c;
    c[0] = b0 * c0[0] + b1 * c1[0] + b2 * c2[0];
    c[0] = b0 * c0[1] + b1 * c1[1] + b2 * c2[1];
    c[2] = b0 * c0[2] + b1 * c1[2] + b2 * c2[2];

    return c;
}

bool RayTrace::RayCompute(uint32_t x, uint32_t y)
{
    vec3 o;
    vec3 d;
    float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
    if (cam.type == "perspective") {
        o = l;
        d = -cam.znear * f + u * uu + v * vv;
    } else {
        o = l + u * uu + v * vv;
        d = w;
    }
    bool hit = false;
    int tri = -1;
    float t, g_u, g_v;
    float min_t = numeric_limits<float>::max();
    for (int i = 0; i < input->size(); i ++) {
        if(Moller_Trumbore(o, d, (*input)[i].pos[0], (*input)[i].pos[1], (*input)[i].pos[2], t, g_u, g_v)) {
            if (cam.type == "perspective" || t > cam.znear && t < cam.zfar) {
                // now just orthographic can do depth test 
                if (t < min_t) {
                    min_t = t;
                    hit = true;
                    tri = i;
                }
            }
        }
    }
    if (hit) {
        auto tr = (*input)[tri];
        auto color = tr.color;// use g to cal color
        vec3 c0(color[0][0] / 255.0f, color[0][1] / 255.0f, color[0][2] / 255.0f );
        vec3 c1(color[1][0] / 255.0f, color[1][1] / 255.0f, color[1][2] / 255.0f);
        vec3 c2(color[2][0] / 255.0f, color[2][1] / 255.0f, color[2][2] / 255.0f);
        auto final = InterpColor(c0, c1, c2, g_u, g_v, 1 - (g_u + g_v));

        auto l = normalize(vec3(lgt.m[3])  - (o + d * min_t));
        auto n = normalize(cross(tr.pos[1] - tr.pos[0], tr.pos[2] - tr.pos[0]));
        auto v = LambertianShading(final, lgt.intensity, l, n);
        colors[(y * scr.w + x) * 3 + 0] = v[0];
        colors[(y * scr.w + x) * 3 + 1] = v[1];
        colors[(y * scr.w + x) * 3 + 2] = v[2];
    }
    return hit;
}

void RayTrace::RayShading(uint16_t x, uint16_t y)
{
    colors[(y * scr.w + x) * 3 + 0] = 255;
}