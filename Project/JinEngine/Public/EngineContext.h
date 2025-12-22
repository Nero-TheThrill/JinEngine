#pragma once

#include "InputManager.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "StateManager.h"
#include "WindowManager.h"

struct GLFWwindow;
class JinEngine;

/**
 * @brief Aggregates pointers to core engine subsystems.
 *
 * @details
 * EngineContext acts as a lightweight dependency container that is passed
 * throughout the engine during update, render, and lifecycle calls.
 *
 * It provides unified access to major engine systems such as:
 * - State management
 * - Window and input handling
 * - Rendering
 * - Audio
 * - Core engine control
 *
 * EngineContext does not own these systems.
 * It only stores raw pointers that are managed and lifetime-controlled
 * by JinEngine.
 *
 * This structure is intentionally simple and copyable, and is commonly
 * passed by const reference to avoid tight coupling between systems.
 */
struct EngineContext
{
    /**
     * @brief Pointer to the StateManager responsible for game state transitions.
     */
    StateManager* stateManager = nullptr;

    /**
     * @brief Pointer to the WindowManager handling window creation and events.
     */
    WindowManager* windowManager = nullptr;

    /**
     * @brief Pointer to the InputManager handling keyboard and mouse input.
     */
    InputManager* inputManager = nullptr;

    /**
     * @brief Pointer to the RenderManager responsible for rendering operations.
     */
    RenderManager* renderManager = nullptr;

    /**
     * @brief Pointer to the SoundManager handling audio playback and control.
     */
    SoundManager* soundManager = nullptr;

    /**
     * @brief Pointer to the core JinEngine instance.
     *
     * @details
     * This pointer allows subsystems to query engine-wide settings
     * such as debug rendering flags or global execution state.
     */
    JinEngine* engine = nullptr;
};
