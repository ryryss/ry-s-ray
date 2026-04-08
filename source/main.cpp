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
        target.ReSize(w, h);
    });

    Renderer renderer;
    int times = 10;
    bool over = false;
    thread t([&]() {
        while (times--) {
            renderer.Render(s, target);
        }
        over = true;
    });
    t.detach();

    while (!over) {
        d.Present(target.pixels); // present to display
        Sleep(5);
    }
    return 0;
}
