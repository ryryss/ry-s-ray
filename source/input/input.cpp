#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "input.h"
#include "math.hpp"
using namespace ry;
using namespace std;
namespace gltf = tinygltf;

inline string GetExtension(const string& filename) {
    return filesystem::path(filename).extension().string();
}

unique_ptr<IModelLoader> Input::CreateLoader(const string& filename)
{
    auto ext = GetExtension(filename);
    if (ext == ".gltf" || ext == ".glb") {
        return std::make_unique<GLTFLoader>();
    }

    // if (.obj) return OBJLoader
    // if (.fbx) return FBXLoader

    throw std::runtime_error("Unsupported format");
}

Model GLTFLoader::Load(const string& filename)
{
    gltf::TinyGLTF loader;
    string err;
    string warn;

    bool ret = loader.LoadBinaryFromFile(&raw, &err, &warn, filename);
    if (!ret) {
        cout << "Warn: " << warn << endl;
        cerr << "Err: " << err << endl;
        cerr << "Failed to load GLB: " << filename << endl;
        throw ("File Error");
    }


    ParseNode();
    for (int i = 0; i < nodes.size(); i++) {
        const auto& n = raw.nodes[i];
        const auto& node = nodes[i];
        ParseMesh(i);
        ParseCamera(i);
        // ParseLight(i);
    }
    ParseImage();

    return BuildModel();
}

Model GLTFLoader::BuildModel()
{
    assert(vertIdx.size() % 3 == 0);

    Model m;
    // vert
    auto tris = m.GetTriangles();
    for (int i = 0; i < vertices.size() / 3; i++) {
        const auto& v0 = vertices[vertIdx[i]];
        const auto& v1 = vertices[vertIdx[i + 1]];
        const auto& v2 = vertices[vertIdx[i + 2]];
        tris.push_back(BuildTriangle(v0, v1, v2));
    }

    // camera
    for (const auto& camNode : cams) {
        if (camNode.c != nullptr) {
            const auto c = camNode.c;
            Camera cam;
            if (c->type == "perspective") {
                cam.znear = c->perspective.znear;
                cam.zfar = c->perspective.zfar;
                cam.yfov = c->perspective.yfov;
                cam.aspectRatio = c->perspective.aspectRatio;
                cam.type = 0;
            } else if (c->type == "orthographic") {
                cam.znear = c->orthographic.znear;
                cam.zfar = c->orthographic.zfar;
                cam.xmag = c->orthographic.xmag;
                cam.ymag = c->orthographic.ymag;
                cam.type = 1;
            } else {
                throw("cam info error");
            }

            // use camera world coordinate to direct get base vector
            cam.e = vec3(camNode.m[3]);
            cam.w = -normalize(vec3(camNode.m[2]));
            cam.v = normalize(camNode.m[1]);
            cam.u = normalize(camNode.m[0]);
            /*
               some book like¡¶Ray Tracing in One Weekend¡·will use :
               cam.w = normalize(cam.e - vec3(0, 0, -1));
               cam.u = normalize(cross(cam.w, vec3(0, 1, 0)));
               cam.v = cross(cam.u, cam.w);
               in this program, will cause errors
            */
            cam.m = camNode.m;
            m.AddCamera(cam);
        }
    }
    
    // light
    // now just support area light
    for (const auto& idxs : emissiveVertIdx) {
        Light light;
        for (int i = 0; i < idxs.size() / 3; i++) {
            const auto& v0 = vertices[idxs[i]];
            const auto& v1 = vertices[idxs[i + 1]];
            const auto& v2 = vertices[idxs[i + 2]];
            light.tIdxs.push_back(tris.size());
            tris.push_back(BuildTriangle(v0, v1, v2));
        }
        auto& m = raw.materials[vertices[idxs[0]].material]; // all vertex materials from idx are the same
        auto& pbr = m.pbrMetallicRoughness;
        float emissiveStrength = 0.0;
        if (auto it = m.extensions.find("KHR_materials_emissive_strength");
            it != m.extensions.end()) {
            const gltf::Value& val = it->second.Get("emissiveStrength");
            emissiveStrength = static_cast<float>(val.Get<double>());
        }
        auto baseColorFactor = vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1],
            pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
        light.emissiveStrength = emissiveStrength;
        light.I = vec3(baseColorFactor);

        // set arae
        for (const auto& idx : light.tIdxs) {
            auto& tri = tris[idx];
            vec3 e1 = tri.v1 - tri.v0;
            vec3 e2 = tri.v2 - tri.v0;
            light.area += 0.5f * length(cross(e1, e2));
        }
    }

    //metrial
    
    // image / texture
    return m;
}

