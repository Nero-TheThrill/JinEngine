#pragma once

#include "InputManager.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "StateManager.h"
#include "WindowManager.h"

struct GLFWwindow;
class JinEngine;

/**
 * @brief Lightweight context container that provides access to core engine systems.
 *
 * @details
 * EngineContext is a non-owning structure used to pass references to core engine systems
 * (such as rendering, input, audio, state management, and window management) throughout
 * the engine and gameplay code.
 *
 * Instead of relying on global variables or singletons, the engine passes EngineContext
 * to functions such as GameState::Update(), Draw(), Object update loops, and resource loading.
 *
 * All members are raw pointers and are owned by JinEngine. EngineContext does not manage
 * the lifetime of these systems.
 *
 * Typical usage includes:
 * - Accessing RenderManager to submit draw calls
 * - Accessing InputManager to query input state
 * - Accessing StateManager to change game states
 * - Accessing WindowManager for resolution or viewport information
 * - Accessing SoundManager for audio playback
 *
 * @note
 * All pointers are expected to be valid during runtime after engine initialization.
 *
 * @warning
 * EngineContext does not perform null checks internally. Users must ensure that
 * the engine is properly initialized before accessing its members.
 */
struct EngineContext
{
    /**
     * @brief Pointer to the state manager.
     *
     * @details
     * Used to query or change the current game state.
     */
    StateManager* stateManager = nullptr;

    /**
     * @brief Pointer to the window manager.
     *
     * @details
     * Provides access to window size, background color, and window-related properties.
     */
    WindowManager* windowManager = nullptr;

    /**
     * @brief Pointer to the input manager.
     *
     * @details
     * Used to query keyboard, mouse, and other input states.
     */
    InputManager* inputManager = nullptr;

    /**
     * @brief Pointer to the render manager.
     *
     * @details
     * Used to submit draw calls, clear buffers, and access rendering resources.
     */
    RenderManager* renderManager = nullptr;

    /**
     * @brief Pointer to the sound manager.
     *
     * @details
     * Used to play, stop, and control audio.
     */
    SoundManager* soundManager = nullptr;

    /**
     * @brief Pointer to the main engine instance.
     *
     * @details
     * Provides access to engine-level functionality such as quitting the application.
     */
    JinEngine* engine = nullptr;
};