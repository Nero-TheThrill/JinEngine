#pragma once
#include "Object.h"

/**
 * @brief Base class for all in-game objects with gameplay behavior.
 *
 * @details
 * GameObject is a concrete specialization of Object that represents
 * entities participating in gameplay logic.
 *
 * It is constructed with ObjectType::GAME and is intended to be managed
 * by ObjectManager within a GameState.
 *
 * All lifecycle functions are provided as overridable hooks and are empty
 * by default, allowing derived classes to implement only the behavior
 * they require.
 */
class GameObject : public Object
{
public:
    /**
     * @brief Constructs a GameObject with GAME object type.
     */
    GameObject() : Object(ObjectType::GAME) {}

    /**
     * @brief Virtual destructor for GameObject.
     */
    ~GameObject() override = default;

    /**
     * @brief Initializes the game object.
     *
     * @details
     * This function is called during the GameState initialization phase,
     * after the GameState Init function and before LateInit.
     */
    void Init([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Performs post-initialization logic for the game object.
     *
     * @details
     * This function is called after all GameObjects have completed Init.
     */
    void LateInit([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Updates the game object every frame.
     *
     * @details
     * This function is called once per frame during the GameState update phase.
     */
    void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Draws the game object.
     *
     * @details
     * This function is called during the rendering phase of the GameState.
     */
    void Draw([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Frees runtime resources owned by the game object.
     *
     * @details
     * This function is called when the GameState is being freed
     * or when the object is explicitly destroyed.
     */
    void Free([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Performs late cleanup logic for the game object.
     *
     * @details
     * This function is called after Free and is intended for
     * dependency-safe cleanup.
     */
    void LateFree([[maybe_unused]] const EngineContext& engineContext) override {}
};
