#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "model.h"
#include "bvh.h"
using namespace ry;
using namespace std;
using namespace glm;

bool Model::LoadFromFile(const string& file)
{
    gltf::TinyGLTF loader;
    string err;
    string warn;

    bool ret = loader.LoadBinaryFromFile(&raw, &err, &warn, file);
    if (!ret) {
        cout << "Warn: " << warn << endl;
        cerr << "Err: " << err << endl;
        cerr << "Failed to load GLB: " << file << endl;
        throw ("File Error");
        return false;
    }
    ParseNode();
    for (int i = 0; i < nodes.size(); i++) {
        const auto& n = raw.nodes[i];
        const auto& node = nodes[i];
        ParseMesh(i);
        ParseCamera(i);
        ParseLight(i);
    }
    ParseImage();
    // temp solution
    bvh = make_unique<BVH>(this, triangles.size());
    return true;
}

void Model::ParseNode()
{
    if (raw.scenes.size() > 1) {
        throw ("now just sup 1 cam 1 scene");
    }

    nodes.resize(raw.nodes.size());
    for (const auto& s : raw.scenes) {
        for (const auto& i : s.nodes) {
            const auto& n = raw.nodes[i]; // root node
            roots.push_back(i);
            auto& p = nodes[i];        // parent and root
            p.name = n.name;
            p.i = i;
            p.m = GetNodeMat(i) * p.m; // root's mat is root
            ParseChildNode(i);         // apply mat to all child node
        }
    }
}

void Model::ParseMesh(int num)
{
    const auto& n = raw.nodes[num];
    if (n.mesh < 0) {
        return;
    }
    const auto& node = nodes[num];
    const mat4& m = node.m;

    const auto& mesh = raw.meshes[n.mesh];
    for (const auto& p : mesh.primitives) {
        ParsePrimitive(p, m);
    }
}

void Model::ParseChildNode(int num)
{
    const auto& n = raw.nodes[num];
    auto& p = nodes[num]; // parent
    p.c.resize(n.children.size());
    for (int i = 0; i < n.children.size(); i++) {
        int c_num = n.children[i];
        p.c[i] = c_num;
        auto& c = nodes[c_num]; // child node
        c.name = raw.nodes[c_num].name;
        c.i = num;
        auto trans = GetNodeMat(c_num);
        c.m = p.m * trans * c.m; // apply mat
        ParseChildNode(c_num);
    }
}

void Model::ParseCamera(int num)
{
    const auto& n = raw.nodes[num];
    if (n.camera < 0) {
        return;
    }
    const auto& node = nodes[num];
    auto cam = Camera(node);

    const auto& c = raw.cameras[n.camera];
    cam.type = c.type;
    if (cam.type == "perspective") {
        cam.znear = c.perspective.znear;
        cam.zfar = c.perspective.zfar;
        cam.yfov = c.perspective.yfov;
        cam.aspectRatio = c.perspective.aspectRatio;
    } else if (c.type == "orthographic") {
        cam.znear = c.orthographic.znear;
        cam.zfar = c.orthographic.zfar;
        cam.xmag = c.orthographic.xmag;
        cam.ymag = c.orthographic.ymag;
    } else {
        throw("cam info error");
    }

    // use camera world coordinate to direct get base vector
    cam.e = vec3(cam.m[3]);
    cam.w = -normalize(vec3(cam.m[2]));
    cam.v = normalize(cam.m[1]);
    cam.u = normalize(cam.m[0]);
    /*
       some book like¡¶Ray Tracing in One Weekend¡·will use :
       cam.w = normalize(cam.e - vec3(0, 0, -1));
       cam.u = normalize(cross(cam.w, vec3(0, 1, 0)));
       cam.v = cross(cam.u, cam.w);
       in this program, will cause errors
    */
    cam.i = cameras.size();
    cameras.push_back(cam);
}

void Model::ParseLight(int num)
{
    const auto& n = raw.nodes[num];
    if (n.light < 0) {
        return;
    }
    const auto& node = nodes[num];
    auto light = Light(node);

    const auto& l = raw.lights[n.light];
    light.type = l.type;
    // lgt.intensity = l.intensity;
    // lgt.color = { l.color[0], l.color[1], l.color[2] };
    if (light.type == "") {} // TODO : need process point light
    light.i = lights.size();
    lights.push_back(light);
}

