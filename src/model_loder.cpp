#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "model_loder.h"

using namespace std;
using namespace tinygltf;
using namespace ry;
using namespace glm;

bool GLBModelLoader::LoadFromFile(const string& file)
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

void GLBModelLoader::ParseCam(int num)
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

void GLBModelLoader::ParseLgt(int num)
{
    if (model.lights.size() > 1) {
        throw("now just sup one lgt");
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

void GLBModelLoader::ParsePrimitive(const Primitive& p, const mat4& m)
{
    const auto idx = move(ParseVertIdx(p));
    const auto tex = move(ParseTexTure(p));
    const auto nor = move(ParseNormal(p));
    const auto col = move(ParseVertColor(p));

    const auto& image = model.images[0];
    const unsigned char* pixels = image.image.data();
    int w = image.width;
    int h = image.height;
    int comp = image.component;

    // get bufferView data
    const Accessor& posAccessor = model.accessors[p.attributes.find("POSITION")->second];
    const BufferView& posView = model.bufferViews[posAccessor.bufferView];
    const Buffer& posBuffer = model.buffers[posView.buffer];
    const float* posData = reinterpret_cast<const float*>(
        &posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);
    // parse vert
    vector<Vertex> vertices(posAccessor.count);
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].position[0] = posData[i * 3 + 0];
        vertices[i].position[1] = posData[i * 3 + 1];
        vertices[i].position[2] = posData[i * 3 + 2];
        /*if (colorData) {
            vertices[i].color[0] = colorData[i * 3 + 0];
            vertices[i].color[1] = colorData[i * 3 + 1];
            vertices[i].color[2] = colorData[i * 3 + 2];
        } else {
            vertices[i].color[0] = vertices[i].color[1] = vertices[i].color[2] = 1.0f;
        }*/
        if (texcoords) {
            vertices[i].texcoord[0] = texcoords[i * 2 + 0];
            vertices[i].texcoord[1] = texcoords[i * 2 + 1];

            auto u = vertices[i].texcoord[0];
            auto v = vertices[i].texcoord[1];
            int x = int(u * (w - 1));
            int y = int((1.0f - v) * (h - 1)); // 翻转V
            int idx = (y * w + x) * comp;
            vertices[i].color[0] = pixels[idx + 0];
            vertices[i].color[1] = pixels[idx + 1];
            vertices[i].color[2] = pixels[idx + 2];
        } else {
            vertices[i].texcoord[0] = vertices[i].texcoord[1] = 0.0f;
        }

        if (normal) {
            vertices[i].normal[0] = normal[i * 3 + 0];
            vertices[i].normal[1] = normal[i * 3 + 1];
            vertices[i].normal[2] = normal[i * 3 + 2];
        }
    }

    // change to tri
    mat3 n_m = transpose(inverse(mat3(m)));
    vector<Triangle> toTri(indices.size() / 3);
    for (auto i = 0; i < indices.size(); i += 3) {
        for (int j = 0; j < 3; j++) {
            toTri[i / 3].pos[0 + j] = m * vec4(vertices[indices[i + j]].position, 1.0f);
            // toTri[i / 3].color[0 + j] = vertices[indices[i + j]].color;
            toTri[i / 3].normal[0 + j] = normalize(n_m * vertices[indices[i + j]].normal);
            if (texcoords) {
                toTri[i / 3].color[j][0] = vertices[indices[i + j]].color[0];
                toTri[i / 3].color[j][1] = vertices[indices[i + j]].color[1];
                toTri[i / 3].color[j][2] = vertices[indices[i + j]].color[2];
            }
        }
    }
    tris.insert(tris.end(), toTri.begin(), toTri.end());
}

vector<uint32_t> GLBModelLoader::ParseVertIdx(const Primitive& p)
{
    vector<uint32_t> res;
    if (p.indices >= 0) {
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

vector<vec2> GLBModelLoader::ParseTexTure(const Primitive& p)
{
    vector<vec2> res;
    auto it = p.attributes.find("TEXCOORD_0");
    if (it == p.attributes.end()) {
        return res;
    }
    const auto& acc = model.accessors[it->second];
    const auto& v = model.bufferViews[acc.bufferView];
    const auto& b = model.buffers[v.buffer];
    const unsigned char* pData = &b.data[v.byteOffset + acc.byteOffset];
    res.reserve(acc.count);
    size_t stride = acc.ByteStride(v);
    for (size_t i = 0; i < acc.count; i++) {
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
        res.push_back(uv);
    }
    return res;
}

vector<vec3> GLBModelLoader::ParseNormal(const Primitive& p)
{
    vector<vec3> res;
    auto it = p.attributes.find("NORMAL");
    if (it == p.attributes.end()) {
        return res;
    }
    const auto& acc = model.accessors[it->second];
    const auto& v = model.bufferViews[acc.bufferView];
    const auto& b = model.buffers[v.buffer];
    const unsigned char* pData = &b.data[v.byteOffset + acc.byteOffset];
    size_t stride = acc.ByteStride(v);
    res.reserve(acc.count);
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
        res.push_back(normalize(n)); // 法线要归一化
    }
    return res;
}

vector<vec4> GLBModelLoader::ParseVertColor(const Primitive& p)
{
    vector<vec4> res;
    auto it = p.attributes.find("COLOR_0");
    if (it == p.attributes.end()) {
        return res;
    }
    const auto& acc = model.accessors[it->second];
    const auto& v = model.bufferViews[acc.bufferView];
    const auto& b = model.buffers[v.buffer];
    const unsigned char* pData = &b.data[v.byteOffset + acc.byteOffset];
    return res;
}

void GLBModelLoader::ParseNode()
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

void GLBModelLoader::ParseMesh(int num)
{
    const auto& node = nodes[num];
    const auto& n = model.nodes[num];
    const auto& mesh = model.meshes[n.mesh];
    const mat4& m = node.m;
    for (const auto& p : mesh.primitives) {
        ParsePrimitive(p, m);
    }
}

void GLBModelLoader::ParseChildNode(int num)
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

 mat4 GLBModelLoader::GetNodeMat(int num)
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
