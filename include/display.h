#ifndef DISPLAY
#define DISPLAY
#include <GLFW/glfw3.h>
#include "pub.h"

class Display {
public:
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    ~Display() {
        glDeleteTextures(1, &texture);
        glfwTerminate();
    }
    static Display& GetInstance(uint16_t width = 800, uint16_t height = 600, const char* title = "title") {
        static Display instance(width, height, title);
        return instance;
    }

    void UpdateFrame(const uint8_t* pixels) {
        ResizeTexture(width, height);

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    uint16_t getWindowWidth() const { return width; }
    uint16_t getWindowHeight() const { return height; }
private:
    Display(uint16_t w, uint16_t h, const char* title) : width(w), height(h) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        window = glfwCreateWindow(width, height, title, NULL, NULL);
        glViewport(0, 0, width, height);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }
        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int w, int h) {
            auto* self = static_cast<Display*>(glfwGetWindowUserPointer(win));
            self->width = w;
            self->height = h;
        });

        glGenTextures(1, &texture);
        ResizeTexture(w, h);
    }

    void ResizeTexture(uint16_t w, uint16_t h) {
        if (old_w != width || old_h != height) {
            old_w = width;
            old_h = height;
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
    }

    GLFWwindow* window;
    GLuint texture;
    uint16_t width;
    uint16_t height;

    uint16_t old_w;
    uint16_t old_h;
};
#endif // !DISPLAY
