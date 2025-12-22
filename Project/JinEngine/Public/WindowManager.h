#pragma once
#include <string>
#include "vec4.hpp"
class JinEngine;
struct GLFWwindow;
struct EngineContext;

/**
 * @brief Manages the application's primary GLFW window and related OS-facing behaviors.
 *
 * @details
 * WindowManager owns the GLFWwindow handle and tracks the current window size and presentation state
 * (windowed vs fullscreen). It also stores a background clear color used by the engine's per-frame
 * clear routine.
 *
 * Resize handling is driven by the GLFW framebuffer size callback (framebuffer_size_callback),
 * which updates this manager's cached width/height, updates all cameras' screen sizes through the
 * active GameState's CameraManager, notifies the RenderManager about the resize, and finally invokes
 * any user-registered resize callbacks via NotifyResize().
 *
 * Most lifecycle and per-frame operations (Init/SwapBuffers/ClearScreen/PollEvents/Free) are private
 * and are intended to be orchestrated by JinEngine.
 */
class WindowManager
{
    friend JinEngine;
    friend void framebuffer_size_callback(GLFWwindow*, int, int);
public:
    /**
     * @brief Callback signature for resize notifications.
     *
     * @details
     * The callback receives the new framebuffer width/height in pixels, matching the values reported
     * by GLFW's framebuffer size callback.
     */
    using ResizeCallback = std::function<void(int /*width*/, int /*height*/)>;

    /**
     * @brief Constructs the manager with a null window handle and default window settings.
     *
     * @details
     * The default size is 800x600 and the default background clear color is (0.4, 0.4, 0.4, 1).
     * The actual window is created during Init().
     */
    WindowManager() :window(nullptr), windowWidth(800), windowHeight(600), backgroundColor(0.4, 0.4, 0.4, 1) {}

    /**
     * @brief Returns the underlying GLFWwindow handle.
     *
     * @return A raw GLFWwindow pointer, or nullptr if Init() has not created the window yet.
     */
    [[nodiscard]] GLFWwindow* GetHandle() const { return window; }

    /**
     * @brief Returns the cached framebuffer width of the window in pixels.
     *
     * @details
     * This value is updated when Resize() is called and also when GLFW reports a framebuffer size change.
     */
    [[nodiscard]] int GetWidth() const { return windowWidth; }

    /**
     * @brief Returns the cached framebuffer height of the window in pixels.
     *
     * @details
     * This value is updated when Resize() is called and also when GLFW reports a framebuffer size change.
     */
    [[nodiscard]] int GetHeight() const { return windowHeight; }

    /**
     * @brief Requests the window be resized to the given width/height.
     *
     * @details
     * This calls glfwSetWindowSize() and updates the cached width/height immediately.
     * Note that GLFW may also trigger the framebuffer size callback depending on platform and DPI scaling.
     *
     * @param width  Target width in pixels.
     * @param height Target height in pixels.
     */
    void Resize(int width, int height);

    /**
     * @brief Sets the OS window title text.
     *
     * @param title New title string to pass to GLFW.
     */
    void SetTitle(const std::string& title) const;

    /**
     * @brief Sets the background clear color used by ClearScreen().
     *
     * @details
     * The color is stored and later applied through glClearColor() when the engine clears the frame.
     *
     * @param color RGBA color in linear float components.
     */
    void SetBackgroundColor(glm::vec4 color) { backgroundColor = color; }

    /**
     * @brief Returns the currently configured background clear color.
     */
    [[nodiscard]] glm::vec4 GetBackgroundColor() const { return backgroundColor; }

    /**
     * @brief Toggles fullscreen mode using the primary monitor.
     *
     * @details
     * When enabling fullscreen, the current windowed position and size are saved and the window is
     * switched to the primary monitor's current video mode. When disabling, the saved windowed
     * position/size are restored.
     *
     * If the window is null or the requested state matches the current state, this function does nothing.
     *
     * @param enable True to enter fullscreen, false to return to windowed mode.
     */
    void SetFullScreen(bool enable);

    /**
     * @brief Returns whether the window is currently in fullscreen mode.
     */
    [[nodiscard]] bool IsFullScreen() const { return isFullscreen; }

