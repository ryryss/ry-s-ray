#include <Windows.h>
#include "pub.h"
#include "display.h"
#include "loader.h"
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
    auto& model = Loader::GetInstance();
    if (!model.LoadFromFile(input)) {
        throw("can not open glb/gltf.");
    }
    auto& d = Display::GetInstance();
    model.ProcessCamera({d.getWindowWidth(), d.getWindowHeight()});

    bool keepRender = true;
    Tracer r;
    r.SetInOutPut({ d.getWindowWidth(), d.getWindowHeight() }, &model, d.GetPixels().data());
    thread t([&r, &keepRender](){
        // while (keepRender) {
            auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            r.Excute();
            cout << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - now << endl;
        // }
    });
    t.detach();

    while (1) {
        d.UpdateFrame();
        Sleep(50);
    }
    keepRender = false;
    return 0;
}
