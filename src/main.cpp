#include <Windows.h>
#include "pub.h"
#include "display.h"
#include "model_loder.h"
#include "ray_trace.h"

using namespace std;
using namespace ry;
using namespace std::chrono;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        throw("just need a glb/gltf input");
    }
    string g = argv[1];
    GLBModelLoader l;
    if (!l.LoadFromFile(g)) {
        throw("can not open glb/gltf");
    }
    auto& d = Display::GetInstance();

    RayTrace r;
    r.SetCamera(l.GetCam());
    r.SetData(&l.GetTriangles());
 
    while (1) {
        auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        r.Excute({ d.getWindowWidth(), d.getWindowHeight() });
        cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - now << endl;
        d.UpdateFrame(r.GetResult().data());
    }
    return 0;
}
