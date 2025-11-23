#pragma once
#include "pch.h"
#include "scene.h"
namespace ry {
class DisplayBuffer {
public:
    DisplayBuffer() {
        active.store(0);
    }
    std::vector<vec4>& GetDisplayBuffer() {
        return pixels[active.load(std::memory_order_acquire)];
    }
    std::vector<vec4>& GetFreeBuffer() {
        return pixels[1 - active.load(std::memory_order_acquire)];
    }
    void SwapBuffer() {
        unsigned int current = active.load(std::memory_order_relaxed);
        unsigned int next = 1 - current;
        active.store(next, std::memory_order_release);
    }
    void ResizeBuffer(uint16_t w, uint16_t h) {
        size_t newSize = static_cast<size_t>(w) * h;
        pixels[0].assign(newSize, vec4(0.0f));
        pixels[1].assign(newSize, vec4(0.0f));
    }
private:
    std::atomic<unsigned int> active; // false : use pixel0 to display
    std::vector<vec4> pixels[2];
};

class Renderer {
public:
    virtual void Render(Scene* s) = 0;
    void ResizeBuffer(uint16_t w, uint16_t h) {
        windowChange = true;
        newWidth = w;
        newHeight = h;
    }
    std::vector<vec4>& Present() {
        return buffer.GetDisplayBuffer();
    }
protected:
    DisplayBuffer buffer;

    uint16_t newWidth, newHeight;
    bool windowChange = false;
    uint16_t width, height;
    Scene* scene;
};
}