#include "tracer.h"
#include "algorithm.h"
#include "task.h"

using namespace std;
using namespace ry;
using namespace glm;
using namespace alg;

void Tracer::ProcessCamera()
{
    auto& cam = model->GetCam();
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
    } else {
        cam.xmag = cam.ymag * scr.w / scr.h;
    }
}

void Tracer::Excute(const Screen& s)
{
    if (s.h <= 0 && s.w <= 0) {
        // throw ("");
    }
    else if (scr.h == s.h && scr.w == s.w) {
        
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
                        // RayShading(x, y);
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
                    // post processing ?
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
    auto& cam = model->GetCam();
    float uu = -cam.xmag + 2 * cam.xmag * (x + 0.5) / scr.w;
    float vv = -cam.ymag + 2 * cam.ymag * (y + 0.5) / scr.h;
    if (cam.type == "perspective") {
        o = l;
        d = -cam.znear * f + u * uu + v * vv;
    } else {
        o = l + u * uu + v * vv;
        d = w;
    }
    return { o, d };
}

bool Tracer::RayCompute(uint32_t x, uint32_t y)
{
    // 1、compute viewing ray
    auto [o, d] = RayGeneration(x, y);

    // 2、find first object hit by ray and its surface normal n
    auto& cam = model->GetCam();
    Triangle hTri; // hit this tri
    float t, gu, gv;
    float min_t = numeric_limits<float>::max();

    const auto& ts = model->GetTriangles();
    const auto& vs = model->GetVertices();
    for (auto& tri : ts) {
        const vec3& a = vs[tri.idx[0]].pos;
        const vec3& b = vs[tri.idx[1]].pos;
        const vec3& c = vs[tri.idx[2]].pos;
        if(Moller_Trumbore(o, d, a, b, c, t, gu, gv)) {
            if (cam.type == "perspective" || t > cam.znear && t < cam.zfar) {
                // now just orthographic can do depth test 
                if (t < min_t) {
                    min_t = t;
                    hTri = tri;
                    hTri.bary = {1 - gu - gv, gu, gv};
                }
            }
        }
    }
    // 3、set pixel color to value computed from hit point, light, and n
    if (min_t < numeric_limits<float>::max()) {
        RayShading(x, y, hTri, o + d * min_t);
    }
    return min_t < numeric_limits<float>::max();
}

// Ambient Shading
vec4 A = vec4(0.051, 0.051, 0.051, 1.0) * 1.0f; // ka * Ia, A = env light
void Tracer::RayShading(uint16_t x, uint16_t y, Triangle& t, const vec3& hitPoint)
{
    const auto& vs = model->GetVertices();
    auto& lgt = model->GetLgt();
    auto l = normalize(vec3(lgt.m[3]) - hitPoint);

    const auto& a = vs[t.idx[0]]; // 3 vert of tri
    const auto& b = vs[t.idx[1]];
    const auto& c = vs[t.idx[2]];
    auto n = normalize(t.bary[0] * a.normal + t.bary[1] * b.normal + t.bary[2] * c.normal);

    // t.color = t.bary[0] * a.color + t.bary[1] * b.color + t.bary[2] * c.color;
    t.color = SampleTexture(t.bary, a.uv, b.uv, c.uv);
    auto& cam = model->GetCam();
    auto v = normalize(vec3(cam.m[3]) - hitPoint);
    pixels[y * scr.w + x] = A + BlinnPhongShading(t.color, t.color, 1.0, l, n, v);
}

vec4 Tracer::SampleTexture(const vec3& bary, const vec2& uv0, const vec2& uv1, const vec2& uv2)
{
    vec2 uv = bary[0] * uv0 + bary[1] * uv1 + bary[2] * uv2;
    const auto& image = model->GetTexTureImg();
    const auto* pixel = image.image.data();
    uint32_t x = uv[0] * (image.width - 1);
    uint32_t y = uv[1] * (image.height - 1); // no need reverse
    // int((1.0f - v) * (image.height - 1));
    int idx = (y * image.width + x) * image.component;

    return { pixel[idx + 0] / 255.0f,
             pixel[idx + 1] / 255.0f,
             pixel[idx + 2] / 255.0f,
            (image.component == 4) ? pixel[idx + 3] / 255.0f : 1.0f };
}
