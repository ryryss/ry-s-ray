#include <Windows.h>
#include "pub.h"
#include "display.h"
#include "loder.h"
#include "tracer.h"

using namespace std;
using namespace ry;
using namespace std::chrono;

int main(int argc, char* argv[]) {
    string input;
    if (argc < 2) {
        cout << "please input a simple gltf/glb file: " << endl;
        cin >> input;
    } else {
        input = argv[1];
    }
    Loader model;
    if (!model.LoadFromFile(input)) {
        throw("can not open glb/gltf.");
    }
    auto& d = Display::GetInstance();

    Tracer r;
    r.SetModel(&model);
    auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    r.Excute({ d.getWindowWidth(), d.getWindowHeight() });
    cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - now << endl;
    while (1) {
        d.UpdateFrame(r.GetResult().data());
    }
    return 0;
}
