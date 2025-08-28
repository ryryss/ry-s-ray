#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "loder.h"

using namespace std;
using namespace tinygltf;
using namespace ry;
using namespace glm;

bool Loader::LoadFromFile(const string& file)
{
    TinyGLTF loader;
    string err;
    string warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, file);
    if (!ret) {
        cout << "Warn: " << warn << endl;
        cerr << "Err: " << err << endl;
        cerr << "Failed to load GLB: " << file << endl;
        return false;
    }

    ParseNode();
    for (int i = 0; i < nodes.size(); i++) {
        const auto& n = model.nodes[i];
        const auto& node = nodes[i];
        if (n.mesh >= 0) {
            ParseMesh(i);
        }
        if (n.camera >= 0) {
            ParseCam(i);
        }
        if (n.light >= 0) {
            ParseLgt(i);
        }
    }
    return true;
}

void Loader::ParseCam(int num)
{
    if (model.cameras.size() > 1) {
        throw("now just sup one cam");
    }
    const auto& n = model.nodes[num];
    const auto& node = nodes[num];

    const auto& c = model.cameras[n.camera];
    cam = ry::Camera(node);
    cam.type = c.type;
    if (c.type == "perspective") {
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
}

void Loader::ParseLgt(int num)
{
    if (model.lights.size() > 1) {
        throw("now just sup one light");
    }
    const auto& n = model.nodes[num];
    const auto& node = nodes[num];

    const auto& l = model.lights[n.light];
    lgt = ry::Light(node);
    lgt.type = l.type;
    lgt.intensity = l.intensity;
    lgt.color = {l.color[0], l.color[1], l.color[2]};
    if (lgt.type == "") {
    }
}

void Loader::ParsePrimitive(const Primitive& p, const mat4& m)
{
    const auto idx = move(ParseVertIdx(p));
    std::vector<ry::Vertex> vert;
    ParsePosition(p, vert);
    ParseTexTure(p, vert);
    ParseNormal(p, vert);
    ParseVertColor(p, vert);
    // apply trans
    mat3 n_m = transpose(inverse(mat3(m)));
    for (auto i = 0; i < vert.size(); i++) {
        vert[i].pos = m * vec4(vert[i].pos, 1.0f);
        vert[i].normal = normalize(n_m * vert[i].normal);
    }
    // collect tri
    int vSize = vertices.size();
    int tSize = triangles.size();
    triangles.resize(tSize + idx.size() / 3);
    for (int i = 0; i < idx.size(); i+=3) {
        int tIdx = i / 3;
        triangles[tIdx + tSize].idx[0] = idx[i] + vSize;
        triangles[tIdx + tSize].idx[1] = idx[i + 1] + vSize;
        triangles[tIdx + tSize].idx[2] = idx[i + 2] + vSize;
    }
    vertices.insert(vertices.end(), vert.begin(), vert.end());
    cout << "parse result : vertices size = "  << vertices.size()
        << " triangles size = " << triangles.size() << endl;
}

vector<uint32_t> Loader::ParseVertIdx(const Primitive& p)
{
    vector<uint32_t> res;
    if (p.indices < 0) {
        return res;
    }
    const auto& acc = model.accessors[p.indices];
    const auto& v = model.bufferViews[acc.bufferView];
    const auto& b = model.buffers[v.buffer];
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

void Loader::ParseTexTure(const Primitive& p, vector<Vertex>& vert)
{
    // https://github.khronos.org/glTF-Tutorials/gltfTutorial/gltfTutorial_013_SimpleTexture.html
    // TODO: mult textures sup
    auto it = p.attributes.find("TEXCOORD_0");
    if (it == p.attributes.end()) {
        cout << "no texture" << endl;
        return;
    }
    const auto& acc = model.accessors[it->second];
    const auto& v = model.bufferViews[acc.bufferView];
    const auto& b = model.buffers[v.buffer];
    const unsigned char* pData = &b.data[v.byteOffset + acc.byteOffset];
    size_t stride = acc.ByteStride(v);
    for (size_t i = 0; i < acc.count; i++) {
        if (model.images.size() > 0) {
            vec2 uv(0.0f);
            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                const float* ptr = reinterpret_cast<const float*>(pData + i * stride);
                uv.x = ptr[0];
                uv.y = ptr[1];
            }
            else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                const uint8_t* ptr = reinterpret_cast<const uint8_t*>(pData + i * stride);
                uv.x = ptr[0] / 255.0f;
                uv.y = ptr[1] / 255.0f;
            }
            else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                const uint16_t* ptr = reinterpret_cast<const uint16_t*>(pData + i * stride);
                uv.x = ptr[0] / 65535.0f;
                uv.y = ptr[1] / 65535.0f;
            }
            vert[i].uv = uv;
            //direct apply uv
            const auto& image = model.images[0]; // simple get first texture
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

void Loader::ParseNormal(const Primitive& p, vector<Vertex>& vert)
{
    auto it = p.attributes.find("NORMAL");
    if (it == p.attributes.end()) {
        cout << "no normal" << endl;
        return;
    }
    const auto& acc = model.accessors[it->second];
    const auto& v = model.bufferViews[acc.bufferView];
    const auto& b = model.buffers[v.buffer];
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

void Loader::ParseVertColor(const Primitive& p, vector<Vertex>& vert)
{
    auto it = p.attributes.find("COLOR_0");
    if (it == p.attributes.end()) {
        cout << "no vert color" << endl;
        return;
    }
    const auto& acc = model.accessors[it->second];
    const auto& v = model.bufferViews[acc.bufferView];
    const auto& b = model.buffers[v.buffer];
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

void Loader::ParsePosition(const Primitive& p, vector<Vertex>& vert)
{
    auto it = p.attributes.find("POSITION");
    if (it == p.attributes.end()) {
        cout << "no vertex" << endl;
        return;
    }
    const auto& acc = model.accessors[it->second];
    const auto& v = model.bufferViews[acc.bufferView];
    const auto& b = model.buffers[v.buffer];
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

void Loader::ParseNode()
{
    if (model.scenes.size() > 1) {
        throw ("now just sup 1 cam 1 scene");
    }

    nodes.resize(model.nodes.size());
    for (const auto& s : model.scenes) {
        for (const auto& i : s.nodes) {
            const auto& n = model.nodes[i]; // root node
            roots.push_back(i);
            auto& p = nodes[i];        // parent and root
            p.name = n.name;
            p.num = i;
            p.m = GetNodeMat(i) * p.m; // root's mat is root
            ParseChildNode(i);         // apply mat to all child node
        }
    }
}

void Loader::ParseMesh(int num)
{
    const auto& node = nodes[num];
    const auto& n = model.nodes[num];
    const auto& mesh = model.meshes[n.mesh];
    const mat4& m = node.m;
    for (const auto& p : mesh.primitives) {
        ParsePrimitive(p, m);
    }
}

void Loader::ParseChildNode(int num)
{
    const auto& n = model.nodes[num];
    auto& p = nodes[num]; // parent
    p.c.resize(n.children.size());
    for (int i = 0; i < n.children.size(); i++) {
        int c_num = n.children[i];
        p.c[i] = c_num;
        auto& c = nodes[c_num]; // child node
        c.name = model.nodes[c_num].name;
        c.num = num;
        auto trans = GetNodeMat(c_num);
        c.m = p.m * trans * c.m; // apply mat
        ParseChildNode(c_num);
    }
}

 mat4 Loader::GetNodeMat(int num)
{
     mat4 t = mat4(1.0f);
     if (num < 0) {
         return t;
     }
    const auto& n = model.nodes[num];
    if (n.matrix.size() == 16) {
        t = make_mat4(n.matrix.data());
        return t;
    } else if (n.translation.empty() && n.rotation.empty() && n.scale.empty()) {
        return t;
    } else {
        mat4 T = n.translation.empty() ? glm::mat4(1.0f): 
            translate(glm::mat4(1.0f), { n.translation[0], n.translation[1], n.translation[2]});
        
        mat4 R = n.rotation.empty() ? glm::mat4(1.0f) : 
            toMat4(quat((n.rotation[3]), (n.rotation[0]), (n.rotation[1]), (n.rotation[2])));

        mat4 S = n.scale.empty() ? glm::mat4(1.0f) : 
            glm::scale(glm::mat4(1.0f), { n.scale[0], n.scale[1], n.scale[2] });

        t = T * R * S;
    }
    return t;
}
