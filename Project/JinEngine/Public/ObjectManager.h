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

/**
 * @brief Owns and orchestrates the lifecycle, rendering submission, and collision checks of Object instances in a GameState.
 *
 * @details
 * ObjectManager is the central container for Objects inside a GameState.
 *
 * Lifecycle model:
 * - AddObject() does not immediately place the object into the active list. It assigns the tag and moves the object into pendingObjects.
 * - Pending objects are initialized in AddAllPendingObjects():
 *   1) Init(engineContext) is called first (for every pending object).
 *   2) LateInit(engineContext) is called after all pending objects have completed Init().
 *   3) Objects are then moved into objects, registered into objectMap, and added to rawPtrObjects.
 *
 * Per-frame flow:
 * - UpdateAll():
 *   - Updates all alive objects.
 *   - For TEXT objects, requests a font-atlas/mesh sync check before Update().
 *   - Updates SpriteAnimator if present, and keeps Collider scale synchronized to Transform scale.
 *   - Frees and removes dead objects, then initializes any newly pending objects.
 * - DrawAll() / DrawObjects() / DrawObjectsWithTag():
 *   - Submits object lists to RenderManager for batching and draw command generation.
 *
 * Ownership and lookup:
 * - objects and pendingObjects own Object instances via std::unique_ptr.
 * - objectMap maps a tag string to the most recently registered Object* for that tag.
 * - rawPtrObjects stores raw pointers to all active objects for iteration and fast submission.
 *
 * Collision:
 * - CheckCollision() performs broad-phase collision candidate generation using SpatialHashGrid, then filters pairs via collision masks.
 * - Actual narrow-phase collision test is delegated to Collider::CheckCollision() and collision callbacks are invoked via Object::OnCollision().
 */
class ObjectManager
{
    friend GameState;
public:
    /**
     * @brief Enqueues an object to be added to the manager and assigns an optional tag.
     *
     * @details
     * This function:
     * - Asserts the object is not null.
     * - Calls obj->SetTag(tag).
     * - Moves the object into pendingObjects.
     *
     * The object will not be part of objects/rawPtrObjects/objectMap until AddAllPendingObjects() runs
     * (triggered by InitAll() and on every UpdateAll()).
     *
     * @param obj Object to add (ownership is transferred).
     * @param tag Tag string to assign; may be empty.
     * @return Raw pointer to the enqueued object (owned by the manager after the move).
     */
    [[maybe_unused]] Object* AddObject(std::unique_ptr<Object> obj, const std::string& tag = "");

    /**
     * @brief Initializes all currently pending objects.
     *
     * @details
     * Calls AddAllPendingObjects(engineContext) to run Init/LateInit and move pending objects into the active lists.
     *
     * @param engineContext Engine-wide context providing access to managers.
     */
    void InitAll(const EngineContext& engineContext);

    /**
     * @brief Updates all active objects and performs lifecycle maintenance.
     *
     * @details
     * For each alive object in objects:
     * - If the object type is TEXT, calls TextObject::CheckFontAtlasAndMeshUpdate() before Update().
     * - Calls Object::Update(dt, engineContext).
     * - If the object has animation, calls SpriteAnimator::Update(dt).
     * - If the object has a collider, calls Collider::SyncWithTransformScale().
     *
     * After updating:
     * - EraseDeadObjects(engineContext) calls Free/LateFree for dead objects and removes them from tracking containers.
     * - AddAllPendingObjects(engineContext) initializes and activates any objects added during this frame.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Engine-wide context.
     */
    void UpdateAll(float dt, const EngineContext& engineContext);

    /**
     * @brief Submits all active objects to RenderManager for rendering.
     *
     * @details
     * Submits rawPtrObjects to engineContext.renderManager->Submit(...).
     *
     * @param engineContext Engine-wide context providing access to RenderManager.
     */
    void DrawAll(const EngineContext& engineContext);

    /**
     * @brief Submits a provided object list to RenderManager for rendering.
     *
     * @details
     * This does not modify manager ownership or tracking; it only forwards the list to RenderManager.
     *
     * @param engineContext Engine-wide context.
     * @param objects       Object pointers to submit.
     */
    void DrawObjects(const EngineContext& engineContext, const std::vector<Object*>& objects);

