#include "tracer.h"
#include "algorithm.h"
#include "task.h"

using namespace std;
using namespace ry;
using namespace glm;
using namespace alg;

void Tracer::ProcessCamera()
{
    auto cam = model->GetCam();
    up = normalize(vec3(cam.m[1])); // look up
    f = normalize(vec3(cam.m[2]));  // look forward
    l = vec3(cam.m[3]);

    w = normalize((l - f));
    u = normalize(cross(up, f));
    v = cross(f, u);

    // use cam aspect ratio
    if (cam.type == "perspective") {
        cam.ymag = cam.znear * tan(cam.yfov / 2);
        cam.xmag = cam.ymag * cam.aspectRatio;
    }
    else {
        cam.xmag = cam.ymag * scr.w / scr.h;
    }
}

void Tracer::Excute(const Screen& s)
{
    if (s.h <= 0 && s.w <= 0) {
        // throw ("");
    } else {
        scr = s;
        pixels.clear();
        pixels.resize(s.h * s.w);
        ProcessCamera();
    }
    Calculate();
}

void Tracer::Calculate()
{
    auto& t = Task::GetInstance();
    auto w_cnt = t.WokerCnt();
    for (auto i = 0; i < w_cnt; i ++) {
        t.Add([this, i, &t, h = scr.h / w_cnt]() {
            // use the number of pixels on the y-axis to parallel cal
            for (uint16_t y = i * h; y < (i + 1) * h; y++) {
                for (uint16_t x = 0; x < scr.w; x++) {
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
std::pair<ry::vec3, ry::vec3> Tracer::RayGeneration(uint32_t x, uint32_t y)
{
    vec3 o, d;
    auto cam = model->GetCam();
    float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
    if (cam.type == "perspective") {
        o = l;
        d = -cam.znear * f + u * uu + v * vv;
    }
    else {
        o = l + u * uu + v * vv;
        d = w;
    }
    return { o, d };
}

vec3 InterpColor(const vec3& c0, const vec3& c1, const vec3& c2,
    float b0, float b1, float b2) {
    vec3 c;
    c[0] = b0 * c0[0] + b1 * c1[0] + b2 * c2[0];
    c[0] = b0 * c0[1] + b1 * c1[1] + b2 * c2[1];
    c[2] = b0 * c0[2] + b1 * c1[2] + b2 * c2[2];

    return c;
}

bool Tracer::RayCompute(uint32_t x, uint32_t y)
{
    auto [o, d] = RayGeneration(x, y);
    auto cam = model->GetCam();

    bool hit = false;
    int tri = -1;
    float t, g_u, g_v;
    float min_t = numeric_limits<float>::max();

    const auto pos = model->GetPosition();
    const auto idx = model->GetIndex();
    for (int i = 0; i < pos.size(); i+=3) {
        const vec3& a = pos[idx[i]]; // 3 vert of tri
        const vec3& b = pos[idx[i + 1]];
        const vec3& c = pos[idx[i + 2]];
        if(Moller_Trumbore(o, d, a, b, c, t, g_u, g_v)) {
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
    /*if (hit) {
        auto tr = (*input)[tri];
        auto color = tr.color;// use g to cal color
        vec3 c0(color[0][0] / 255.0f, color[0][1] / 255.0f, color[0][2] / 255.0f );
        vec3 c1(color[1][0] / 255.0f, color[1][1] / 255.0f, color[1][2] / 255.0f);
        vec3 c2(color[2][0] / 255.0f, color[2][1] / 255.0f, color[2][2] / 255.0f);
        auto final = InterpColor(c0, c1, c2, g_u, g_v, 1 - (g_u + g_v));

        auto l = normalize(vec3(lgt.m[3])  - (o + d * min_t));
        auto n = normalize(cross(tr.pos[1] - tr.pos[0], tr.pos[2] - tr.pos[0]));
        auto v = LambertianShading(final, lgt.intensity, l, n);
        pixels[(y * scr.w + x) * 3 + 0] = v[0];
        colors[(y * scr.w + x) * 3 + 1] = v[1];
        colors[(y * scr.w + x) * 3 + 2] = v[2];
    }*/
    return hit;
}

void Tracer::RayShading(uint16_t x, uint16_t y)
{
    // colors[(y * scr.w + x) * 3 + 0] = 255;
}