#include "ray_trace.h"
#include "task.h"

using namespace std;
using namespace ry;
using namespace glm;

// Individual algorithm functions
// TODO: will move to other file
float Moller_Trumbore(const vec3& o, const vec3& d, const vec3& a, const vec3& b, const vec3& c)
{
    // tri base vec
    vec3 e1 = b - a;
    vec3 e2 = c - a;

    vec3 s1 = cross(d, e2);
    float det = dot(e1, s1);
    if (fabs(det) <= 1e-8) {
        return numeric_limits<float>::max();
    }
    
    vec3 s = o - a;
    float ted = 1 / det;
    float u = dot(s, s1) * ted;
    if (u < 0.0f || u > 1.0f) { // use center of gravity coordinates to judge
        return numeric_limits<float>::max();
    }

    vec3 s2 = cross(s, e1);
    float v = dot(d, s2) * ted;
    if (v < 0.0f || v + u > 1.0f) {
        return numeric_limits<float>::max();
    }

    return dot(e2, s2) * ted;
}

void RayTrace::SetCamera(const ry::Camera& c)
{
    cam = c;
    /*up = normalize(vec3(c.m[1]));
    f = normalize(vec3(c.m[2]));
    l = vec3(c.m[3]);*/
    up = c.m * vec4(0.0, 1.0, 0.0, 1.0);
    f = c.m * vec4(0.0, 0.0, 1.0, 1.0);
    l = c.m * vec4(0.0, 0.0, 0.0, 1.0);

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
    while(!t.Free());
    auto w_cnt = t.WokerCnt();
    for (auto i = 0; i < w_cnt; i ++) {
        t.Add([this, i, &t, h = scr.h / w_cnt]() {
            // use the number of pixels on the y-axis to parallel cal
            for (uint16_t y = i * h; y < (i + 1) * h; y++) {
                for (uint16_t x = 0; x < scr.w; x++) {
                    float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
                    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
                    if (RayCompute(uu, vv)) {
                        RayShading(x, y);
                    }
                }
            }
        });
    }
    t.Excute();

    if (auto mod = scr.h % w_cnt; mod) {
        for (uint16_t y = scr.h / w_cnt * w_cnt; y < scr.h; y++) {
            for (uint16_t x = 0; x < scr.w; x++) {
                float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
                float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
                if (RayCompute(uu, vv)) {
                    RayShading(x, y);
                }
            }
        }
    }

    // below is the serial version
    // l = -xmag  r = xmag b = -ymag t = ymag
    // u = l + (r − l)(i + 0.5)/nx
    /*for (uint16_t y = 0; y < scr.h; y++) {
        for (uint16_t x = 0; x < scr.w; x++) {
            float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
            float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
            auto o = l + u * uu + v * vv;
            if (RayCompute(o)) {
                RayShading(x,y);
            }
        }
    }*/
}

bool RayTrace::RayCompute(const float& uu, const float& vv)
{
    vec3 o;
    vec3 d;
    if (cam.type == "perspective") {
        o = l;
        d = -cam.znear * f + u * uu + v * vv;
    } else {
        o = l + u * uu + v * vv;
        d = w;
    }
    bool hit = false;
    float min_t = numeric_limits<float>::max();
    for (int i = 0; i < input->size(); i ++) {
        auto t = Moller_Trumbore(o, d, (*input)[i].pos[0], (*input)[i].pos[1], (*input)[i].pos[2]);
        if (cam.type == "perspective" || t > cam.znear && t < cam.zfar) {
            // now just orthographic can do depth test 
            if (t < min_t) {
                min_t = t;
                hit = true;
            }
        }
    }
    return hit;
}

void RayTrace::RayShading(uint16_t x, uint16_t y)
{
    colors[(y * scr.w + x) * 3 + 0] = 255;
}