    /**
     * @brief Enables or disables user resizing of the window.
     *
     * @details
     * Internally this sets the GLFW_RESIZABLE window attribute. When shouldRestrict is true,
     * the window becomes non-resizable.
     *
     * @param shouldRestrict True to prevent resizing, false to allow resizing.
     */
    void RestrictResizing(bool shouldRestrict);

    /**
     * @brief Shows or hides the OS cursor while the window is focused.
     *
     * @details
     * This maps to GLFW cursor input modes (GLFW_CURSOR_NORMAL vs GLFW_CURSOR_HIDDEN).
     * Note that this function does not update the cached isCursorVisible flag.
     *
     * @param visible True to show the cursor, false to hide it.
     */
    void SetCursorVisible(bool visible);

    /**
     * @brief Registers or replaces a resize callback identified by a string key.
     *
     * @details
     * The callback is stored in an internal map. If the key already exists, the previous callback
     * is replaced.
     *
     * Callbacks are invoked from NotifyResize(), which is called by the GLFW framebuffer size callback.
     *
     * @param key Unique identifier for the callback entry.
     * @param cb  Callable to execute when the framebuffer size changes.
     */
    void AddResizeCallback(const std::string& key, ResizeCallback cb);

    /**
     * @brief Removes a previously registered resize callback by key.
     *
     * @param key Key used when the callback was registered.
     */
    void RemoveResizeCallback(const std::string& key);

private:
    /**
     * @brief Initializes GLFW, creates the window, and installs engine-level callbacks.
     *
     * @details
     * Init() performs the following:
     * - Initializes GLFW and configures an OpenGL core profile context (4.6).
     * - Creates a GLFW window and makes its context current.
     * - Initializes GLAD for OpenGL function loading.
     * - Enables OpenGL debug output and installs a debug message callback (non-notification severities).
     * - Sets the GLFW user pointer to the provided JinEngine instance.
     * - Registers callbacks for framebuffer resize, scroll, key input, mouse buttons, and window close.
     *
     * The framebuffer resize callback updates WindowManager dimensions, updates cameras, notifies the
     * RenderManager, and then broadcasts to user resize callbacks.
     *
     * @param _windowWidth  Initial window width.
     * @param _windowHeight Initial window height.
     * @param engine        Engine instance stored into GLFW's window user pointer.
     * @return True if initialization succeeds and the window is created; false otherwise.
     */
    bool Init(int _windowWidth, int _windowHeight, JinEngine& engine);

    /**
     * @brief Updates the cached window width without calling GLFW APIs.
     *
     * @details
     * This is primarily used by the GLFW framebuffer size callback to keep the cached dimensions in sync.
     *
     * @param width New cached width in pixels.
     */
    void SetWidth(int width) { this->windowWidth = width; }

    /**
     * @brief Updates the cached window height without calling GLFW APIs.
     *
     * @details
     * This is primarily used by the GLFW framebuffer size callback to keep the cached dimensions in sync.
     *
     * @param height New cached height in pixels.
     */
    void SetHeight(int height) { this->windowHeight = height; }

    /**
     * @brief Swaps the front/back buffers of the window.
     *
     * @details
     * This wraps glfwSwapBuffers(window) and is intended to be called once per frame by the engine.
     */
    void SwapBuffers() const;

    /**
     * @brief Clears the color buffer using the current backgroundColor.
     *
     * @details
     * This calls glClearColor(backgroundColor) followed by glClear(GL_COLOR_BUFFER_BIT).
     */
    void ClearScreen() const;

    /**
     * @brief Polls GLFW events.
     *
     * @details
     * This wraps glfwPollEvents() and is intended to be called once per frame by the engine.
     */
    void PollEvents() const;

    /**
     * @brief Releases the GLFW window and unregisters callbacks.
     *
     * @details
     * This clears installed GLFW callbacks, clears the window user pointer, destroys the GLFW window,
     * and leaves the WindowManager's raw window handle in its current state.
     *
     * This function does not call glfwTerminate().
     */
    void Free() const;

    /**
     * @brief Invokes all registered resize callbacks with the new size.
     *
     * @details
     * Each stored callback is checked for validity before invocation.
     *
     * @param width  New framebuffer width in pixels.
     * @param height New framebuffer height in pixels.
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
