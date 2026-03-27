#pragma once
#include "CameraManager.h"
#include "EngineContext.h"
#include "ObjectManager.h"
#include "JinEngine.h"
class StateManager;
struct EngineContext;

/**
 * @brief Base class representing one runtime game state in the engine.
 *
 * @details
 * GameState defines the high-level lifecycle for a playable or transitional state
 * such as a title screen, gameplay scene, pause menu, or loading screen.
 *
 * Each state owns:
 * - an ObjectManager for object lifecycle, update, rendering submission, and collision
 * - a CameraManager for per-state camera storage and active camera selection
 *
 * A GameState is not driven directly by user code through its public lifecycle methods.
 * Instead, StateManager invokes the internal SystemLoad(), SystemInit(), SystemUpdate(),
 * SystemFree(), and SystemUnload() functions, which in turn call the overridable
 * protected callbacks in the correct engine-defined order. :contentReference[oaicite:4]{index=4}
 *
 * The default behavior already provides useful state functionality:
 * - Update() updates all objects through ObjectManager
 * - Draw() submits all objects for rendering through ObjectManager
 * - SystemUpdate() performs collision checks and optional collider debug rendering
 *
 * This allows derived states to override only the parts they need while still
 * keeping the engine lifecycle intact.
 *
 * @note
 * GameState is intended to be subclassed by user-defined states.
 *
 * @note
 * The state owns its ObjectManager and CameraManager directly, so their lifetime
 * matches the lifetime of the state instance.
 */
class GameState
{
    friend StateManager;

public:
    /**
     * @brief Virtual destructor for polymorphic state cleanup.
     */
    virtual ~GameState() = default;

    /**
     * @brief Returns the ObjectManager owned by this state.
     *
     * @details
     * Derived states can use this manager to add, search, update, draw, and remove objects.
     * The default GameState update and draw behavior delegates to this manager.
     *
     * @return Reference to the state's ObjectManager.
     */
    [[nodiscard]] virtual ObjectManager& GetObjectManager() { return objectManager; }

    /**
     * @brief Returns the CameraManager owned by this state.
     *
     * @details
     * Each state maintains its own camera set. The active camera is used by rendering,
     * culling, and debug drawing while the state is running.
     *
     * @return Reference to the state's CameraManager.
     */
    [[nodiscard]] CameraManager& GetCameraManager() { return cameraManager; }

    /**
     * @brief Returns the currently active camera for this state.
     *
     * @details
     * This forwards the request to the state's CameraManager.
     *
     * @return Pointer to the active Camera2D, or nullptr if no valid active camera exists.
     */
    [[nodiscard]] Camera2D* GetActiveCamera() const { return cameraManager.GetActiveCamera(); }

    /**
     * @brief Sets the state's active camera by tag.
     *
     * @details
     * If the requested tag exists in the CameraManager, that camera becomes active.
     * Otherwise, the current active camera remains unchanged.
     *
     * @param tag Camera tag to activate.
     */
    void SetActiveCamera(const std::string& tag) { cameraManager.SetActiveCamera(tag); }

protected:
    /**
     * @brief Called during state initialization before objects are initialized.
     *
     * @details
     * Override this function to perform state-level setup such as:
     * - creating or registering cameras
     * - adding initial objects into ObjectManager
     * - initializing state variables
     *
     * During the engine lifecycle, SystemInit() calls this first, then initializes
     * all pending objects through ObjectManager::InitAll(), and finally calls LateInit().
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void Init([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Called after Init() and after pending objects have completed initialization.
     *
     * @details
     * Override this when post-initialization logic depends on objects already being
     * initialized and inserted into the active object lists.
     *
     * SystemInit() executes this after objectManager.InitAll(engineContext).
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void LateInit([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Called once per frame to update the state.
     *
     * @details
     * The default implementation updates all objects by calling objectManager.UpdateAll(dt, engineContext).
     * That object update flow includes per-object Update(), optional animation advancement,
     * collider transform synchronization, dead object removal, and pending object promotion. :contentReference[oaicite:10]{index=10}
     *
     * Derived states may override this to add custom state logic. When overriding,
     * call GameState::Update(dt, engineContext) if the default object update behavior
     * should still occur.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Provides access to core engine systems.
     */
    virtual void Update(float dt, [[maybe_unused]] const EngineContext& engineContext) { objectManager.UpdateAll(dt, engineContext); }

