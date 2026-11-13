#include "gl.h"
#include "glfw3.h"
#include "Engine.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    JinEngine* jinEngine = static_cast<JinEngine*>(glfwGetWindowUserPointer(window));
    if (jinEngine)
    {
        jinEngine->GetEngineContext().windowManager->SetWidth(width);
        jinEngine->GetEngineContext().windowManager->SetHeight(height);
        auto* state = jinEngine->GetEngineContext().stateManager->GetCurrentState();
        if (state)
        {
            state->GetCameraManager().SetScreenSizeForAll(width, height);
        }
        jinEngine->GetEngineContext().renderManager->OnResize(width, height);
        jinEngine->GetEngineContext().windowManager->NotifyResize(width, height);

        JIN_LOG("changed: " << jinEngine->GetEngineContext().windowManager->GetWidth() << " " << jinEngine->GetEngineContext().windowManager->GetHeight());
    }
}

void WindowManager::RestrictResizing(bool shouldRestrict)
{
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, shouldRestrict? GLFW_FALSE : GLFW_TRUE);
}

void WindowManager::SetCursorVisible(bool visible)
{
    if (visible)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

bool WindowManager::Init(int _windowWidth, int _windowHeight, JinEngine& engine)
{

    if (!glfwInit())
    {
        JIN_ERR("Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    windowWidth = _windowWidth;
    windowHeight = _windowHeight;

    window = glfwCreateWindow(windowWidth, windowHeight, "JIN ENGINE", nullptr, nullptr);
    if (!window)
    {
        JIN_ERR("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        JIN_ERR("Failed to initialize GLAD");
        return false;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
        0, nullptr, GL_FALSE);
    glDebugMessageCallback(
        [](GLenum /*src*/, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* msg, const void*) {
            if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
            if (type == GL_DEBUG_TYPE_OTHER) return;
            if (severity == GL_DEBUG_SEVERITY_HIGH) { JIN_ERR(std::string("[GL] ") + msg); }
            else if (severity == GL_DEBUG_SEVERITY_MEDIUM) { JIN_WRN(std::string("[GL] ") + msg); }
            else { JIN_LOG(std::string("[GL] ") + msg); }
        }, nullptr);

    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);
    glfwSetWindowUserPointer(window, &engine);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetScrollCallback(window,
        [](GLFWwindow* w, double xoff, double yoff)
        {
            JinEngine* jinEngine = static_cast<JinEngine*>(glfwGetWindowUserPointer(w));
            if (jinEngine)
                jinEngine->GetEngineContext().inputManager->AddScroll(xoff, yoff);
        });
    glfwSetKeyCallback(window,
        [](GLFWwindow* w, int key, int sc, int action, int mods)
        {
            if (auto* jinEngine = static_cast<JinEngine*>(glfwGetWindowUserPointer(w)))
                jinEngine->GetEngineContext().inputManager->OnKey(key, sc, action, mods);
        });

    glfwSetMouseButtonCallback(window,
        [](GLFWwindow* w, int button, int action, int mods)
        {
            if (auto* jinEngine = static_cast<JinEngine*>(glfwGetWindowUserPointer(w)))
                jinEngine->GetEngineContext().inputManager->OnMouseButton(button, action, mods);
        });

    glfwSetWindowPosCallback(window,
        [](GLFWwindow* w, int, int)
        {
            //if (auto* jinEngine = (JinEngine*)glfwGetWindowUserPointer(w))
            //    jinEngine->GetEngineContext().inputManager->Reset();
        });

    glfwSetWindowCloseCallback(window, [](GLFWwindow* w) {
        if (auto* e = static_cast<JinEngine*>(glfwGetWindowUserPointer(w)))
            e->RequestQuit();
        });

    return true;
}

void WindowManager::Resize(int width, int height)
{
    if (window)
    {
        glfwSetWindowSize(window, width, height);
    }
}

void WindowManager::SetTitle(const std::string& title) const
{
    glfwSetWindowTitle(window, title.c_str());
}

void WindowManager::SetFullScreen(bool enable)
{
    if (!window || isFullscreen == enable)
        return;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    if (enable)
    {
        glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
        glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
    }

    isFullscreen = enable;
}
void WindowManager::Free() const
{
    if (window)
    {
        glfwSetFramebufferSizeCallback(window, nullptr);
        glfwSetScrollCallback(window, nullptr);
        glfwSetKeyCallback(window, nullptr);
        glfwSetMouseButtonCallback(window, nullptr);
        glfwSetWindowUserPointer(window, nullptr);
        glfwSetWindowCloseCallback(window, nullptr);
        glfwDestroyWindow(window);
    }
}

void WindowManager::SwapBuffers() const
{
    glfwSwapBuffers(window);
}

void WindowManager::ClearScreen() const
{
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void WindowManager::PollEvents() const
{
    glfwPollEvents();
}

void WindowManager::AddResizeCallback(const std::string& key, ResizeCallback cb)
{
    resizeCallbacks[key] = std::move(cb);
}

void WindowManager::RemoveResizeCallback(const std::string& key)
{
    resizeCallbacks.erase(key);
}

void WindowManager::NotifyResize(int width, int height)
{
    for (auto& [_, callback] : resizeCallbacks)
    {
        if (callback)
            callback(width, height);
    }
}
