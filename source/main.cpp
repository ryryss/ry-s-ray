#include <Windows.h>
#include "display.hpp"
#include "path.h"

using namespace ry;
using namespace std;
using namespace std::chrono;

int main(int argc, char* argv[]) {
    string input; 
    if (argc < 2) { // now just sup one input
        cout << "please input a simple gltf/glb file: " << endl;
        cin >> input;
    } else {
        input = argv[1];
    }
    Scene scene;
    scene.AddModel(input);
    auto& d = Display::GetInstance();

    bool keepRender = true;
    PathRenderer renderer;
    thread t([&](){
        // while (keepRender) {
            auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            renderer.Render(&scene, d.getWindowWidth(), d.getWindowHeight(), d.GetPixels().data());
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