void Model::ParseMaterial(int num)
{
    if (num < 0) {
        return;
    }
    Material mat;
    mat.SetRawPtr(this, &raw.materials[num]);
    // In most cases, the rendering is based on triangles
    // so there is no need to record the material index for each vertex.
    materials.push_back(mat);
}

void Model::ParseEmissiveMaterial(int num, const vector<uint64_t>& ids)
{
    if (num < 0) {
        return;
    }
    // TODO emissive texture
    // get emissive info
    auto& m = raw.materials[num];
    auto& pbr = m.pbrMetallicRoughness;
    float emissiveStrength = 0.0;
    if (auto it = m.extensions.find("KHR_materials_emissive_strength");
        it != m.extensions.end()) {
        const gltf::Value& val = it->second.Get("emissiveStrength");
        emissiveStrength = static_cast<float>(val.Get<double>());
    }
    auto baseColorFactor = vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1],
        pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
    // set material info
    auto& material = materials.back();
    material.SetEmissiveInfo(baseColorFactor, emissiveStrength);
    // set emissive face as a area light
    Light light(this);
    light.emissiveStrength = emissiveStrength;
    light.I = vec3(baseColorFactor);
    light.name = "AreaLight";
    light.triangles = ids;
    for (auto i : ids) {
        auto& tri = triangles[i];
        auto& a = vertices[tri.vertIdx[0]].pos;
        auto& b = vertices[tri.vertIdx[1]].pos;
        auto& c = vertices[tri.vertIdx[2]].pos;
        vec3 e1 = b - a;
        vec3 e2 = c - a;
        light.area += 0.5f * length(cross(e1, e2));
    }
    lights.push_back(light);
}

std::vector<uint32_t> Model::ParseVertIdx(const gltf::Primitive& p)
{
    vector<uint32_t> res;
    if (p.indices < 0) {
        return res;
    }
    const auto& acc = raw.accessors[p.indices];
    const auto& v = raw.bufferViews[acc.bufferView];
    const auto& b = raw.buffers[v.buffer];
    const void* data = &b.data[v.byteOffset + acc.byteOffset];
    res.resize(acc.count);

    switch (acc.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
        const uint16_t* buf = reinterpret_cast<const uint16_t*>(data);
        for (size_t i = 0; i < acc.count; ++i)
            res[i] = static_cast<unsigned int>(buf[i]);
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
        const uint32_t* buf = reinterpret_cast<const uint32_t*>(data);
        for (size_t i = 0; i < acc.count; ++i)
            res[i] = buf[i];
        break;
    }
    default:
        cerr << "Unsupported index component type." << endl;
    }
    return res;
}

void Model::ParsePrimitive(const gltf::Primitive& p, const mat4& m)
{
    const auto idx = move(ParseVertIdx(p));
    std::vector<Vertex> vert;
    ParsePosition(p, vert);
    ParseTexTureCoord(p, vert);
    ParseNormal(p, vert);
    ParseVertColor(p, vert);
    ParseMaterial(p.material);

    int vSize = vertices.size();
    // apply trans
    mat3 n_m = transpose(inverse(mat3(m)));
    for (auto i = 0; i < vert.size(); i++) {
        vert[i].pos = m * vec4(vert[i].pos, 1.0f);
        vert[i].normal = normalize(n_m * vert[i].normal);
#ifdef DEBUG
        // cout << "add vert pos = ";
        // PrintVec(vert[i].pos);
#endif
    }
    vertices.insert(vertices.end(), vert.begin(), vert.end());
    
    bool emissive = IsEmissive(p.material);
    vector<uint64_t> emissiveTris;
    // collect tris
    for (int i = 0; i < idx.size(); i += 3) {
        triangles.push_back(Triangle());
        auto& tri = triangles.back();
        tri.vertIdx[0] = idx[i + 0] + vSize;
        tri.vertIdx[1] = idx[i + 1] + vSize;
        tri.vertIdx[2] = idx[i + 2] + vSize;
        tri.material = p.material;
        auto& a = vertices[tri.vertIdx[0]].pos;
        auto& b = vertices[tri.vertIdx[1]].pos;
        auto& c = vertices[tri.vertIdx[2]].pos;
        tri.c = (a + b + c) / 3.0f;
        // tri.normal = normalize(cross(b - a, c - a));

        if (emissive) {
            emissiveTris.push_back(triangles.size() - 1);
        }
    }
    cout << "parse result : vertices size = " << vert.size()
         << " triangles size = " << idx.size() / 3 << endl;

    if (emissive) {
        ParseEmissiveMaterial(p.material, emissiveTris);
    }
}

