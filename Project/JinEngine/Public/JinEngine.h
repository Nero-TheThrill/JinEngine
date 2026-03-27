#pragma once
#include "EngineContext.h"

/**
 * @brief Core engine class that manages initialization, main loop execution, and system lifecycle.
 *
 * @details
 * JinEngine is the central coordinator of all engine subsystems including windowing, input, rendering,
 * sound, and state management. It owns all major managers and provides a unified EngineContext that
 * is passed to all systems and game states.
 *
 * The engine lifecycle is as follows:
 * 1. Init() ¡æ Initializes all subsystems and sets up EngineContext
 * 2. Run() ¡æ Executes the main game loop
 * 3. RequestQuit() ¡æ Signals the loop to terminate
 * 4. Free() ¡æ Cleans up resources and terminates GLFW
 *
 * Internally, Run() performs:
 * - Frame timing (EngineTimer)
 * - Input processing
 * - State update and rendering
 * - Sound update
 * - Buffer swapping
 *
 * @note
 * This class should typically be instantiated once per application.
 */
class JinEngine
{
public:
    JinEngine() = default;

    /**
     * @brief Initializes all core engine systems.
     *
     * @details
     * This function sets up the EngineContext and initializes:
     * - WindowManager (creates OpenGL context and window)
     * - InputManager (binds input callbacks)
     * - SoundManager (initializes audio engine)
     * - RenderManager (prepares rendering pipeline)
     *
     * If any critical subsystem fails to initialize, the function returns false.
     *
     * @param windowWidth Width of the application window.
     * @param windowHeight Height of the application window.
     * @return true if initialization succeeded, false otherwise.
     */
    [[nodiscard]] bool Init(int windowWidth, int windowHeight);

    /**
     * @brief Starts the main game loop.
     *
     * @details
     * This function runs the core loop until:
     * - RequestQuit() is called, or
     * - The window is closed by the user.
     *
     * Each frame performs:
     * - Delta time calculation (EngineTimer)
     * - Event polling and input update
     * - StateManager update and draw
     * - SoundManager update
     * - Buffer swap
     *
     * After exiting the loop, all subsystems are properly freed.
     *
     * @note
     * This function blocks until the engine terminates.
     */
    void Run();

    /**
     * @brief Requests termination of the main loop.
     *
     * @details
     * Sets an internal flag that causes Run() to exit on the next frame.
     * This is typically triggered by window close callbacks or user-defined logic.
     */
    void RequestQuit();

    /**
     * @brief Returns a reference to the engine context.
     *
     * @details
     * EngineContext provides access to all core systems such as:
     * - StateManager
     * - RenderManager
     * - InputManager
     * - WindowManager
     * - SoundManager
     *
     * This is the primary interface used by game states and objects.
     *
     * @return Reference to EngineContext.
     */
    [[nodiscard]] EngineContext& GetEngineContext() { return engineContext; }

    /**
     * @brief Enables or disables debug drawing.
     *
     * @details
     * When enabled, debug rendering such as collider outlines or debug lines
     * will be rendered during the frame.
     *
     * @param shouldShow Whether debug drawing should be enabled.
     */
    void RenderDebugDraws(bool shouldShow) { showDebugDraw = shouldShow; }

    /**
     * @brief Checks whether debug drawing is enabled.
     *
     * @details
     * Used internally by RenderManager to determine whether debug draw commands
     * should be flushed.
     *
     * @return true if debug drawing is enabled, false otherwise.
     */
    [[nodiscard]] bool ShouldRenderDebugDraws() const { return showDebugDraw; }

private:
    /**
     * @brief Terminates the engine and releases platform-level resources.
     *
     * @details
     * Currently responsible for terminating GLFW.
     * Called at the end of Run().
     */
    void Free() const;

    /**
     * @brief Initializes the EngineContext structure.
     *
     * @details
     * Assigns pointers of all subsystem managers into EngineContext so that
     * other systems can access them in a unified way.
     *
     * This must be called before any subsystem initialization.
     */
    void SetEngineContext();

    EngineContext engineContext;
    StateManager stateManager;
    WindowManager windowManager;
    InputManager inputManager;
    RenderManager renderManager;
    SoundManager soundManager;

    bool shouldRun = true;
    bool showDebugDraw = false;
};