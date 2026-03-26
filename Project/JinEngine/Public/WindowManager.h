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

    /**
     * @brief Constructs a WindowManager with default window state.
     *
     * @details
     * Initializes the internal window handle to nullptr and sets the default
     * window size to 800x600. The default clear color is initialized to
     * (0.4, 0.4, 0.4, 1.0). This constructor only prepares the manager state;
     * it does not create a GLFW window or initialize OpenGL.
     *
     * @note
     * Actual window creation happens later through Init().
     */
    WindowManager() :window(nullptr), windowWidth(800), windowHeight(600), backgroundColor(0.4, 0.4, 0.4, 1) {}

    /**
     * @brief Returns the raw GLFW window handle.
     *
     * @details
     * Provides direct access to the internally managed GLFWwindow pointer.
     * This is mainly used by engine systems that need to call GLFW APIs
     * directly, such as input initialization or low-level window operations.
     *
     * @return
     * The internal GLFWwindow pointer, or nullptr if the window has not been
     * created yet.
     */
    [[nodiscard]] GLFWwindow* GetHandle() const { return window; }

    /**
     * @brief Returns the current cached window width.
     *
     * @details
     * This value is updated during initialization, explicit Resize() calls,
     * fullscreen transitions, and framebuffer resize callbacks.
     *
     * @return
     * The current window width in pixels.
     */
    [[nodiscard]] int GetWidth() const { return windowWidth; }

    /**
     * @brief Returns the current cached window height.
     *
     * @details
     * This value is updated during initialization, explicit Resize() calls,
     * fullscreen transitions, and framebuffer resize callbacks.
     *
     * @return
     * The current window height in pixels.
     */
    [[nodiscard]] int GetHeight() const { return windowHeight; }

    /**
     * @brief Returns the current cached window size as a 2D integer vector.
     *
     * @details
     * Combines the internally stored width and height into a single
     * glm::ivec2 value. This is a convenience wrapper around GetWidth()
     * and GetHeight().
     *
     * @return
     * A glm::ivec2 containing {windowWidth, windowHeight}.
     */
    glm::ivec2 GetWindowSize() const { return glm::ivec2(windowWidth, windowHeight); }

    /**
     * @brief Requests a window size change.
     *
     * @details
     * Calls glfwSetWindowSize() on the managed window and immediately updates
     * the cached width and height values stored in this manager. This function
     * only performs the resize request when a valid window exists.
     *
     * Unlike the framebuffer resize callback path, this function itself does
     * not directly update camera sizes, render targets, or registered resize
     * callbacks. Those follow-up reactions occur through the GLFW resize
     * callback when the underlying windowing system reports the size change.
     *
     * @param width
     * The new requested window width in pixels.
     *
     * @param height
     * The new requested window height in pixels.
     */
    void Resize(int width, int height);

    /**
     * @brief Sets the native window title text.
     *
     * @details
     * Forwards the given string to glfwSetWindowTitle(). The engine uses this
     * during the main loop to display the current FPS in the title bar.
     *
     * @param title
     * The title string to display on the window.
     */
    void SetTitle(const std::string& title) const;

    /**
     * @brief Sets the background clear color used when clearing the frame.
     *
     * @details
     * Stores the color that will later be used by ClearScreen(). The actual
     * OpenGL clear call does not happen here.
     *
     * @param color
     * RGBA color used as the frame clear color.
     */
    void SetBackgroundColor(glm::vec4 color) { backgroundColor = color; }

    /**
     * @brief Returns the currently stored background clear color.
     *
     * @details
     * The returned color is used by both WindowManager::ClearScreen() and the
     * RenderManager offscreen frame begin path when clearing the current frame.
     *
     * @return
     * The currently stored RGBA clear color.
     */
    [[nodiscard]] glm::vec4 GetBackgroundColor() const { return backgroundColor; }

    /**
     * @brief Switches the window between windowed mode and fullscreen mode.
     *
     * @details
     * When entering fullscreen, the current windowed position and size are
     * saved so that they can be restored later. The function then attaches
     * the window to the primary monitor using that monitor's current video
     * mode. When leaving fullscreen, the saved windowed position and size
     * are restored.
     *
     * The internal cached width and height are updated immediately after the
     * mode switch. This function returns early if no window exists or if the
     * requested fullscreen state already matches the current state.
     *
     * @param enable
     * True to switch to fullscreen mode, false to restore windowed mode.
     *
     * @note
     * This function updates the cached size values directly, but any
     * framebuffer-dependent systems should still rely on the actual resize
     * callback path for full synchronization.
     */
    void SetFullScreen(bool enable);

    /**
     * @brief Returns whether the manager is currently in fullscreen mode.
     *
     * @return
     * True if fullscreen mode is enabled, otherwise false.
     */
    [[nodiscard]] bool IsFullScreen() const { return isFullscreen; }

    /**
     * @brief Enables or disables user-driven window resizing.
     *
     * @details
     * Internally calls glfwSetWindowAttrib() with GLFW_RESIZABLE. Passing true
     * restricts resizing by making the window non-resizable, while passing
     * false allows the user to resize the window.
     *
     * @param shouldRestrict
     * True to prevent manual resizing, false to allow it.
     */
    void RestrictResizing(bool shouldRestrict);

    /**
     * @brief Shows or hides the mouse cursor for this window.
     *
     * @details
     * Uses glfwSetInputMode() with GLFW_CURSOR. When visible is true, the
     * cursor is restored to normal display mode. When false, the cursor is
     * hidden. This function only changes GLFW cursor mode and does not update
     * the isCursorVisible member variable in the current implementation.
     *
     * @param visible
     * True to show the cursor, false to hide it.
     *
     * @note
     * The current implementation changes the actual GLFW cursor state, but the
     * cached isCursorVisible flag is not synchronized here.
     */
    void SetCursorVisible(bool visible);

    /**
     * @brief Registers or replaces a resize callback under a string key.
     *
     * @details
     * Stores the callback in an internal unordered_map using the given key.
     * If the same key already exists, the previous callback is overwritten.
     * Registered callbacks are invoked from NotifyResize(), which is triggered
     * by the framebuffer size callback after engine-side resize handling has
     * already updated the window size, active state camera sizes, and render
     * target sizes.
     *
     * @param key
     * Unique identifier for the callback entry.
     *
     * @param cb
     * Callback to invoke with the new width and height.
     */
    void AddResizeCallback(const std::string& key, ResizeCallback cb);

    /**
     * @brief Removes a previously registered resize callback.
     *
     * @details
     * Erases the callback associated with the given key from the internal
     * callback map. If the key does not exist, nothing happens.
     *
     * @param key
     * Identifier of the callback to remove.
     */
    void RemoveResizeCallback(const std::string& key);

