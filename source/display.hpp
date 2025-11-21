#pragma once
#include "pch.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
namespace ry {
class Display {
public:
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    ~Display() {
        glDeleteTextures(1, &texture);
        glfwTerminate();
    }
    static Display& GetInstance(const char* title = "title") {
        static Display instance(title);
        return instance;
    }
    std::vector<vec4>& GetPixels() {
        return pixels;
    }

    void UpdateFrame() {
        ResizeTexture(width, height);

        glBindTexture(GL_TEXTURE_2D, texture);
        // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, pixels.data());
        // set x y start from the bottom left 
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
        glEnd();

        DrawConTrolWindow();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    uint16_t getWindowWidth() const { return width; }
    uint16_t getWindowHeight() const { return height; }
private:
    void DrawConTrolWindow() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Render Controls");
        // ImGui::Text("Renderer Status: %s", keepRender > 0 ? "Rendering..." : "Stopped");

        static float roughness = 0.5f;
        static float metallic = 0.1f;
        ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f);

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    Display(const char* title) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        if (mode) {
            int screenWidth = mode->width;
            int screenHeight = mode->height;
            float scaleWidth = 1.0;
            float scaleHeight = 1.0;
            glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &scaleWidth, &scaleHeight);
            width = screenWidth * scaleWidth / 2;
            height = screenHeight * scaleHeight / 2;
        } else {
            height = 1440;
            width = height * 1.6667;
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
            self->pixels.resize(w * h);
        });

        glGenTextures(1, &texture);
        ResizeTexture(width, height);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        io.FontGlobalScale = getWindowHeight() * 0.04f / 16.0f;
        // set main control window default size
        ImGui::SetNextWindowSize(ImVec2(width * 0.1f, height * 0.1f), ImGuiCond_Appearing);
    }

    void ResizeTexture(uint16_t w, uint16_t h) {
        if (old_w != width || old_h != height) {
            old_w = width;
            old_h = height;
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
            pixels.resize(w * h);
        }
    }
    std::vector<vec4> pixels;
    GLFWwindow* window;
    GLuint texture;
    uint16_t width;
    uint16_t height;

    uint16_t old_w;
    uint16_t old_h;
};
}
