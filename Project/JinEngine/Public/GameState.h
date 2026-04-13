#pragma once
#include "CameraManager.h"
#include "EngineContext.h"
#include "ObjectManager.h"
#include "JinEngine.h"
class StateManager;
struct EngineContext;

class GameState
{
    friend StateManager;

public:
    virtual ~GameState() = default;

    [[nodiscard]] virtual ObjectManager& GetObjectManager() { return objectManager; }

    [[nodiscard]] CameraManager& GetCameraManager() { return cameraManager; }

    [[nodiscard]] Camera2D* GetActiveCamera() const { return cameraManager.GetActiveCamera(); }

    void SetActiveCamera(const std::string& tag) { cameraManager.SetActiveCamera(tag); }

protected:
    virtual void Init([[maybe_unused]] const EngineContext& engineContext) {}

    virtual void LateInit([[maybe_unused]] const EngineContext& engineContext) {}

    virtual void Update(float dt, [[maybe_unused]] const EngineContext& engineContext) { objectManager.UpdateAll(dt, engineContext); }

    virtual void LateUpdate([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) {}

    virtual void Load([[maybe_unused]] const EngineContext& engineContext) {}

    virtual void Draw([[maybe_unused]] const EngineContext& engineContext)
    {
        objectManager.DrawAll(engineContext);
    }

    virtual void PostProcessing([[maybe_unused]] const EngineContext& engineContext) {}

    virtual void Free([[maybe_unused]] const EngineContext& engineContext) {}

    virtual void Unload([[maybe_unused]] const EngineContext& engineContext) {}

    void Restart(const EngineContext& engineContext)
    {
        SystemFree(engineContext);
        SystemInit(engineContext);
    }

    ObjectManager objectManager;
    CameraManager cameraManager;

private:
    virtual void SystemLoad(const EngineContext& engineContext)
    {
        Load(engineContext);
    }

    virtual void SystemInit(const EngineContext& engineContext)
    {
        Init(engineContext);
        objectManager.InitAll(engineContext);
        LateInit(engineContext);
   
    }

    virtual void SystemUpdate(float dt, const EngineContext& engineContext)
    {
        Update(dt, engineContext);

        objectManager.CheckCollision(engineContext);
        if (engineContext.engine->ShouldRenderDebugDraws())
            objectManager.DrawColliderDebug(engineContext.renderManager, cameraManager.GetActiveCamera());

        LateUpdate(dt, engineContext);
    }

    virtual void SystemFree(const EngineContext& engineContext)
    {
        Free(engineContext);
        objectManager.FreeAll(engineContext);
    }

    virtual void SystemUnload(const EngineContext& engineContext)
    {
        Unload(engineContext);
    }
};
