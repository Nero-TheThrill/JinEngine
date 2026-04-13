#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

#include "RenderManager.h"

class GameState;
class Object;
struct EngineContext;
class Camera2D;

class ObjectManager
{
    friend GameState;
public:
    [[maybe_unused]]Object* AddObject(std::unique_ptr<Object> obj, const std::string& tag = "");
    void InitAll(const EngineContext& engineContext);
    void UpdateAll(float dt, const EngineContext& engineContext);
    void DrawAll(const EngineContext& engineContext);
    void DrawObjects(const EngineContext& engineContext, const std::vector<Object*>& objects);
    void DrawObjectsWithTag(const EngineContext& engineContext, const std::string& tag);

    void FreeAll(const EngineContext& engineContext);

    [[nodiscard]] Object* FindByTag(const std::string& tag) const;
    void FindByTag(const std::string& tag, std::vector<Object*>& result);
    void CheckCollision(const EngineContext& engineContext);

    [[nodiscard]] CollisionGroupRegistry& GetCollisionGroupRegistry() { return collisionGroupRegistry; }

    [[nodiscard]] std::vector<Object*> GetAllRawPtrObjects() { return rawPtrObjects; }

private:
    void AddAllPendingObjects(const EngineContext& engineContext);
    void EraseDeadObjects(const EngineContext& engineContext);
    void DrawColliderDebug(RenderManager* rm, Camera2D* cam);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Object>> pendingObjects;
    std::unordered_map<std::string, Object*> objectMap;
    std::vector<Object*> rawPtrObjects;
    SpatialHashGrid broadPhaseGrid;
    CollisionGroupRegistry collisionGroupRegistry;
};
