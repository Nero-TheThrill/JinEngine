#pragma once
#include "Object.h"

/**
 * @brief Convenience base class for gameplay-oriented objects.
 *
 * @details
 * GameObject is a lightweight derived class of Object that represents a standard
 * gameplay object type. Its constructor initializes the base Object with
 * ObjectType::GAME, and it provides empty default implementations for the full
 * object lifecycle:
 *
 * - Init()
 * - LateInit()
 * - Update()
 * - Draw()
 * - Free()
 * - LateFree()
 *
 * This allows users to derive from GameObject and override only the callbacks
 * they actually need, without being forced to implement every pure virtual
 * function from Object manually.
 *
 * In the engine update flow, these lifecycle functions are called by ObjectManager
 * as objects are added, updated, drawn, and removed.
 *
 * @note
 * This class does not add new state of its own. It mainly exists as a user-facing
 * convenience layer above Object.
 */
class GameObject : public Object
{
public:
    /**
     * @brief Constructs a gameplay object.
     *
     * @details
     * Initializes the base Object with ObjectType::GAME so the engine can
     * classify this instance as a standard gameplay object.
     */
    GameObject() : Object(ObjectType::GAME) {}

    /**
     * @brief Virtual destructor.
     *
     * @details
     * Uses the default implementation because GameObject itself does not own
     * additional resources beyond those managed by its base class and derived classes.
     */
    ~GameObject() override = default;

    /**
     * @brief Called when the object is first initialized.
     *
     * @details
     * This default implementation does nothing.
     * Override this function in derived classes to perform one-time setup such as:
     * - binding meshes or materials
     * - initializing gameplay state
     * - attaching colliders or animation data
     *
     * This function is invoked by ObjectManager when pending objects are promoted
     * into the active object lifecycle. :contentReference[oaicite:2]{index=2}
     *
     * @param engineContext Provides access to core engine systems.
     */
    void Init([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Called after the initial Init phase has completed.
     *
     * @details
     * This default implementation does nothing.
     * Override this when the object needs post-initialization logic that should run
     * after all pending objects in the current batch have completed Init().
     *
     * This is useful when initialization depends on other objects already being present
     * in the manager. :contentReference[oaicite:3]{index=3}
     *
     * @param engineContext Provides access to core engine systems.
     */
    void LateInit([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Updates the object once per frame.
     *
     * @details
     * This default implementation does nothing.
     * Override this function to implement gameplay logic, movement, state transitions,
     * input-driven behavior, timers, AI, or other per-frame processing.
     *
     * During the engine update loop, ObjectManager calls Update() for alive objects.
     * Additional systems such as animation updates and collider scale synchronization
     * may also be performed around this stage by the engine.
     *
     * @param dt Delta time in seconds for the current frame.
     * @param engineContext Provides access to core engine systems.
     */
    void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Allows the object to prepare rendering-related state before submission.
     *
     * @details
     * This default implementation does nothing.
     * Override this function when the object needs to modify rendering parameters
     * immediately before draw submission, such as material uniforms, animation-driven
     * data, or custom visual behavior.
     *
     * Actual rendering submission is handled through ObjectManager and RenderManager,
     * but this callback gives each object a chance to participate in that flow.
     *
     * @param engineContext Provides access to core engine systems.
     */
    void Draw([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Called when the object begins its destruction or removal phase.
     *
     * @details
     * This default implementation does nothing.
     * Override this function to release gameplay-side resources, detach state,
     * stop sounds, or perform cleanup logic before LateFree() is called.
     *
     * ObjectManager calls Free() for dead objects before performing their final
     * removal from internal storage. :contentReference[oaicite:6]{index=6}
     *
     * @param engineContext Provides access to core engine systems.
     */
    void Free([[maybe_unused]] const EngineContext& engineContext) override {}

    /**
     * @brief Called after Free() during the final cleanup stage.
     *
     * @details
     * This default implementation does nothing.
     * Override this function for cleanup work that should happen after the main
     * Free() stage, especially when destruction order relative to other objects matters.
     *
     * This is the last lifecycle callback before the object is removed from
     * ObjectManager's internal containers. :contentReference[oaicite:7]{index=7}
     *
     * @param engineContext Provides access to core engine systems.
     */
    void LateFree([[maybe_unused]] const EngineContext& engineContext) override {}
};