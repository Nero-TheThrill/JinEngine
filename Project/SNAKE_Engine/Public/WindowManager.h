#pragma once
#include <string>
#include "vec4.hpp"
class SNAKE_Engine;
struct GLFWwindow;
struct EngineContext;

/**
 * @brief Manages the application window and its properties.
 *
 * @details
 * WindowManager is responsible for creating, managing, and controlling
 * the main GLFW window used by the engine. It handles operations such as
 * resizing, fullscreen toggling, background color management, and cursor
 * visibility. It acts as a bridge between the OS window system and the engine.
 */
class WindowManager
{
    friend SNAKE_Engine;
    friend void framebuffer_size_callback(GLFWwindow*, int, int);
public:
    WindowManager() :window(nullptr), windowWidth(800), windowHeight(600), backgroundColor(0.4, 0.4, 0.4, 1) {}

    /**
     * @brief Retrieves the underlying GLFW window handle.
     *
     * @return Pointer to the GLFWwindow object.
     * @code
     * GLFWwindow* handle = engineContext.windowManager->GetHandle();
     * @endcode
     */
    [[nodiscard]] GLFWwindow* GetHandle() const { return window; }

    /**
     * @brief Gets the current width of the window.
     *
     * @return Window width in pixels.
     */
    [[nodiscard]] int GetWidth() const { return windowWidth; }

    /**
     * @brief Gets the current height of the window.
     *
     * @return Window height in pixels.
     */
    [[nodiscard]] int GetHeight() const { return windowHeight; }

    /**
     * @brief Resizes the application window.
     *
     * @details
     * Updates the GLFW window dimensions to the given width and height.
     * Also updates the stored dimensions internally.
     *
     * @param width New width in pixels.
     * @param height New height in pixels.
     */
    void Resize(int width, int height);

    /**
     * @brief Sets the title of the window.
     *
     * @param title The new title string to display.
     * @code
     * engineContext.windowManager->SetTitle("My Game - FPS: 60");
     * @endcode
     */
    void SetTitle(const std::string& title) const;

    /**
     * @brief Sets the background clear color of the window.
     *
     * @param color RGBA vector for the background color.
     */
    void SetBackgroundColor(glm::vec4 color) { backgroundColor = color; }

    /**
     * @brief Gets the current background clear color.
     *
     * @return RGBA vector representing the current background color.
     */
    [[nodiscard]] glm::vec4 GetBackgroundColor() const { return backgroundColor; }

    /**
     * @brief Toggles fullscreen mode for the window.
     *
     * @details
     * When enabled, the window switches to fullscreen using the primary monitor.
     * When disabled, it returns to its previous windowed size and position.
     *
     * @param enable True to enable fullscreen, false to return to windowed mode.
     */
    void SetFullScreen(bool enable);

    /**
     * @brief Checks if the window is currently in fullscreen mode.
     *
     * @return True if fullscreen, false otherwise.
     */
    [[nodiscard]] bool IsFullScreen() const { return isFullscreen; }

    /**
     * @brief Enables or disables window resizing by the user.
     *
     * @param shouldRestrict True to disable resizing, false to allow it.
     */
    void RestrictResizing(bool shouldRestrict);

    /**
     * @brief Shows or hides the mouse cursor within the window.
     *
     * @param visible True to show the cursor, false to hide it.
     */
    void SetCursorVisible(bool visible);
private:
    /**
     * @brief Initializes the GLFW window.
     *
     * @param _windowWidth Desired window width in pixels.
     * @param _windowHeight Desired window height in pixels.
     * @param engine Reference to the engine, required for event callbacks.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Init(int _windowWidth, int _windowHeight, SNAKE_Engine& engine);

    /**
     * @brief Sets the internal width value.
     *
     * @param width New width in pixels.
     */
    void SetWidth(int width) { this->windowWidth = width; }

    /**
     * @brief Sets the internal height value.
     *
     * @param height New height in pixels.
     */
    void SetHeight(int height) { this->windowHeight = height; }

    /**
     * @brief Swaps the front and back buffers.
     *
     * @details
     * Called once per frame at the end of the render loop to present the rendered image.
     */
    void SwapBuffers() const;

    /**
     * @brief Clears the window screen with the set background color.
     *
     * @details
     * Internally calls `glClearColor` and `glClear(GL_COLOR_BUFFER_BIT)`.
     */
    void ClearScreen() const;

    /**
     * @brief Polls for window and input events.
     *
     * @details
     * Wraps `glfwPollEvents` and should be called once per frame in the main loop.
     */
    void PollEvents() const;

    /**
     * @brief Releases and destroys the GLFW window.
     *
     * @details
     * Destroys the GLFW window and unregisters callbacks.
     */
    void Free() const;

    GLFWwindow* window;
    int windowWidth;
    int windowHeight;
    glm::vec4 backgroundColor;

    bool isFullscreen = false;
    bool isCursorVisible = false;
    int windowedPosX = 100, windowedPosY = 100;
    int windowedWidth = 800, windowedHeight = 600;
};
