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
 * @brief Owns, initializes, updates, renders, and removes runtime objects for the active game state.
 *
 * @details
 * ObjectManager is the central container responsible for the lifetime flow of all
 * state-owned Object instances. It manages several parallel views of the same object set:
 *
 * - objects:
 *   The main owning container for active objects.
 *
 * - pendingObjects:
 *   Newly added objects that have not yet completed the manager's initialization flow.
 *
 * - objectMap:
 *   A tag-to-object lookup table used for quick single-object queries.
 *
 * - rawPtrObjects:
 *   A non-owning cache of raw pointers used by rendering, collision checks, and broad iteration.
 *
 * - broadPhaseGrid:
 *   A spatial hash structure used to reduce the cost of collision pair generation.
 *
 * - collisionGroupRegistry:
 *   A tag-to-bit registry used to build collision category and mask values.
 *
 * In the current implementation, AddObject() does not immediately place the object
 * into the active owning list. Instead, it pushes the object into pendingObjects and
 * returns its raw pointer. The actual Init()/LateInit() flow and insertion into
 * objects, objectMap, and rawPtrObjects happen later through AddAllPendingObjects().
 *
 * UpdateAll() is responsible for the main object tick flow. It updates alive objects,
 * performs text-specific font/mesh refresh logic, advances attached sprite animators,
 * synchronizes colliders with transform scale, removes dead objects, and finally
 * integrates pending objects into the active list. :contentReference[oaicite:3]{index=3}
 *
 * Rendering is delegated to RenderManager through Submit(), using either all active
 * raw pointers or filtered subsets. Collision checks are handled separately through
 * CheckCollision(), which first performs broad-phase pair generation with SpatialHashGrid
 * and then applies collision mask/category filtering before calling collider narrow-phase
 * checks and Object::OnCollision().
 *
 * @note
 * This class is closely tied to GameState and acts as one of its internal core systems.
 */
class ObjectManager
{
    friend GameState;

public:
    /**
     * @brief Queues a new object for later initialization and activation.
     *
     * @details
     * This function takes ownership of the supplied object and stores it in
     * pendingObjects rather than directly placing it into the active object list.
     * A raw pointer to the same object is returned immediately so caller code can
     * continue configuring it before or after the next manager update phase.
     *
     * In the current implementation:
     * - the input object must not be null
     * - the tag is assigned immediately through Object::SetTag()
     * - the object is pushed into pendingObjects
     * - actual Init()/LateInit() and active-list insertion happen later in AddAllPendingObjects()
     *
     * Unlike some earlier versions of the system, rawPtrObjects and objectMap are
     * not updated here. That work is deferred until the pending activation step.
     *
     * @param obj Unique ownership of the object to add.
     * @param tag Optional tag assigned to the object before activation.
     * @return Raw pointer to the queued object.
     *
     * @note
     * The returned pointer is non-owning. Lifetime remains controlled by ObjectManager.
     */
    [[maybe_unused]] Object* AddObject(std::unique_ptr<Object> obj, const std::string& tag = "");

    /**
     * @brief Activates all currently pending objects.
     *
     * @details
     * In the current implementation this function simply forwards to
     * AddAllPendingObjects(engineContext), which processes every queued object by:
     * - calling Init()
     * - performing text-specific atlas/mesh refresh if needed
     * - calling LateInit()
     * - inserting the object into objectMap, rawPtrObjects, and objects
     *
     * Because AddAllPendingObjects() loops until pendingObjects becomes empty,
     * it also supports the case where object initialization itself adds more objects.
     *
     * @param engineContext Shared engine services used during object initialization.
     */
    void InitAll(const EngineContext& engineContext);