    /**
     * @brief Finds objects by tag and submits them to RenderManager for rendering.
     *
     * @details
     * Builds a filtered list using FindByTag(tag, filteredObjects) and submits that list.
     *
     * Note that multiple objects may share the same tag; this function submits all alive matches
     * from rawPtrObjects.
     *
     * @param engineContext Engine-wide context.
     * @param tag           Tag to filter by.
     */
    void DrawObjectsWithTag(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Frees all objects currently owned by the manager and clears internal containers.
     *
     * @details
     * Calls Free(engineContext) then LateFree(engineContext) for:
     * - all active objects
     * - all pending objects
     *
     * Afterwards, clears objects, pendingObjects, objectMap, and rawPtrObjects.
     *
     * @param engineContext Engine-wide context.
     */
    void FreeAll(const EngineContext& engineContext);

    /**
     * @brief Returns one alive object for the given tag if present.
     *
     * @details
     * This checks objectMap first and returns the stored pointer only if it is still alive.
     * If multiple objects share the same tag, this returns whichever objectMap currently points to
     * (typically the most recently activated object with that tag).
     *
     * @param tag Tag to search.
     * @return Alive object pointer, or nullptr if not found or not alive.
     */
    [[nodiscard]] Object* FindByTag(const std::string& tag) const;

    /**
     * @brief Appends all alive objects that match a tag into an output vector.
     *
     * @details
     * Iterates rawPtrObjects and appends entries where:
     * - obj is not null
     * - obj->IsAlive() is true
     * - obj->GetTag() equals the provided tag
     *
     * The result vector is not cleared by this function; it only appends.
     *
     * @param tag    Tag to match.
     * @param result Output list that receives matching objects (appended).
     */
    void FindByTag(const std::string& tag, std::vector<Object*>& result);

    /**
     * @brief Performs broad-phase + narrow-phase collision processing for active objects.
     *
     * @details
     * The collision pipeline is:
     * 1) Clear broadPhaseGrid and insert every Object that currently has a Collider.
     * 2) broadPhaseGrid.ComputeCollisions(...) enumerates candidate pairs.
     * 3) For each candidate pair:
     *    - Applies collision mask/category filtering using:
     *      (a->GetCollisionMask() & b->GetCollisionCategory()) and vice versa.
     *    - Deduplicates pairs using a hash of the ordered Object* pointers.
     *    - Performs narrow-phase by calling a->GetCollider()->CheckCollision(b->GetCollider()).
     *    - If colliding, invokes callbacks a->OnCollision(b) and b->OnCollision(a).
     *
     * This function assumes rawPtrObjects contains active objects and that Collider pointers remain valid
     * for the duration of the call.
     */
    void CheckCollision();

    /**
     * @brief Returns the collision group registry owned by this manager.
     *
     * @details
     * This provides access to the group/category/mask configuration used by objects and colliders.
     *
     * @return Reference to the internal CollisionGroupRegistry.
     */
    [[nodiscard]] CollisionGroupRegistry& GetCollisionGroupRegistry() { return collisionGroupRegistry; }

    /**
     * @brief Returns a snapshot vector of raw pointers to all active objects.
     *
     * @details
     * The returned vector is the manager's internal list. Modifying it externally can break invariants.
     *
     * @return Vector of Object* currently tracked as active.
     */
    [[nodiscard]] std::vector<Object*> GetAllRawPtrObjects() { return rawPtrObjects; }

private:
    /**
     * @brief Initializes all pending objects and moves them into the active containers.
     *
     * @details
     * This function repeatedly drains pendingObjects to handle the case where object initialization
     * spawns additional objects via AddObject().
     *
     * For each drained batch:
     * - Calls Init(engineContext) on every object.
     * - Moves them into a temporary accumulation list.
     *
     * After no pending objects remain:
     * - Calls LateInit(engineContext) on each accumulated object.
     * - Registers the object into objectMap using its tag.
     * - Appends the object's raw pointer into rawPtrObjects.
     * - Moves ownership into objects.
     *
     * @param engineContext Engine-wide context.
     */
    void AddAllPendingObjects(const EngineContext& engineContext);

    /**
     * @brief Frees and removes all objects that are no longer alive.
     *
     * @details
     * This function:
     * - Collects dead Object* from objects (where IsAlive() is false).
     * - Calls Free(engineContext) on each dead object.
     * - Calls LateFree(engineContext) on each dead object.
     * - Removes each dead object from objectMap (by tag) and from rawPtrObjects.
     * - Erases the corresponding std::unique_ptr entries from objects.
     *
     * @param engineContext Engine-wide context.
     */
    void EraseDeadObjects(const EngineContext& engineContext);

    /**
     * @brief Draws collider debug shapes for visible, alive objects.
     *
     * @details
     * Iterates rawPtrObjects, filters to alive+visible objects, and if an object has a Collider,
     * calls Collider::DrawDebug(rm, cam). The actual drawing is performed by RenderManager debug-line
     * submission APIs and is flushed by the renderer.
     *
     * @param rm  RenderManager used for debug draw submission.
     * @param cam Camera used for debug draw projection; may be null depending on collider implementation.
     */
    void DrawColliderDebug(RenderManager* rm, Camera2D* cam);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Object>> pendingObjects;
    std::unordered_map<std::string, Object*> objectMap;
    std::vector<Object*> rawPtrObjects;
    SpatialHashGrid broadPhaseGrid;
    CollisionGroupRegistry collisionGroupRegistry;
};
