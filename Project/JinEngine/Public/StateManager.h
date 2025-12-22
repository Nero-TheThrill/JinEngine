#pragma once
#include <memory>

class GameState;
class JinEngine;
struct EngineContext;

/**
 * @brief Manages the lifetime and transition of GameState instances.
 *
 * @details
 * StateManager is responsible for owning the currently active GameState and
 * safely transitioning to a new state when requested. State changes are not
 * applied immediately; instead, the new state is stored temporarily and the
 * actual transition occurs at a controlled point in the frame lifecycle.
 *
 * This deferred transition model ensures that state changes do not occur in the
 * middle of Update or Draw execution, preventing invalid access and partial
 * state updates.
 *
 * Update, Draw, and Free are driven exclusively by JinEngine and are not intended
 * to be called directly by user code.
 */
class StateManager
{
    friend JinEngine;
    friend WindowManager;
public:
    /**
     * @brief Returns the currently active GameState.
     *
     * @details
     * The returned pointer is owned by StateManager and remains valid until
     * a state transition occurs.
     *
     * @return Pointer to the active GameState, or nullptr if no state is active.
     */
    [[nodiscard]] GameState* GetCurrentState() const;

    /**
     * @brief Requests a transition to a new GameState.
     *
     * @details
     * The provided GameState is stored internally as the next state and will
     * replace the current state at a safe synchronization point, typically
     * between frames.
     *
     * If a next state is already pending, it will be overwritten.
     *
     * @param newState Unique ownership of the new GameState to activate.
     */
    void ChangeState(std::unique_ptr<GameState> newState);

private:
    /**
     * @brief Updates the currently active GameState.
     *
     * @details
     * This function forwards the update call to the current GameState and also
     * handles pending state transitions if a new state has been requested.
     *
     * @param dt             Delta time in seconds.
     * @param engineContext Engine-wide context.
     */
    void Update(float dt, const EngineContext& engineContext);

    /**
     * @brief Draws the currently active GameState.
     *
     * @details
     * Rendering is delegated to the active GameState. This function is expected
     * to be called once per frame after Update().
     *
     * @param engineContext Engine-wide context.
     */
    void Draw(const EngineContext& engineContext);

    /**
     * @brief Frees the currently active GameState.
     *
     * @details
     * This calls the state's cleanup routines and releases ownership of the
     * current state.
     *
     * @param engineContext Engine-wide context.
     */
    void Free(const EngineContext& engineContext);

    std::unique_ptr<GameState> currentState;
    std::unique_ptr<GameState> nextState;
};
