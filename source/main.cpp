#include <Windows.h>
#include "display.hpp"
#include "path.h"

using namespace ry;
using namespace std;

int main(int argc, char* argv[]) {
    string input; 
    if (argc < 2) { // now just sup one input
        cout << "please input a simple gltf/glb file: " << endl;
        cin >> input;
    } else {
        input = argv[1];
    }
    auto& d = Display::GetInstance();

    Scene scene;
    scene.AddModel(input);

    int keepRender = 10;
    PathRenderer renderer;
    thread t([&](){
        while (keepRender--) {
            renderer.Render(&scene, d.getWindowWidth(), d.getWindowHeight(), d.GetPixels().data());
        }
    });
    t.detach();

    while (1) {
        d.UpdateFrame(); // present to display
        Sleep(5);
    }
    keepRender = false;
    return 0;
}
