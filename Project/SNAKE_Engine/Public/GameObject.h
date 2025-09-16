#pragma once
#include "Object.h"

/**
 * @brief Default implementation of a basic in-game object.
 *
 * @details
 * GameObject is a concrete subclass of Object with type = GAME.
 * It provides empty overrides for all lifecycle methods
 * (Init, Update, Draw, Free, etc.), making it suitable as a
 * lightweight placeholder or a base for user-defined objects.
 *
 * Typical usage:
 * - Inherit from GameObject to create your own gameplay objects.
 * - Override only the lifecycle functions you need.
 *
 * @code
 * class Player : public GameObject {
 * public:
 *     void Init(const EngineContext& ctx) override {
 *         SetTag("[Object]Player");
 *         SetMaterial(ctx, "[Mat]player");
 *     }
 *     void Update(float dt, const EngineContext& ctx) override {
 *         // Player-specific logic
 *     }
 * };
 * @endcode
 */
class GameObject : public Object
{
public:
    /**
     * @brief Constructs a GameObject with type GAME.
     */
    GameObject() : Object(ObjectType::GAME) {}

    /**
     * @brief Virtual destructor.
     */
    ~GameObject() override = default;

    /**
     * @brief Called once when the object is initialized.
     */
    void Init([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Called after Init() for deferred setup.
     */
    void LateInit([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Per-frame update function.
     *
     * @param dt Delta time in seconds.
     */
    void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Per-frame draw function.
     */
    void Draw([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Cleanup called before the object is destroyed.
     */
    void Free([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Late cleanup after Free() for cross-object dependencies.
     */
    void LateFree([[maybe_unused]] const EngineContext& engineContext) override {}
};
