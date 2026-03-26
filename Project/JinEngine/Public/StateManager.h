#pragma once
#include <memory>

class GameState;
class JinEngine;
struct EngineContext;

/**
 * @brief Manages the active game state and performs deferred state transitions for the engine.
 *
 * @details
 * StateManager owns the currently active GameState and an optional pending GameState
 * that has been requested through ChangeState().
 *
 * State changes are not applied immediately when ChangeState() is called.
 * Instead, the new state is stored in `nextState` and the actual transition is processed
 * inside Update(). This deferred design helps avoid replacing the active state in the middle
 * of user logic or rendering code.
 *
 * During a state transition, StateManager performs the following sequence:
 * - calls SystemFree() on the old state,
 * - calls SystemUnload() on the old state,
 * - stops all active sounds through SoundManager,
 * - moves the pending state into currentState,
 * - calls SystemLoad() on the new state,
 * - calls SystemInit() on the new state,
 * - updates all cameras in the new state's CameraManager to match the current window size.
 *
 * In addition to state switching, this class also drives the main per-frame update and draw
 * flow for the current state.
 *
 * @note
 * StateManager owns states exclusively through std::unique_ptr.
 *
 * @note
 * This class is intended to be controlled internally by the engine.
 * JinEngine is declared as a friend so it can access lifecycle functions such as Update(), Draw(), and Free().
 */
class StateManager
{
    friend JinEngine;
    friend WindowManager;
public:
    /**
     * @brief Returns the currently active game state.
     *
     * @details
     * This function returns the raw pointer stored in currentState.
     * If no state is currently active, it returns nullptr.
     *
     * @return Pointer to the currently active GameState.
     * @return nullptr if no state is active.
     *
     * @note
     * The returned pointer is non-owning.
     * State lifetime is still managed entirely by StateManager.
     */
    [[nodiscard]] GameState* GetCurrentState() const;

    /**
     * @brief Schedules a transition to a new game state.
     *
     * @details
     * This function does not immediately replace the current state.
     * Instead, it stores the provided state in nextState, and the actual transition
     * is performed during the next Update() call.
     *
     * If another pending state already exists, it is overwritten by the new one.
     *
     * @param newState New state to activate on the next update step.
     *
     * @note
     * The transition is deferred intentionally so that state replacement happens at a controlled point
     * in the engine loop.
     */
    void ChangeState(std::unique_ptr<GameState> newState);

private:

    /**
     * @brief Processes pending state transitions and updates the active state.
     *
     * @details
     * This function first checks whether a pending state exists in nextState.
     * If so, it performs a full state transition.
     *
     * Transition flow:
     * - If currentState exists:
     *   - call currentState->SystemFree(engineContext)
     *   - call currentState->SystemUnload(engineContext)
     *   - stop all sounds through engineContext.soundManager->ControlAll(Stop)
     * - move nextState into currentState
     * - call currentState->SystemLoad(engineContext)
     * - call currentState->SystemInit(engineContext)
     * - resize all cameras owned by the new state to the current window width and height
     *
     * After transition handling, if currentState exists, this function calls
     * currentState->SystemUpdate(dt, engineContext).
     *
     * @param dt Delta time for the current frame.
     * @param engineContext Shared engine systems and managers used by the state.
     *
     * @note
     * ChangeState() only queues the transition. Update() is where the actual swap happens.
     *
     * @note
     * Sound playback is forcefully stopped when leaving the current state.
     */
    void Update(float dt, const EngineContext& engineContext);

    /**
     * @brief Renders the current state and executes the engine's frame rendering sequence.
     *
     * @details
     * If there is an active currentState, this function performs the following rendering flow:
     *
     * 1. BeginFrame() is called on RenderManager.
     * 2. currentState->Draw(engineContext) is executed.
     * 3. RenderManager::FlushDrawCommands(engineContext) is called.
     * 4. If debug rendering is enabled, RenderManager::FlushDebugLineDrawCommands(engineContext) is called.
     * 5. EndFrame() is called on RenderManager.
     * 6. currentState->PostProcessing(engineContext) is executed.
     * 7. RenderManager::FlushDrawCommands(engineContext) is called again for post-processing output.
     *
     * If there is no active state, this function does nothing.
     *
     * @param engineContext Shared engine systems and managers used during rendering.
     *
     * @note
     * This function does more than simply call the state's Draw().
     * It also controls the full render-manager frame lifecycle around that draw call.
     *
     * @note
     * Debug draw flushing occurs only when engine->ShouldRenderDebugDraws() returns true.
     */
    void Draw(const EngineContext& engineContext);

    /**
     * @brief Frees and unloads the current state, then clears ownership of it.
     *
     * @details
     * If currentState exists, this function performs:
     * - currentState->SystemFree(engineContext)
     * - currentState->SystemUnload(engineContext)
     * - currentState.reset()
     *
     * After this function finishes, no active state remains.
     *
     * @param engineContext Shared engine systems and managers used by the state during shutdown.
     *
     * @note
     * This function only releases the currently active state.
     * It does not process nextState.
     */
    void Free(const EngineContext& engineContext);

    std::unique_ptr<GameState> currentState;
    std::unique_ptr<GameState> nextState;
};