private:
    /**
     * @brief Creates the GLFW window and initializes the window subsystem.
     *
     * @details
     * This is the core startup routine used by JinEngine::Init(). It
     * initializes GLFW, configures the OpenGL context version and profile,
     * creates the native window, makes the context current, loads GLAD,
     * enables OpenGL debug output, sets the initial viewport, stores the
     * engine pointer as the GLFW user pointer, and installs the engine's
     * window/input callbacks.
     *
     * Installed callbacks include:
     * - framebuffer size callback for resize propagation
     * - scroll callback for InputManager scroll accumulation
     * - key callback for staged keyboard input
     * - mouse button callback for staged mouse input
     * - window close callback to request engine shutdown
     *
     * @param _windowWidth
     * Initial window width in pixels.
     *
     * @param _windowHeight
     * Initial window height in pixels.
     *
     * @param engine
     * Owning engine instance used as the GLFW user pointer so callbacks can
     * reach EngineContext and related managers.
     *
     * @return
     * True if initialization succeeds, otherwise false.
     *
     * @note
     * This function is intentionally private because the engine controls the
     * full lifecycle of the window.
     */
    bool Init(int _windowWidth, int _windowHeight, JinEngine& engine);

    /**
     * @brief Updates the cached width value.
     *
     * @details
     * This helper is primarily used by the framebuffer resize callback after
     * GLFW reports the actual framebuffer size.
     *
     * @param width
     * New cached width.
     */
    void SetWidth(int width) { this->windowWidth = width; }

    /**
     * @brief Updates the cached height value.
     *
     * @details
     * This helper is primarily used by the framebuffer resize callback after
     * GLFW reports the actual framebuffer size.
     *
     * @param height
     * New cached height.
     */
    void SetHeight(int height) { this->windowHeight = height; }

    /**
     * @brief Updates the cached window size using a 2D vector.
     *
     * @param size
     * New cached size expressed as {width, height}.
     */
    void SetWindowSize(glm::ivec2 size) { this->windowWidth = size.x; this->windowHeight = size.y; }

    /**
     * @brief Swaps the front and back buffers of the window.
     *
     * @details
     * Called once per frame near the end of JinEngine::Run() after update,
     * draw, and audio processing are complete.
     */
    void SwapBuffers() const;

    /**
     * @brief Clears the current framebuffer using the stored background color.
     *
     * @details
     * Calls glClearColor() with the color configured through
     * SetBackgroundColor(), then clears the color buffer bit.
     *
     * @note
     * In the current engine loop this is called before state update and draw.
     * If RenderManager offscreen rendering is enabled, BeginFrame() performs
     * its own clear on the offscreen target as well.
     */
    void ClearScreen() const;

    /**
     * @brief Polls and dispatches pending window/input events.
     *
     * @details
     * Wraps glfwPollEvents() and is called once per frame in the main engine
     * loop before input state promotion and game-state update.
     */
    void PollEvents() const;

    /**
     * @brief Destroys the window and unregisters installed GLFW callbacks.
     *
     * @details
     * Clears the framebuffer, scroll, key, mouse button, and close callbacks,
     * removes the user pointer, and destroys the GLFW window. This is part of
     * the engine shutdown sequence executed after state, render, and sound
     * systems have been freed.
     *
     * @note
     * The current implementation destroys the GLFW window but does not null out
     * the stored pointer afterward.
     */
    void Free() const;

    /**
     * @brief Invokes all registered resize callbacks.
     *
     * @details
     * Iterates through every stored callback and passes the new width and
     * height values to each valid entry. This is called from the framebuffer
     * resize callback after core engine resize propagation has already run.
     *
     * @param width
     * Updated framebuffer width.
     *
     * @param height
     * Updated framebuffer height.
     */
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