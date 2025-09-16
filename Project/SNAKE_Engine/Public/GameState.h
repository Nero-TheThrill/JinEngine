#pragma once
#include "CameraManager.h"
#include "EngineContext.h"
#include "ObjectManager.h"
#include "SNAKE_Engine.h"
class StateManager;
struct EngineContext;

/**
 * @brief Abstract base class representing a game state in the engine.
 *
 * @details
 * GameState defines the lifecycle and update/draw pipeline of a state
 * (e.g., title screen, main menu, gameplay). It owns its own
 * ObjectManager and CameraManager, and exposes overridable hooks for
 * initialization, updating, drawing, and cleanup.
 *
 * The engine calls internal System* functions to guarantee correct
 * ordering of Init, Update, Collision checks, Draw, and Free steps.
 * Derived classes should override the protected virtual hooks (Init,
 * Update, Draw, etc.) rather than calling System* directly.
 *
 * Typical usage:
 * @code
 * class PlayState : public GameState {
 * protected:
 *     void Load(const EngineContext& engineContext) override {
 *         // Load textures, shaders, sounds
 *     }
 *     void Init(const EngineContext& engineContext) override {
 *         // Spawn player and enemies
 *     }
 *     void Update(float dt, const EngineContext& engineContext) override {
 *         GameState::Update(dt, engineContext); // updates objects
 *         // Extra game logic
 *     }
 * };
 * @endcode
 */
class GameState
{
    friend StateManager;

public:
    virtual ~GameState() = default;

    /**
     * @brief Provides access to this state's ObjectManager.
     */
    [[nodiscard]] virtual ObjectManager& GetObjectManager() { return objectManager; }

    /**
     * @brief Provides access to this state's CameraManager.
     */
    [[nodiscard]] CameraManager& GetCameraManager() { return cameraManager; }

    /**
     * @brief Returns the currently active camera.
     */
    [[nodiscard]] Camera2D* GetActiveCamera() const { return cameraManager.GetActiveCamera(); }

    /**
     * @brief Sets the active camera by tag.
     *
     * @param tag Camera tag registered in CameraManager.
     */
    void SetActiveCamera(const std::string& tag) { cameraManager.SetActiveCamera(tag); }

protected:
    /**
     * @brief Initialization hook for derived states.
     *
     * @param engineContext Engine subsystems context.
     */
    virtual void Init([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Late initialization hook after Init().
     */
    virtual void LateInit([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Per-frame update hook.
     *
     * @details
     * Default implementation updates all objects via ObjectManager.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Engine context.
     */
    virtual void Update(float dt, [[maybe_unused]] const EngineContext& engineContext) { objectManager.UpdateAll(dt, engineContext); }

    /**
     * @brief Post-update hook called after Update().
     */
    virtual void LateUpdate([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Load resources needed by this state.
     */
    virtual void Load([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Per-frame draw hook.
     *
     * @details
     * Default implementation draws all objects via ObjectManager.
     */
    virtual void Draw([[maybe_unused]] const EngineContext& engineContext)
    {
        objectManager.DrawAll(engineContext);
    }

    /**
     * @brief Post-processing pass hook.
     *
     * @details
     * Called after all objects have been drawn. Override to apply
     * fullscreen effects or post-render logic.
     */
    virtual void PostProcessing([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Cleanup hook before objects are destroyed.
     */
    virtual void Free([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Resource unload hook (called before state exit).
     */
    virtual void Unload([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Restarts the state by calling Free then Init.
     *
     * @param engineContext Engine context.
     */
    void Restart(const EngineContext& engineContext)
    {
        SystemFree(engineContext);
        SystemInit(engineContext);
    }

    ObjectManager objectManager; ///< Per-state object manager.
    CameraManager cameraManager; ///< Per-state camera manager.

private:
    /**
     * @brief Internal: calls Load().
     */
    virtual void SystemLoad(const EngineContext& engineContext)
    {
        Load(engineContext);
    }

    /**
     * @brief Internal: orchestrates Init, ObjectManager InitAll, LateInit.
     */
    virtual void SystemInit(const EngineContext& engineContext)
    {
        Init(engineContext);
        objectManager.InitAll(engineContext);
        LateInit(engineContext);
        objectManager.AddAllPendingObjects(engineContext);
    }

    /**
     * @brief Internal: orchestrates Update, collision checks, debug draw, LateUpdate.
     */
    virtual void SystemUpdate(float dt, const EngineContext& engineContext)
    {
        Update(dt, engineContext);

        objectManager.CheckCollision();
        if (engineContext.engine->ShouldRenderDebugDraws())
            objectManager.DrawColliderDebug(engineContext.renderManager, cameraManager.GetActiveCamera());

        LateUpdate(dt, engineContext);
    }

    /**
     * @brief Internal: orchestrates Free and ObjectManager FreeAll.
     */
    virtual void SystemFree(const EngineContext& engineContext)
    {
        Free(engineContext);
        objectManager.FreeAll(engineContext);
    }

    /**
     * @brief Internal: calls Unload().
     */
    virtual void SystemUnload(const EngineContext& engineContext)
    {
        Unload(engineContext);
    }
};
