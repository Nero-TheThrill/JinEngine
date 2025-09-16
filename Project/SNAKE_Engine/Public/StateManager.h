#pragma once
#include <memory>

class GameState;
class SNAKE_Engine;
struct EngineContext;

/**
 * @brief Manages game states and their transitions.
 *
 * @details
 * The StateManager holds the currently active GameState and handles transitions
 * to new states. It ensures proper unloading, freeing, and reinitialization of
 * resources when switching between states. Internally, the manager defers state
 * changes until the next update tick to avoid inconsistent state transitions.
 *
 * Example usage in engine loop:
 * @code
 * stateManager.Update(dt, engineContext);
 * stateManager.Draw(engineContext);
 * @endcode
 */
class StateManager
{
    friend SNAKE_Engine;
public:
    /**
     * @brief Returns the currently active GameState.
     *
     * @details
     * If no state has been set yet, this function returns nullptr.
     *
     * @return Pointer to the current GameState or nullptr if none.
     * @code
     * GameState* state = engineContext.stateManager->GetCurrentState();
     * if (state)
     *     state->DoSomething();
     * @endcode
     */
    [[nodiscard]] GameState* GetCurrentState() const;

    /**
     * @brief Requests a change to a new GameState.
     *
     * @details
     * The new state will not be activated immediately. Instead, it is stored
     * in `nextState` and will be applied during the next `Update()` call.
     * This ensures a clean transition with proper resource unloading and loading.
     *
     * @param newState Unique pointer to the new GameState to transition into.
     * @note
     * The currently active state's `SystemFree` and `SystemUnload` will be called
     * before the new state's `SystemLoad` and `SystemInit` are invoked.
     * @code
     * engineContext.stateManager->ChangeState(std::make_unique<MyGameState>());
     * @endcode
     */
    void ChangeState(std::unique_ptr<GameState> newState);

private:
    /**
     * @brief Updates the currently active GameState and handles pending transitions.
     *
     * @details
     * - If a `nextState` exists, the current state is freed and unloaded,
     *   and the new state is loaded and initialized.
     * - After transition, or if no transition is pending, the active state's
     *   `SystemUpdate` is called with the given delta time.
     *
     * @param dt Delta time since last frame in seconds.
     * @param engineContext Reference to the engine context providing managers.
     */
    void Update(float dt, const EngineContext& engineContext);

    /**
     * @brief Renders the current GameState.
     *
     * @details
     * This function calls:
     * - `RenderManager::BeginFrame`
     * - The current state's `Draw`
     * - Flushes draw commands (including optional debug draws)
     * - `RenderManager::EndFrame`
     * - Calls the current state's `PostProcessing`
     *
     * @param engineContext Reference to the engine context.
     */
    void Draw(const EngineContext& engineContext);

    /**
     * @brief Frees the resources of the currently active GameState.
     *
     * @details
     * Calls the state's `SystemFree` and `SystemUnload` methods
     * before resetting the current state pointer.
     *
     * @param engineContext Reference to the engine context.
     */
    void Free(const EngineContext& engineContext);

    std::unique_ptr<GameState> currentState; ///< Currently active game state.
    std::unique_ptr<GameState> nextState;    ///< State scheduled to replace the current one.
};
