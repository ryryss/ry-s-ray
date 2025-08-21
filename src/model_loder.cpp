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
    if (lgt.type == "") {
    }
}

void GLBModelLoader::ParsePrimitive(const Primitive& p, const mat4& m)
{
    // get bufferView data
    const Accessor& posAccessor = model.accessors[p.attributes.find("POSITION")->second];
    const BufferView& posView = model.bufferViews[posAccessor.bufferView];
    const Buffer& posBuffer = model.buffers[posView.buffer];

    const float* posData = reinterpret_cast<const float*>(
        &posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);

    // parse vert index
    vector<uint16_t> indices;
    if (p.indices >= 0) {
        const auto& idxAccessor = model.accessors[p.indices];
        const auto& idxView = model.bufferViews[idxAccessor.bufferView];
        const auto& idxBuffer = model.buffers[idxView.buffer];

        const void* data = &idxBuffer.data[idxView.byteOffset + idxAccessor.byteOffset];
        indices.resize(idxAccessor.count);

        switch (idxAccessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            const uint16_t* buf = reinterpret_cast<const uint16_t*>(data);
            for (size_t i = 0; i < idxAccessor.count; ++i)
                indices[i] = static_cast<unsigned int>(buf[i]);
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            const uint32_t* buf = reinterpret_cast<const uint32_t*>(data);
            for (size_t i = 0; i < idxAccessor.count; ++i)
                indices[i] = buf[i];
            break;
        }
        default:
            cerr << "Unsupported index component type." << endl;
        }
    }

    // parse color
    const float* colorData = nullptr;
    size_t colorStride = 0;
    if (auto it = p.attributes.find("COLOR_0"); it != p.attributes.end()) {
        const auto& colAccessor = model.accessors[it->second];
        const auto& colView = model.bufferViews[colAccessor.bufferView];
        const auto& colBuffer = model.buffers[colView.buffer];
        colorData = reinterpret_cast<const float*>(
            &colBuffer.data[colView.byteOffset + colAccessor.byteOffset]);
    }

    // parse vert
    vector<Vertex> vertices(posAccessor.count);
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].position[0] = posData[i * 3 + 0];
        vertices[i].position[1] = posData[i * 3 + 1];
        vertices[i].position[2] = posData[i * 3 + 2];
        if (colorData) {
            vertices[i].color[0] = colorData[i * 3 + 0];
            vertices[i].color[1] = colorData[i * 3 + 1];
            vertices[i].color[2] = colorData[i * 3 + 2];
        }
        else {
            vertices[i].color[0] = vertices[i].color[1] = vertices[i].color[2] = 1.0f;
        }
    }

    // change to tri
    vector<Triangle> toTri(indices.size() / 3);
    for (auto i = 0; i < indices.size(); i += 3) {
        for (int j = 0; j < 3; j++) {
            toTri[i / 3].pos[0 + j] = m * vec4(vertices[indices[i + j]].position, 1.0f);
            toTri[i / 3].color[0 + j] = vertices[indices[i + j]].color;
        }
    }
    tris.insert(tris.end(), toTri.begin(), toTri.end());
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