    /**
     * @brief Called after the main Update phase and after built-in collision/debug processing.
     *
     * @details
     * SystemUpdate() calls Update(), then performs collision checks, optional collider
     * debug rendering, and finally calls LateUpdate(). This makes LateUpdate() a good place
     * for logic that should happen after the regular object update and collision stage.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Provides access to core engine systems.
     */
    virtual void LateUpdate([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Called when the state's resources should be loaded.
     *
     * @details
     * Override this to load or register state-specific resources such as textures,
     * shaders, fonts, sounds, materials, or sprite sheets.
     *
     * StateManager calls SystemLoad() when switching into a new state, before SystemInit(). :contentReference[oaicite:12]{index=12}
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void Load([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Called when the state should submit its visible content for rendering.
     *
     * @details
     * The default implementation submits all state objects through objectManager.DrawAll(engineContext).
     * The actual render pass is performed later by RenderManager when StateManager drives the frame.
     *
     * Override this when the state needs custom drawing order, selective object drawing,
     * UI-only drawing, or additional pre-submission logic.
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void Draw([[maybe_unused]] const EngineContext& engineContext)
    {
        objectManager.DrawAll(engineContext);
    }

    /**
     * @brief Called after the main scene frame has been rendered.
     *
     * @details
     * StateManager::Draw() runs:
     * - RenderManager::BeginFrame()
     * - Draw()
     * - Flush draw commands
     * - optional debug line flush
     * - RenderManager::EndFrame()
     * - PostProcessing()
     * - final draw command flush
     *
     * Override this when the state wants to queue rendering work after the main scene pass,
     * such as fullscreen effects, overlays, or render-texture based post processing.
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void PostProcessing([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Called when the state begins its cleanup phase.
     *
     * @details
     * Override this to release state-owned logic resources or stop state-specific behavior
     * before ObjectManager::FreeAll() is called by SystemFree().
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void Free([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Called when the state's loaded resources should be unloaded.
     *
     * @details
     * Override this to unregister or release resources associated with this state
     * after runtime cleanup has finished.
     *
     * StateManager calls SystemUnload() when leaving the current state. :contentReference[oaicite:16]{index=16}
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void Unload([[maybe_unused]] const EngineContext& engineContext) {}

    /**
     * @brief Restarts the state by running cleanup and initialization again.
     *
     * @details
     * The current implementation calls:
     * - SystemFree(engineContext)
     * - SystemInit(engineContext)
     *
     * This reinitializes the state's object lifecycle without re-running Load() or Unload().
     *
     * @param engineContext Provides access to core engine systems.
     *
     * @note
     * Any resource logic that must be reloaded is the responsibility of the derived state.
     */
    void Restart(const EngineContext& engineContext)
    {
        SystemFree(engineContext);
        SystemInit(engineContext);
    }

    /**
     * @brief Object manager owned by this state.
     *
     * @details
     * Stores active objects, pending objects, tag lookup structures, and collision helpers
     * used during the state lifetime. :contentReference[oaicite:17]{index=17}
     */
    ObjectManager objectManager;

    /**
     * @brief Camera manager owned by this state.
     *
     * @details
     * Stores cameras local to this state, including the active camera used for rendering.
     * The CameraManager constructor creates a default "main" camera automatically. :contentReference[oaicite:18]{index=18}
     */
    CameraManager cameraManager;

private:
    /**
     * @brief Internal state load entry point used by StateManager.
     *
     * @details
     * Forwards directly to the user-overridable Load() callback.
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void SystemLoad(const EngineContext& engineContext)
    {
        Load(engineContext);
    }

    /**
     * @brief Internal state initialization entry point used by StateManager.
     *
     * @details
     * Executes the initialization sequence in the following order:
     * 1. Init(engineContext)
     * 2. objectManager.InitAll(engineContext)
     * 3. LateInit(engineContext)
     *
     * This ensures that state-level setup can enqueue objects first, then all pending
     * objects are initialized, and finally late initialization logic can run with those
     * objects already active.
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void SystemInit(const EngineContext& engineContext)
    {
        Init(engineContext);
        objectManager.InitAll(engineContext);
        LateInit(engineContext);

    }

    /**
     * @brief Internal per-frame update entry point used by StateManager.
     *
     * @details
     * Executes the update sequence in the following order:
     * 1. Update(dt, engineContext)
     * 2. objectManager.CheckCollision()
     * 3. optional objectManager.DrawColliderDebug(...)
     * 4. LateUpdate(dt, engineContext)
     *
     * Debug collider visualization is only queued when the engine indicates that
     * debug drawing is enabled.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Provides access to core engine systems.
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
     * @brief Internal cleanup entry point used by StateManager.
     *
     * @details
     * Executes the cleanup sequence in the following order:
     * 1. Free(engineContext)
     * 2. objectManager.FreeAll(engineContext)
     *
     * This allows derived states to run custom cleanup before the state's objects
     * are fully released.
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void SystemFree(const EngineContext& engineContext)
    {
        Free(engineContext);
        objectManager.FreeAll(engineContext);
    }

    /**
     * @brief Internal unload entry point used by StateManager.
     *
     * @details
     * Forwards directly to the user-overridable Unload() callback.
     *
     * @param engineContext Provides access to core engine systems.
     */
    virtual void SystemUnload(const EngineContext& engineContext)
    {
        Unload(engineContext);
    }
};