    /**
     * @brief Updates all active objects, removes dead ones, and activates newly queued ones.
     *
     * @details
     * This is the main per-frame management function for runtime objects.
     *
     * For each active object in objects:
     * - only alive objects are processed
     * - if the object type is TEXT, CheckFontAtlasAndMeshUpdate() is called first
     * - Update(dt, engineContext) is called
     * - if a SpriteAnimator is attached, it is advanced
     * - if a Collider is attached, SyncWithTransformScale() is called
     *
     * After the main loop:
     * - EraseDeadObjects(engineContext) removes dead objects safely
     * - AddAllPendingObjects(engineContext) initializes and activates any newly queued objects
     *
     * This means object creation and object destruction are both resolved through
     * controlled post-update stages rather than during the middle of the main loop. :contentReference[oaicite:7]{index=7}
     *
     * @param dt Delta time for the current frame.
     * @param engineContext Shared engine services used during update.
     */
    void UpdateAll(float dt, const EngineContext& engineContext);

    /**
     * @brief Submits all currently tracked active object pointers to RenderManager.
     *
     * @details
     * This function forwards rawPtrObjects to RenderManager::Submit().
     * The render manager then performs filtering, batching, and later flushes the
     * accumulated draw commands from the state manager's draw flow.
     *
     * @param engineContext Shared engine services used to access RenderManager.
     */
    void DrawAll(const EngineContext& engineContext);

    /**
     * @brief Submits a caller-provided subset of objects to RenderManager.
     *
     * @details
     * This is a convenience function for rendering a filtered object list without
     * requiring the caller to interact with RenderManager directly.
     *
     * @param engineContext Shared engine services used to access RenderManager.
     * @param objects Non-owning list of object pointers to submit for rendering.
     */
    void DrawObjects(const EngineContext& engineContext, const std::vector<Object*>& objects);