void Model::ParseImage()
{
    for (auto i : raw.images) {
        images.push_back(Image());
        auto& image = images.back();
        int levels = 1 + (int)floor(std::log2(std::max(i.width, i.height)));
        image.mm.reserve(levels);
        image.mm.push_back(MipMap());

        auto& mipmap = image.mm.back();
        mipmap.width = i.width;
        mipmap.height = i.height;
        mipmap.pixels.resize(i.width * i.height * 4);
        for (int j = 0; j < mipmap.pixels.size(); j++) {
            mipmap.pixels[j] = i.image[j] / 255;
        }

        auto& mipmaps = image.mm;
        for (int j = 1; j < levels; j++) {
            const MipMap& prev = mipmaps[j - 1];
            int w = std::max(1, prev.width / 2);
            int h = std::max(1, prev.height / 2);

            mipmaps.push_back(MipMap());
            auto& level = mipmaps.back();
            level.width = w;
            level.height = h;
            level.pixels.resize(w * h * 4); // RGBA8

            // downsample
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    int r = 0, g = 0, b = 0, a = 0;
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            int srcX = std::min(prev.width - 1, x * 2 + dx);
                            int srcY = std::min(prev.height - 1, y * 2 + dy);
                            int idx = (srcY * prev.width + srcX) * 4;
                            r += prev.pixels[idx + 0];
                            g += prev.pixels[idx + 1];
                            b += prev.pixels[idx + 2];
                            a += prev.pixels[idx + 3];
                        }
                    }
                    int dstIdx = (y * w + x) * 4;
                    level.pixels[dstIdx + 0] = r / 4;
                    level.pixels[dstIdx + 1] = g / 4;
                    level.pixels[dstIdx + 2] = b / 4;
                    level.pixels[dstIdx + 3] = a / 4;
                }
            }
        }
    }
}

void Model::ParseTexTureCoord(const gltf::Primitive& p, std::vector<Vertex>& vert)
{
    // https://github.khronos.org/glTF-Tutorials/gltfTutorial/gltfTutorial_013_SimpleTexture.html
    // TODO: mult texturescoord sup
    auto it = p.attributes.find("TEXCOORD_0");
    if (it == p.attributes.end()) {
        cout << "no texture" << endl;
        return;
    }
    const auto& acc = raw.accessors[it->second];
    const auto& v = raw.bufferViews[acc.bufferView];
    const auto& b = raw.buffers[v.buffer];
    const unsigned char* pData = &b.data[v.byteOffset + acc.byteOffset];
    size_t stride = acc.ByteStride(v);
    for (size_t i = 0; i < acc.count; i++) {
        if (raw.images.size() > 0) {
            vec2 uv(0.0f);
            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                const float* ptr = reinterpret_cast<const float*>(pData + i * stride);
                uv.x = ptr[0];
                uv.y = ptr[1];
            } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* ptr = reinterpret_cast<const uint8_t*>(pData + i * stride);
                uv.x = ptr[0] / 255.0f;
                uv.y = ptr[1] / 255.0f;
            } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* ptr = reinterpret_cast<const uint16_t*>(pData + i * stride);
                uv.x = ptr[0] / 65535.0f;
                uv.y = ptr[1] / 65535.0f;
            }
            vert[i].uv = uv;
            //direct apply uv
            const auto& image = raw.images[0]; // simple get first texture
            int w = image.width;
            const unsigned char* pixels = image.image.data();
            float u = vert[i].uv[0];
            float v = vert[i].uv[1];
            int x = int(u * (w - 1));
            int y = int(v * (image.height - 1)); // no need reverse
            // int((1.0f - v) * (image.height - 1));
            int idx = (y * w + x) * image.component;

            vert[i].color.r = pixels[idx + 0] / 255.0f;
            vert[i].color.g = pixels[idx + 1] / 255.0f;
            vert[i].color.b = pixels[idx + 2] / 255.0f;
            vert[i].color.a = (image.component == 4) ? pixels[idx + 3] / 255.0f : 1.0f;
        }
    }
}

