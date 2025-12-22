#pragma once
#include "EngineContext.h"

/**
 * @brief Top-level engine facade that owns core managers and drives the main game loop.
 *
 * @details
 * JinEngine is the central orchestrator that:
 * - Owns and initializes the window, input, render, sound, and state systems.
 * - Builds an EngineContext that exposes pointers/references to those systems for use by GameState/Object code.
 * - Runs the main loop (Run) until RequestQuit() is called.
 *
 * Lifetime:
 * - Init() creates/initializes core subsystems and sets up EngineContext.
 * - Run() repeatedly updates input, state, rendering, and window events until shouldRun becomes false.
 * - Free() performs shutdown/cleanup in the correct order.
 *
 * Debug draw:
 * - RenderDebugDraws() toggles engine-wide debug rendering.
 * - ShouldRenderDebugDraws() is mainly used internally by rendering/debug systems to branch behavior.
 */
class JinEngine
{
public:
    /**
     * @brief Constructs the engine with default state.
     */
    JinEngine() = default;

    /**
     * @brief Initializes all core subsystems and prepares the engine to run.
     *
     * @details
     * Expected responsibilities:
     * - Initialize WindowManager with the requested window size.
     * - Initialize RenderManager and create required default resources (fallback shader/texture/mesh/material).
     * - Initialize InputManager with a valid window handle.
     * - Initialize SoundManager audio backend.
     * - Initialize StateManager to a valid initial state (or a loading/boot state), depending on engine setup.
     * - Build EngineContext so that it exposes the above systems consistently to GameState/Object code.
     *
     * If any subsystem fails to initialize, Init() returns false and the engine should not Run().
     *
     * @param windowWidth  Initial window width in pixels.
     * @param windowHeight Initial window height in pixels.
     * @return True if initialization succeeded; false otherwise.
     */
    [[nodiscard]] bool Init(int windowWidth, int windowHeight);

    /**
     * @brief Runs the main loop until RequestQuit() is called.
     *
     * @details
     * Expected responsibilities per frame:
     * - Poll window events and update input state.
     * - Update the active GameState through StateManager (Update/Draw/Free transition handling).
     * - BeginFrame/EndFrame rendering via RenderManager and swap buffers via WindowManager.
     *
     * The loop continues while shouldRun is true.
     */
    void Run();

    /**
     * @brief Requests the engine to stop running at the next safe point in the main loop.
     *
     * @details
     * Sets shouldRun to false. Run() is expected to observe this flag and exit its loop.
     */
    void RequestQuit();

    /**
     * @brief Returns the EngineContext owned by the engine.
     *
     * @details
     * EngineContext is configured by SetEngineContext() and typically contains pointers/references
     * to WindowManager, InputManager, RenderManager, SoundManager, StateManager, and other shared systems.
     *
     * @return Mutable reference to the engine's EngineContext.
     */
    [[nodiscard]] EngineContext& GetEngineContext() { return engineContext; }

    /**
     * @brief Enables or disables rendering of engine-level debug visuals.
     *
     * @details
     * This is a global toggle used by debug rendering paths (e.g., collider debug drawing, line debug drawing).
     * Render systems should consult ShouldRenderDebugDraws() to decide whether to submit/flush debug primitives.
     *
     * @param shouldShow True to enable debug draw; false to disable.
     */
    void RenderDebugDraws(bool shouldShow) { showDebugDraw = shouldShow; }

    /**
     * @brief Returns whether debug visuals should be rendered.
     *
     * @details
     * This function is primarily an internal branch point for renderer/debug systems.
     * External gameplay code typically should not need to query it directly; use RenderDebugDraws()
     * to control the behavior instead.
     *
     * @return True if debug draw is enabled; false otherwise.
     */
    [[nodiscard]] bool ShouldRenderDebugDraws() const { return showDebugDraw; }
private:
    /**
     * @brief Releases core subsystems owned by the engine.
     *
     * @details
     * Expected responsibilities:
     * - Request GameState shutdown through StateManager (Free/Unload if applicable).
     * - Release renderer resources and GPU objects through RenderManager.
     * - Shut down audio through SoundManager.
     * - Destroy the window and terminate related platform resources through WindowManager.
     *
     * This is called during engine shutdown after Run() ends or if Init() fails after partial initialization.
     */
    void Free() const;

    /**
     * @brief Populates EngineContext so all systems can be accessed consistently by runtime code.
     *
     * @details
     * Expected responsibilities:
     * - Assign pointers/references in engineContext to the owned managers (stateManager, windowManager, etc.).
     * - Store any derived/shared runtime values that should be globally accessible.
     *
     * Must be called after the owned manager instances are in a valid initialized state.
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
