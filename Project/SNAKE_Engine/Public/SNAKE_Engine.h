#pragma once
#include "EngineContext.h"

/**
 * @brief Core engine class that manages all subsystems and the main loop.
 *
 * @details
 * SNAKE_Engine is the root object that initializes subsystems (window,
 * input, rendering, audio, and states), manages the game loop, and ensures
 * proper shutdown. It exposes a reference to EngineContext for access
 * to subsystems, and provides entry points for initialization, execution,
 * quitting, and debug rendering control.
 */
class SNAKE_Engine
{
public:
    SNAKE_Engine() = default;

    /**
     * @brief Initializes the engine and its subsystems.
     *
     * @details
     * Sets up the window, input manager, rendering manager, sound manager,
     * and state manager. The EngineContext is populated with pointers
     * to all subsystems so that states and objects can access them.
     *
     * @param windowWidth Desired width of the application window.
     * @param windowHeight Desired height of the application window.
     * @return True if initialization succeeded, false otherwise.
     * @code
     * SNAKE_Engine engine;
     * if (engine.Init(1280, 720))
     *     engine.Run();
     * @endcode
     */
    [[nodiscard]] bool Init(int windowWidth, int windowHeight);

    /**
     * @brief Runs the main engine loop.
     *
     * @details
     * Executes the loop until RequestQuit() is called. Each frame performs:
     * - Event polling
     * - Delta time calculation
     * - StateManager update and draw
     * - Debug draw (if enabled)
     * - Swap buffers
     *
     * This function blocks until the engine shuts down.
     */
    void Run();

    /**
     * @brief Requests the engine loop to exit.
     *
     * @details
     * Sets an internal flag that causes Run() to terminate after the
     * current frame completes. Typically bound to user input like ESC.
     */
    void RequestQuit();

    /**
     * @brief Returns the engine context.
     *
     * @details
     * Provides access to all subsystems (render manager, state manager,
     * input manager, etc.). Useful for game states and objects.
     *
     * @return Reference to EngineContext.
     */
    [[nodiscard]] EngineContext& GetEngineContext() { return engineContext; }

    /**
     * @brief Enables or disables debug rendering.
     *
     * @param shouldShow True to render debug overlays, false to disable.
     */
    void RenderDebugDraws(bool shouldShow) { showDebugDraw = shouldShow; }

    /**
     * @brief Indicates whether debug rendering is enabled.
     *
     * @details
     * Intended as an internal check for RenderManager. Not typically called
     * directly by user code.
     *
     * @return True if debug rendering is enabled.
     */
    [[nodiscard]] bool ShouldRenderDebugDraws() const { return showDebugDraw; }

private:
    /**
     * @brief Releases all engine resources.
     *
     * @details
     * Calls Free() on subsystems and cleans up the EngineContext.
     * Invoked when Run() finishes or Init() fails.
     */
    void Free() const;

    /**
     * @brief Populates EngineContext with subsystem pointers.
     *
     * @details
     * Ensures that subsystems (window, input, render, sound, states)
     * are accessible through EngineContext for use in game states.
     */
    void SetEngineContext();

    EngineContext engineContext; ///< Holds references to subsystems.
    StateManager stateManager;   ///< Manages active and next game states.
    WindowManager windowManager; ///< Creates and manages the application window.
    InputManager inputManager;   ///< Handles keyboard, mouse, and input events.
    RenderManager renderManager; ///< Manages rendering and GPU resources.
    SoundManager soundManager;   ///< Handles audio playback via miniaudio.
    bool shouldRun = true;       ///< Flag controlling the main loop execution.
    bool showDebugDraw = false;  ///< Flag for enabling debug rendering overlays.
};