void Model::ParseNormal(const gltf::Primitive& p, std::vector<Vertex>& vert)
{
    auto it = p.attributes.find("NORMAL");
    if (it == p.attributes.end()) {
        cout << "no normal" << endl;
        return;
    }
    const auto& acc = raw.accessors[it->second];
    const auto& v = raw.bufferViews[acc.bufferView];
    const auto& b = raw.buffers[v.buffer];
    const unsigned char* pData = &b.data[v.byteOffset + acc.byteOffset];
    size_t stride = acc.ByteStride(v);
    for (size_t i = 0; i < acc.count; i++) {
        vec3 n(0.0f);
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            const float* ptr = reinterpret_cast<const float*>(pData + i * stride);
            n.x = ptr[0];
            n.y = ptr[1];
            n.z = ptr[2];
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_BYTE) {
            const int8_t* ptr = reinterpret_cast<const int8_t*>(pData + i * stride);
            n.x = std::max(ptr[0] / 127.0f, -1.0f);
            n.y = std::max(ptr[1] / 127.0f, -1.0f);
            n.z = std::max(ptr[2] / 127.0f, -1.0f);
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
            const int16_t* ptr = reinterpret_cast<const int16_t*>(pData + i * stride);
            n.x = std::max(ptr[0] / 32767.0f, -1.0f);
            n.y = std::max(ptr[1] / 32767.0f, -1.0f);
            n.z = std::max(ptr[2] / 32767.0f, -1.0f);
        }
        vert[i].normal = normalize(n);
    }
}

void Model::ParseVertColor(const gltf::Primitive& p, std::vector<Vertex>& vert)
{
    auto it = p.attributes.find("COLOR_0");
    if (it == p.attributes.end()) {
        cout << "no vert color" << endl;
        return;
    }
    const auto& acc = raw.accessors[it->second];
    const auto& v = raw.bufferViews[acc.bufferView];
    const auto& b = raw.buffers[v.buffer];
    const unsigned char* pData = &b.data[v.byteOffset + acc.byteOffset];
    size_t stride = acc.ByteStride(v);
    for (size_t i = 0; i < acc.count; i++) {
        vec4 c(1.0f);
        if (acc.type == TINYGLTF_TYPE_VEC3) {
            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                const float* ptr = reinterpret_cast<const float*>(pData + i * stride);
                c.r = ptr[0]; c.g = ptr[1]; c.b = ptr[2];
                c.a = 1.0f;
            } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* ptr = reinterpret_cast<const uint8_t*>(pData + i * stride);
                c.r = ptr[0] / 255.0f;
                c.g = ptr[1] / 255.0f;
                c.b = ptr[2] / 255.0f;
                c.a = 1.0f;
            } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* ptr = reinterpret_cast<const uint16_t*>(pData + i * stride);
                c.r = ptr[0] / 65535.0f;
                c.g = ptr[1] / 65535.0f;
                c.b = ptr[2] / 65535.0f;
                c.a = 1.0f;
            }
        } else if (acc.type == TINYGLTF_TYPE_VEC4) {
            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                const float* ptr = reinterpret_cast<const float*>(pData + i * stride);
                c.r = ptr[0]; c.g = ptr[1]; c.b = ptr[2]; c.a = ptr[3];
            } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* ptr = reinterpret_cast<const uint8_t*>(pData + i * stride);
                c.r = ptr[0] / 255.0f;
                c.g = ptr[1] / 255.0f;
                c.b = ptr[2] / 255.0f;
                c.a = ptr[3] / 255.0f;
            } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* ptr = reinterpret_cast<const uint16_t*>(pData + i * stride);
                c.r = ptr[0] / 65535.0f;
                c.g = ptr[1] / 65535.0f;
                c.b = ptr[2] / 65535.0f;
                c.a = ptr[3] / 65535.0f;
            }
        }
        vert[i].color *= c; // if have texture use "*" simple process vert color;
    }
}

