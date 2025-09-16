#pragma once

#include "InputManager.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "StateManager.h"
#include "WindowManager.h"

struct GLFWwindow;
class SNAKE_Engine;

/**
 * @brief Central context holding pointers to all core engine subsystems.
 *
 * @details
 * The EngineContext structure aggregates references to the major subsystems of the engine,
 * providing a unified way for game states, objects, and managers to access engine services.
 *
 * It is initialized inside `SNAKE_Engine::SetEngineContext()` and passed throughout the engine
 * to ensure all systems can communicate without needing direct global access.
 *
 * Members include:
 * - StateManager: Manages game state transitions, updates, and rendering calls.
 * - WindowManager: Handles window creation, resizing, fullscreen toggling, and input callbacks.
 * - InputManager: Provides input querying (keys, mouse, scroll, world positions).
 * - RenderManager: Responsible for resource registration, frame rendering, and draw batching.
 * - SoundManager: Manages audio playback, volume control, and cleanup of sound instances.
 * - SNAKE_Engine: Back-reference to the main engine instance, used for global operations such as quitting.
 */
struct EngineContext
{
    StateManager* stateManager = nullptr;   ///< Pointer to the game state manager.
    WindowManager* windowManager = nullptr; ///< Pointer to the window and display manager.
    InputManager* inputManager = nullptr;   ///< Pointer to the input manager for key/mouse queries.
    RenderManager* renderManager = nullptr; ///< Pointer to the render manager for graphics and resources.
    SoundManager* soundManager = nullptr;   ///< Pointer to the sound manager for audio playback.
    SNAKE_Engine* engine = nullptr;         ///< Back-reference to the main engine instance.
};
