#include "Engine.h"

#include <cassert>
#include <algorithm>
#include <unordered_set>

Object* ObjectManager::AddObject(std::unique_ptr<Object> obj, const std::string& tag)
{
    assert(obj != nullptr && "Cannot add null object");

    obj->SetTag(tag);

    Object* returnVal = obj.get();
    pendingObjects.push_back(std::move(obj));
    return returnVal;
}

void ObjectManager::InitAll(const EngineContext& engineContext)
{
    AddAllPendingObjects(engineContext);
}

void ObjectManager::UpdateAll(float dt, const EngineContext& engineContext)
{
    for (const auto& obj : objects)
    {
        if (obj->IsAlive())
        {
            if (obj->GetType()==ObjectType::TEXT)
            {
                static_cast<TextObject*>(obj.get())->CheckFontAtlasAndMeshUpdate();
            }
            obj->Update(dt, engineContext);
            if (obj->HasAnimation())
                obj->GetSpriteAnimator()->Update(dt);
            if (Collider* col = obj->GetCollider())
                col->SyncWithTransformScale();
        }
    }

    EraseDeadObjects(engineContext);
    AddAllPendingObjects(engineContext);
}

void ObjectManager::AddAllPendingObjects(const EngineContext& engineContext)
{
    std::vector<std::unique_ptr<Object>> allPendingObjects;

    while (!pendingObjects.empty())
    {
        std::vector<std::unique_ptr<Object>> tmparr;
        std::swap(tmparr, pendingObjects);
        for (auto& obj : tmparr)
        {
            obj->Init(engineContext);
            if (obj->GetType() == ObjectType::TEXT)
            {
                static_cast<TextObject*>(obj.get())->CheckFontAtlasAndMeshUpdate();
            }
        }
        for (auto& obj : tmparr)
        {
            allPendingObjects.push_back(std::move(obj));
        }
        tmparr.clear();
    }
    for (auto& obj : allPendingObjects)
    {
        obj->LateInit(engineContext);
        Object* rawPointer = obj.get();
        objectMap[obj->GetTag()] = rawPointer;
        rawPtrObjects.push_back(rawPointer);
        objects.push_back(std::move(obj));
    }
}

void ObjectManager::EraseDeadObjects(const EngineContext& engineContext)
{
    std::vector<Object*> deadObjects;
    for (const auto& obj : objects)
    {
        if (!obj->IsAlive())
            deadObjects.push_back(obj.get());
    }

    for (auto& obj : deadObjects)
        obj->Free(engineContext);

	for (auto& obj : deadObjects)
    {
        obj->LateFree(engineContext);

        const std::string tag = obj->GetTag();

        auto it = objectMap.find(tag);
        if (it != objectMap.end() && it->second == obj)
        {
            Object* replacement = nullptr;

            for (Object* candidate : rawPtrObjects)
            {
                if (candidate == nullptr) continue;
                if (candidate == obj) continue;
                if (!candidate->IsAlive()) continue;
                if (candidate->GetTag() != tag) continue;

                replacement = candidate;
                break;
            }

            if (replacement)
            {
                it->second = replacement;
            }
            else
            {
                objectMap.erase(it);
            }
        }

        rawPtrObjects.erase(std::remove(rawPtrObjects.begin(), rawPtrObjects.end(), obj), rawPtrObjects.end());
    }

    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [&](const std::unique_ptr<Object>& obj)
            {
                return std::find(deadObjects.begin(), deadObjects.end(), obj.get()) != deadObjects.end();
            }),
        objects.end());
}

void ObjectManager::DrawAll(const EngineContext& engineContext)
{
    engineContext.renderManager->Submit(rawPtrObjects, engineContext);
}

void ObjectManager::DrawObjects(const EngineContext& engineContext, const std::vector<Object*>& objects)
{
    engineContext.renderManager->Submit(objects, engineContext);
}

void ObjectManager::DrawObjectsWithTag(const EngineContext& engineContext, const std::string& tag)
{
    std::vector<Object*> filteredObjects;
    FindByTag(tag, filteredObjects);
    engineContext.renderManager->Submit(filteredObjects, engineContext);
}

void ObjectManager::FreeAll(const EngineContext& engineContext)
{
    for (const auto& obj : objects)
        obj->Free(engineContext);
    for (const auto& obj : pendingObjects)
        obj->Free(engineContext);
    
    for (const auto& obj : objects)
        obj->LateFree(engineContext);
    for (const auto& obj : pendingObjects)
        obj->LateFree(engineContext);


    objects.clear();
    objectMap.clear();
    rawPtrObjects.clear();
    pendingObjects.clear();
}

Object* ObjectManager::FindByTag(const std::string& tag) const
{
    auto it = objectMap.find(tag);
    if (it != objectMap.end() && it->second->IsAlive())
        return it->second;
    return nullptr;
}

void ObjectManager::FindByTag(const std::string& tag, std::vector<Object*>& result)
{
    for (Object* obj : rawPtrObjects)
    {
        if (obj && obj->IsAlive() && obj->GetTag() == tag)
            result.push_back(obj);
    }
}
void ObjectManager::CheckCollision()
{
    std::unordered_set<uint64_t> checkedPairs;
    checkedPairs.reserve(2000);

    broadPhaseGrid.Clear();

    for (Object* obj : rawPtrObjects)
    {
        Collider* collider = obj->GetCollider();
        if (collider)
            broadPhaseGrid.Insert(obj);
    }

    broadPhaseGrid.ComputeCollisions([&](Object* a, Object* b)
        {
            if ((a->GetCollisionMask() & b->GetCollisionCategory()) == 0 ||
                (b->GetCollisionMask() & a->GetCollisionCategory()) == 0)
                return;

            if (a > b) std::swap(a, b);
            uint64_t key = std::hash<Object*>()(a) ^ (std::hash<Object*>()(b) << 1);

            if (checkedPairs.find(key) != checkedPairs.end())
                return;

            checkedPairs.insert(key);

            if (a->GetCollider()->CheckCollision(b->GetCollider()))
            {
                a->OnCollision(b);
                b->OnCollision(a);
            }
        });
}

void ObjectManager::DrawColliderDebug(RenderManager* rm, Camera2D* cam)
{
    for (auto* obj : rawPtrObjects)
    {
        if (!obj->IsAlive() || !obj->IsVisible()) continue;

        if (Collider* col = obj->GetCollider())
        {
            col->DrawDebug(rm, cam);
        }
    }
}