void Model::ParsePosition(const gltf::Primitive& p, std::vector<Vertex>& vert)
{
    auto it = p.attributes.find("POSITION");
    if (it == p.attributes.end()) {
        cout << "no vertex" << endl;
        return;
    }
    const auto& acc = raw.accessors[it->second];
    const auto& v = raw.bufferViews[acc.bufferView];
    const auto& b = raw.buffers[v.buffer];
    const unsigned char* pData = &b.data[v.byteOffset + acc.byteOffset];
    size_t stride = acc.ByteStride(v);
    vert.resize(acc.count);
    for (size_t i = 0; i < acc.count; i++) {
        glm::vec3 pos(0.0f);
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            const float* ptr = reinterpret_cast<const float*>(pData + i * stride);
            pos.x = ptr[0];
            pos.y = ptr[1];
            pos.z = ptr[2];
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
            const int16_t* ptr = reinterpret_cast<const int16_t*>(pData + i * stride);
            pos.x = ptr[0] / 32767.0f;
            pos.y = ptr[1] / 32767.0f;
            pos.z = ptr[2] / 32767.0f;
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* ptr = reinterpret_cast<const uint16_t*>(pData + i * stride);
            pos.x = ptr[0] / 65535.0f;
            pos.y = ptr[1] / 65535.0f;
            pos.z = ptr[2] / 65535.0f;
        }
        vert[i].pos = pos;
    }
}

mat4 Model::GetNodeMat(int num)
{
    mat4 t = mat4(1.0f);
    if (num < 0) {
        return t;
    }
    const auto& n = raw.nodes[num];
    if (n.matrix.size() == 16) {
        t = make_mat4(n.matrix.data());
        return t;
    } else if (n.translation.empty() && n.rotation.empty() && n.scale.empty()) {
        return t;
    } else {
        mat4 T = n.translation.empty() ? glm::mat4(1.0f) :
            translate(glm::mat4(1.0f), { n.translation[0], n.translation[1], n.translation[2] });

        mat4 R = n.rotation.empty() ? glm::mat4(1.0f) :
            toMat4(quat((n.rotation[3]), (n.rotation[0]), (n.rotation[1]), (n.rotation[2])));

        mat4 S = n.scale.empty() ? glm::mat4(1.0f) :
            glm::scale(glm::mat4(1.0f), { n.scale[0], n.scale[1], n.scale[2] });

        t = T * R * S;
    }
    return t;
}

bool Model::Intersect(const Ray& r, const vector<uint64_t>& idx, Interaction& isect) const
{
    bool hit = false;
    float t, gu, gv;
    // for (int i = 0; i < triangles.size(); i++) {
    for (auto& i : idx) {
        auto& tri = triangles[i];
        auto& a = vertices[tri.vertIdx[0]].pos;
        auto& b = vertices[tri.vertIdx[1]].pos;
        auto& c = vertices[tri.vertIdx[2]].pos;
        if (Moller_Trumbore(r.o, r.d, a, b, c, t, gu, gv) &&
            t > isect.tMin && t < isect.tMax) {
            isect.tMax = t;
            isect.bary = { 1 - gu - gv, gu, gv };
            isect.p = r.o + t * r.d;
            isect.tri = &tri;
            hit = true;
        }
    }
    if (hit) {
        auto& a = vertices[isect.tri->vertIdx[0]];
        auto& b = vertices[isect.tri->vertIdx[1]];
        auto& c = vertices[isect.tri->vertIdx[2]];
        isect.normal = normalize(isect.bary[0] * a.normal
            + isect.bary[1] * b.normal + isect.bary[2] * c.normal);

        isect.mat = &materials[isect.tri->material];
        isect.bsdf = isect.mat->CreateBSDF(&isect);
    }
    return hit;
}

bool Model::Intersect(const Ray& r, Interaction& isect) const
{
    vector<uint64_t> idx;
    bvh->TraverseBVH(idx, r, bvh->root);
    return Intersect(r, idx, isect);
}