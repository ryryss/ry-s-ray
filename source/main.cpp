#include <Windows.h>
#include "display.hpp"
#include "input.h"
#include "renderer.h"

using namespace ry;
using namespace std;

int main(int argc, char* argv[]) {
    string file; 
    if (argc < 2) { // now just sup one input
        cout << "please input a simple gltf/glb file: " << endl;
        cin >> file;
    } else {
        file = argv[1];
    }
    auto& d = Display::GetInstance();
    
    Scene s;
    s.AddModel(Input::Load(file));
    s.BuildBVH();

    RenderTarget target(d.getWindowHeight(), d.getWindowWidth());
    // for resize window
    d.SetResizeCallback([&target](int w, int h) {
        target.height = h;
        target.width = w;
    });

    Renderer renderer;
    int keepRender = 10;
    thread t([&]() {
        while (keepRender--) {
            renderer.Render(s, target);
        }
    });
    t.detach();

    while (1) {
        d.Present(target.pixels); // present to display
        Sleep(5);
    }
    keepRender = -1;
    return 0;
}
