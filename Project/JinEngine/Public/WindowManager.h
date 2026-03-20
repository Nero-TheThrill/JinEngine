#pragma once
#include <string>
#include "vec4.hpp"
class JinEngine;
struct GLFWwindow;
struct EngineContext;

class WindowManager
{
    friend JinEngine;
    friend void framebuffer_size_callback(GLFWwindow*, int, int);
public:
    using ResizeCallback = std::function<void(int /*width*/, int /*height*/)>;

    WindowManager() :window(nullptr), windowWidth(800), windowHeight(600), backgroundColor(0.4,0.4,0.4,1){}

    [[nodiscard]] GLFWwindow* GetHandle() const { return window; }

    [[nodiscard]] int GetWidth() const { return windowWidth; }

    [[nodiscard]] int GetHeight() const { return windowHeight; }

    glm::ivec2 GetWindowSize() const { return glm::ivec2(windowWidth, windowHeight); }

    void Resize(int width, int height);

    void SetTitle(const std::string& title) const;

    void SetBackgroundColor(glm::vec4 color) { backgroundColor = color; }

    [[nodiscard]] glm::vec4 GetBackgroundColor() const { return backgroundColor; }

    void SetFullScreen(bool enable);

    [[nodiscard]] bool IsFullScreen() const { return isFullscreen; }

    void RestrictResizing(bool shouldRestrict);

    void SetCursorVisible(bool visible);

    void AddResizeCallback(const std::string& key, ResizeCallback cb);

    void RemoveResizeCallback(const std::string& key);

private:
    bool Init(int _windowWidth, int _windowHeight, JinEngine& engine);

    void SetWidth(int width) { this->windowWidth = width; }

    void SetHeight(int height) { this->windowHeight = height; }

    void SetWindowSize(glm::ivec2 size) { this->windowWidth = size.x; this->windowHeight = size.y; }

    void SwapBuffers() const;

    void ClearScreen() const;

    void PollEvents() const;

    void Free() const;

    void NotifyResize(int width, int height);

    GLFWwindow* window;
    int windowWidth;
    int windowHeight;
    glm::vec4 backgroundColor;

    bool isFullscreen = false;
    bool isCursorVisible = false;
    int windowedPosX = 100, windowedPosY = 100;
    int windowedWidth = 800, windowedHeight = 600;

    std::unordered_map<std::string, ResizeCallback> resizeCallbacks;
};