Triangle ry::GLTFLoader::BuildTriangle(const VertexInfo& v0, const VertexInfo& v1, const VertexInfo& v2)
{
    return {
        v0.pos, v1.pos, v2.pos,
        v0.normal, v1.normal, v2.normal,
        v0.uv, v1.uv, v2.uv,
        v0.material // material + / 3 ?
    };
}

mat4 GLTFLoader::GetNodeMat(int num)
{
    mat4 t = mat4(1.0f);
    if (num < 0) {
        return t;
    }
    const auto& n = raw.nodes[num];
    if (n.matrix.size() == 16) {
        t = mat4(n.matrix.data());
        return t;
    } else if (n.translation.empty() && n.rotation.empty() && n.scale.empty()) {
        return t;
    } else {
        mat4 T = n.translation.empty() ? mat4(1.0f) :
            translate(mat4(1.0f), vec3(n.translation[0], n.translation[1], n.translation[2]));

        mat4 R = n.rotation.empty() ? mat4(1.0f) :
            toMat4(quat((n.rotation[3]), (n.rotation[0]), (n.rotation[1]), (n.rotation[2])));

        mat4 S = n.scale.empty() ? mat4(1.0f) :
            scale(mat4(1.0f), vec3(n.scale[0], n.scale[1], n.scale[2]));

        t = T * R * S;
    }
    return t;
}

void GLTFLoader::ParseNode()
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

void GLTFLoader::ParseImage()
{
}

void GLTFLoader::ParseMesh(int num)
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

void GLTFLoader::ParseChildNode(int num)
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

void GLTFLoader::ParseCamera(int num)
{
    const auto& n = raw.nodes[num];
    if (n.camera < 0) {
        return;
    }
    const auto& node = nodes[num];
    auto cam = CameraNode(node);
    cam.c = &raw.cameras[n.camera];
    cams.push_back(cam);
}

void GLTFLoader::ParseLight(int num)
{
    /*const auto& n = raw.nodes[num];
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
    lights.push_back(light);*/
}

void GLTFLoader::ParseMaterial(int num)
{

}

void GLTFLoader::ParseEmissiveMaterial(int num, const std::vector<uint64_t>& ids)
{
}

vector<uint32_t> GLTFLoader::ParseVertIdx(const gltf::Primitive& p)
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

void GLTFLoader::ParsePrimitive(const gltf::Primitive& p, const mat4& m)
{
    auto idx = ParseVertIdx(p);
    vector<VertexInfo> vert;
    ParsePosition(p, vert);
    ParseTexTureCoord(p, vert);
    ParseNormal(p, vert);
    // ParseVertColor(p, vert);
    ParseMaterial(p.material);

    // apply trans
    mat3 nm = transpose(inverse(mat3(m)));
    for (auto i = 0; i < vert.size(); i++) {
        vert[i].pos = m * vec4(vert[i].pos, 1.0f);
        vert[i].normal = normalize(nm * vert[i].normal);
#ifdef DEBUG
        // cout << "add vert pos = ";
        // PrintVec(vert[i].pos);
#endif
    }
    int vertCnt = vertices.size();
    vertices.insert(vertices.end(), vert.begin(), vert.end());

    std::transform(idx.begin(), idx.end(), idx.begin(), [vertCnt](int x) { return x + vertCnt; });
    // for area light
    if (IsEmissive(p.material)) {
        emissiveVertIdx.push_back(idx);
    } else {
        vertIdx.insert(vertIdx.end(), idx.begin(), idx.end());
    }

    cout << "parse result : vertices size = " << vert.size()
        << " triangles size = " << idx.size() / 3 << endl;
}

void GLTFLoader::ParseTexTureCoord(const gltf::Primitive& p, std::vector<VertexInfo>& vert)
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
            float u = vert[i].uv.x;
            float v = vert[i].uv.y;
            int x = int(u * (w - 1));
            int y = int(v * (image.height - 1)); // no need reverse
            // int((1.0f - v) * (image.height - 1));
            int idx = (y * w + x) * image.component;

            vert[i].color.x = pixels[idx + 0] / 255.0f;
            vert[i].color.y = pixels[idx + 1] / 255.0f;
            vert[i].color.z = pixels[idx + 2] / 255.0f;
            vert[i].color.w = (image.component == 4) ? pixels[idx + 3] / 255.0f : 1.0f;
        }
    }
}

void GLTFLoader::ParseNormal(const gltf::Primitive& p, std::vector<VertexInfo>& vert)
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

/*void GLTFLoader::ParseVertColor(const gltf::Primitive& p, std::vector<VertexInfo>& vert)
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
}*/

void GLTFLoader::ParsePosition(const gltf::Primitive& p, std::vector<VertexInfo>& vert)
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
        vec3 pos(0.0f);
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
        vert[i].material = p.material;
    }
}
