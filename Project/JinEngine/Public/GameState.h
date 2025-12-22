#pragma once
#include "CameraManager.h"
#include "EngineContext.h"
#include "ObjectManager.h"
#include "JinEngine.h"

class StateManager;
struct EngineContext;

/**
 * @brief Base class representing a single game state.
 *
 * @details
 * GameState defines the lifecycle and execution flow of a game state managed
 * by StateManager. It owns an ObjectManager and a CameraManager, and provides
 * overridable hooks for loading, initialization, updating, rendering, and cleanup.
 *
 * User-defined game states should inherit from this class and override
 * the protected virtual functions such as Load, Init, Update, Draw, and Free.
 *
 * The System-prefixed functions are called exclusively by StateManager
 * and enforce a strict execution order.
 */
class GameState
{
    friend StateManager;

public:
    /**
     * @brief Virtual destructor for GameState.
     */
    virtual ~GameState() = default;

    /**
     * @brief Returns the ObjectManager owned by this game state.
     *
     * @return Reference to the internal ObjectManager.
     */
    [[nodiscard]] virtual ObjectManager& GetObjectManager() { return objectManager; }

    /**
     * @brief Returns the CameraManager owned by this game state.
     *
     * @return Reference to the internal CameraManager.
     */
    [[nodiscard]] CameraManager& GetCameraManager() { return cameraManager; }

    /**
     * @brief Returns the currently active Camera2D.
     *
     * @return Pointer to the active camera, or nullptr if none is active.
     */
    [[nodiscard]] Camera2D* GetActiveCamera() const { return cameraManager.GetActiveCamera(); }

    /**
     * @brief Sets the active camera using a registered camera tag.
     *
     * @param tag String identifier of the camera to activate.
     */
    void SetActiveCamera(const std::string& tag) { cameraManager.SetActiveCamera(tag); }

protected:
    /**
     * @brief Performs state-specific initialization logic.
     *
     * @details
     * This function is called before ObjectManager::InitAll.
     * It is intended for setting up state-level data.
     */
    virtual void Init([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Performs post-object initialization logic.
     *
     * @details
     * This function is called after all objects have been initialized.
     */
    virtual void LateInit([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Updates the game state.
     *
     * @details
     * The default implementation updates all objects managed
     * by the ObjectManager.
     */
    virtual void Update(float dt, [[maybe_unused]] const EngineContext& engineContext)
    {
        objectManager.UpdateAll(dt, engineContext);
    }

    /**
     * @brief Performs late update logic after collision checks.
     */
    virtual void LateUpdate([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Loads state-specific resources.
     *
     * @details
     * This function is typically used to register shaders, textures,
     * and other shared resources.
     */
    virtual void Load([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Draws all objects belonging to this game state.
     *
     * @details
     * The default implementation delegates rendering to ObjectManager.
     */
    virtual void Draw([[maybe_unused]] const EngineContext& engineContext)
    {
        objectManager.DrawAll(engineContext);
    }

    /**
     * @brief Applies post-processing effects.
     */
    virtual void PostProcessing([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Frees state-specific runtime resources.
     */
    virtual void Free([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Unloads persistent state resources.
     */
    virtual void Unload([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Restarts the game state.
     *
     * @details
     * This function frees the current state and reinitializes it
     * using the same EngineContext.
     */
    void Restart(const EngineContext& engineContext)
    {
        SystemFree(engineContext);
        SystemInit(engineContext);
    }

    ObjectManager objectManager;
    CameraManager cameraManager;

private:
    /**
     * @brief Internal load phase entry point.
     *
     * @details
     * Calls the user-defined Load function.
     */
    virtual void SystemLoad(const EngineContext& engineContext)
    {
        Load(engineContext);
    }

    /**
     * @brief Internal initialization phase entry point.
     *
     * @details
     * Executes Init, initializes all objects,
     * then executes LateInit.
     */
    virtual void SystemInit(const EngineContext& engineContext)
    {
        Init(engineContext);
        objectManager.InitAll(engineContext);
        LateInit(engineContext);
    }

    /**
     * @brief Internal update phase entry point.
     *
     * @details
     * Executes Update, performs collision checks,
     * optionally renders collider debug visuals,
     * then executes LateUpdate.
     */
    virtual void SystemUpdate(float dt, const EngineContext& engineContext)
    {
        Update(dt, engineContext);

        objectManager.CheckCollision();

        if (engineContext.engine->ShouldRenderDebugDraws())
            objectManager.DrawColliderDebug(
                engineContext.renderManager,
                cameraManager.GetActiveCamera()
            );

        LateUpdate(dt, engineContext);
    }

    /**
     * @brief Internal free phase entry point.
     *
     * @details
     * Executes Free and releases all objects.
     */
    virtual void SystemFree(const EngineContext& engineContext)
    {
        Free(engineContext);
        objectManager.FreeAll(engineContext);
    }

    /**
     * @brief Internal unload phase entry point.
     *
     * @details
     * Executes Unload for persistent resource cleanup.
     */
    virtual void SystemUnload(const EngineContext& engineContext)
    {
        Unload(engineContext);
    }
};