    /**
     * @brief Finds all alive objects with a matching tag and submits them for rendering.
     *
     * @details
     * This function builds a temporary filtered list by calling the multi-result
     * FindByTag() overload and then submits that list to RenderManager.
     *
     * Unlike the single-result FindByTag() overload, this version is intended for
     * rendering or processing all matching objects rather than only one representative. :contentReference[oaicite:9]{index=9}
     *
     * @param engineContext Shared engine services used to access RenderManager.
     * @param tag Object tag to filter by.
     */
    void DrawObjectsWithTag(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Frees and clears every active and pending object owned by the manager.
     *
     * @details
     * In the current implementation, this function performs cleanup in two phases:
     *
     * 1. Calls Free(engineContext) for every object in:
     *    - objects
     *    - pendingObjects
     *
     * 2. Calls LateFree(engineContext) for every object in:
     *    - objects
     *    - pendingObjects
     *
     * After cleanup, all internal containers are cleared, including the active
     * owning list, the pending list, the tag map, and the raw pointer cache. :contentReference[oaicite:10]{index=10}
     *
     * @param engineContext Shared engine services used during cleanup.
     */
    void FreeAll(const EngineContext& engineContext);

    /**
     * @brief Finds one alive object associated with the given tag.
     *
     * @details
     * This function performs a fast lookup through objectMap. It returns the mapped
     * object only if:
     * - the tag exists in the map
     * - the mapped object is still alive
     *
     * In the current implementation, objectMap stores one representative pointer
     * per tag. During dead-object removal, if the stored pointer dies and another
     * alive object with the same tag exists, the map entry may be replaced with
     * that surviving object instead of being erased immediately. :contentReference[oaicite:11]{index=11}
     *
     * @param tag Tag to search for.
     * @return Pointer to one alive matching object, or nullptr if none is available.
     */
    [[nodiscard]] Object* FindByTag(const std::string& tag) const;

    /**
     * @brief Appends all alive objects with the given tag into the provided output vector.
     *
     * @details
     * This overload scans rawPtrObjects and appends every pointer that:
     * - is non-null
     * - is alive
     * - has a matching Object::GetTag() value
     *
     * It does not clear the destination vector before appending.
     *
     * @param tag Tag to search for.
     * @param result Output vector that receives matching object pointers.
     */
    void FindByTag(const std::string& tag, std::vector<Object*>& result);

    /**
     * @brief Runs the engine's collision detection flow for all active collidable objects.
     *
     * @details
     * The current collision process works as follows:
     *
     * 1. Create a checkedPairs set to avoid duplicate pair processing.
     * 2. Clear the broadPhaseGrid.
     * 3. Insert every object that has a non-null collider into the spatial grid.
     * 4. Ask the grid to generate candidate pairs.
     * 5. For each pair:
     *    - reject it if collision mask/category bits do not match
     *    - normalize pointer order for stable pair-key generation
     *    - skip it if already processed
     *    - run collider narrow-phase CheckCollision()
     *    - if colliding, call OnCollision() on both objects
     *
     * The broad-phase grid itself uses a spatial hash and treats very large objects
     * specially so they can still be paired against other objects without exploding
     * per-cell insertion cost.
     */
    void CheckCollision();

    /**
     * @brief Returns the collision group registry used by objects to build category and mask bits.
     *
     * @details
     * Object::SetCollision() uses this registry to convert string group names into
     * unique bit values. New groups are assigned incrementally until the 32-bit limit
     * is reached.
     *
     * @return Mutable reference to the collision group registry.
     */
    [[nodiscard]] CollisionGroupRegistry& GetCollisionGroupRegistry() { return collisionGroupRegistry; }

    /**
     * @brief Returns the manager's cached list of raw object pointers.
     *
     * @details
     * This is a convenience snapshot-like return of the current rawPtrObjects vector.
     * It is used by other systems such as RenderManager when checking whether resources
     * are still referenced by objects before allowing unregistration. :contentReference[oaicite:14]{index=14}
     *
     * @return Copy of the current raw object pointer list.
     */
    [[nodiscard]] std::vector<Object*> GetAllRawPtrObjects() { return rawPtrObjects; }

private:
    /**
     * @brief Initializes and activates all currently pending objects.
     *
     * @details
     * This is the internal activation pipeline for newly queued objects.
     *
     * The current implementation repeatedly drains pendingObjects until it becomes empty.
     * This allows object initialization code to enqueue additional objects without losing them.
     *
     * For each drained object:
     * - Init(engineContext) is called first
     * - if the object type is TEXT, CheckFontAtlasAndMeshUpdate() is called immediately after Init()
     * - the object is collected into a temporary activation list
     *
     * After all pending waves are collected:
     * - LateInit(engineContext) is called
     * - objectMap[obj->GetTag()] is updated
     * - rawPtrObjects receives the raw pointer
     * - objects receives the unique_ptr ownership
     *
     * This delayed activation model keeps the main active list stable while new
     * objects are being prepared.
     *
     * @param engineContext Shared engine services used during initialization.
     */
    void AddAllPendingObjects(const EngineContext& engineContext);

    /**
     * @brief Cleans up and removes all dead objects from the active containers.
     *
     * @details
     * This function first collects pointers to every object whose IsAlive() is false.
     * It then performs removal in ordered phases:
     *
     * 1. Call Free(engineContext) on each dead object.
     * 2. Call LateFree(engineContext) on each dead object.
     * 3. Update objectMap:
     *    - if the tag entry points to the dead object,
     *      search rawPtrObjects for another alive object with the same tag
     *    - replace the map entry if a surviving match exists
     *    - otherwise erase the tag entry
     * 4. Remove dead raw pointers from rawPtrObjects.
     * 5. Remove dead unique_ptr entries from objects.
     *
     * This replacement behavior matters when multiple objects share the same tag
     * and the current mapped representative is the one being removed.
     *
     * @param engineContext Shared engine services used during cleanup.
     */
    void EraseDeadObjects(const EngineContext& engineContext);

    /**
     * @brief Draws collider debug geometry for every alive and visible object that has a collider.
     *
     * @details
     * This helper iterates over rawPtrObjects and skips objects that are not alive
     * or not visible. For the remaining objects, if a collider exists, it forwards
     * debug rendering to Collider::DrawDebug().
     *
     * This is an internal helper and is not part of the normal public rendering path. :contentReference[oaicite:17]{index=17}
     *
     * @param rm Render manager used by collider debug draw routines.
     * @param cam Camera used when drawing debug collider shapes.
     */
    void DrawColliderDebug(RenderManager* rm, Camera2D* cam);

    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Object>> pendingObjects;
    std::unordered_map<std::string, Object*> objectMap;
    std::vector<Object*> rawPtrObjects;
    SpatialHashGrid broadPhaseGrid;
    CollisionGroupRegistry collisionGroupRegistry;
};