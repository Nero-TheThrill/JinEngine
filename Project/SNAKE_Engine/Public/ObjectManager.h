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
 * @brief Owns, updates, draws, and queries scene objects.
 *
 * @details
 * ObjectManager stores objects as unique ownership, supports deferred addition
 * via a pending list, keeps raw-pointer views for fast iteration, and exposes
 * tag-based lookup. It also integrates a broad-phase spatial hash for
 * collision checks and provides helper rendering entry points that batch
 * draw calls through RenderManager.
 *
 * Typical frame flow:
 *  - AddObject() queues new objects into @ref pendingObjects.
 *  - UpdateAll() calls @ref AddAllPendingObjects(), updates objects, and
 *    performs collision checks.
 *  - DrawAll() builds a render submission from @ref rawPtrObjects (or a
 *    filtered subset) and flushes through RenderManager.
 *  - EraseDeadObjects() removes killed objects and updates indices/maps.
 */
class ObjectManager
{
    friend GameState;
public:
    /**
     * @brief Adds a new object and returns a raw pointer to it.
     *
     * @details
     * The object is not immediately part of the active @ref objects array.
     * It is pushed to @ref pendingObjects and becomes active on the next call
     * to @ref AddAllPendingObjects(). If a non-empty tag is provided, a tag->object
     * mapping is inserted/updated in @ref objectMap.
     *
     * @param obj Unique ownership of the object to add. Must not be nullptr.
     * @param tag Optional tag used for later lookup (default ="").
     * @return A raw pointer to the newly added object (stable while the object lives).
     * @code
     * auto* enemy = objectManager.AddObject(std::make_unique<Enemy>(), "[Object]Enemy01");
     * @endcode
     */
    [[maybe_unused]] Object* AddObject(std::unique_ptr<Object> obj, const std::string& tag = "");

    /**
     * @brief Initializes all active objects.
     *
     * @details
     * Ensures pending additions are integrated via @ref AddAllPendingObjects(),
     * then calls each object's Init() followed by LateInit(). This is typically
     * invoked once after a state is loaded to bring all objects to a ready state.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    void InitAll(const EngineContext& engineContext);

    /**
     * @brief Updates every active object and processes collisions.
     *
     * @details
     * 1) Integrates @ref pendingObjects into the active arrays.
     * 2) Calls each object's Update(dt, engineContext).
     * 3) Invokes @ref CheckCollision() to run broad-phase queries and
     *    narrow-phase checks according to @ref collisionGroupRegistry.
     * 4) Cleans up killed objects with @ref EraseDeadObjects().
     *
     * @param dt Delta time in seconds since the previous frame.
     * @param engineContext Read-only access to engine subsystems.
     */
    void UpdateAll(float dt, const EngineContext& engineContext);

    /**
     * @brief Draws all active objects through RenderManager.
     *
     * @details
     * Builds a submission set from @ref rawPtrObjects in layer/depth order
     * and forwards it to RenderManager. Debug collider rendering may be issued
     * via @ref DrawColliderDebug() if engine debug flags are enabled.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    void DrawAll(const EngineContext& engineContext);

    /**
     * @brief Draws a specific list of objects through RenderManager.
     *
     * @param engineContext Read-only access to engine subsystems.
     * @param objects A preselected set of object pointers to draw.
     */
    void DrawObjects(const EngineContext& engineContext, const std::vector<Object*>& objects);

    /**
     * @brief Draws all objects that match the given tag.
     *
     * @param engineContext Read-only access to engine subsystems.
     * @param tag Tag string to match (exact match).
     */
    void DrawObjectsWithTag(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Frees all objects in reverse of their lifetime.
     *
     * @details
     * Calls Free() and LateFree() for all active objects, clears containers,
     * and resets maps/indices to an empty state.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    void FreeAll(const EngineContext& engineContext);

    /**
     * @brief Finds the first object with an exact tag match.
     *
     * @param tag Tag to look up.
     * @return Pointer to the object if found; nullptr otherwise.
     * @note
     * Use @ref FindByTag(const std::string&, std::vector<Object*>&) to collect all matches.
     */
    [[nodiscard]] Object* FindByTag(const std::string& tag) const;

    /**
     * @brief Gathers all objects whose tag matches exactly.
     *
     * @param tag Tag string to match.
     * @param result Output vector receiving all matching pointers (appended).
     * @code
     * std::vector<Object*> enemies;
     * objectManager.FindByTag("[Object]Enemy", enemies);
     * @endcode
     */
    void FindByTag(const std::string& tag, std::vector<Object*>& result);

    /**
     * @brief Runs broad-phase + narrow-phase collision checks.
     *
     * @details
     * Uses @ref broadPhaseGrid for candidate pair generation, applies
     * @ref collisionGroupRegistry filters/masks, then calls OnCollision()
     * for confirmed overlaps. Intended to be called from @ref UpdateAll().
     */
    void CheckCollision();

    /**
     * @brief Returns the registry that controls collision groups and masks.
     *
     * @return Reference to the collision group registry.
     */
    [[nodiscard]] CollisionGroupRegistry& GetCollisionGroupRegistry() { return collisionGroupRegistry; }

    /**
     * @brief Returns a snapshot of all active raw object pointers.
     *
     * @details
     * The returned vector is a copy of the internal raw-pointer view.
     * Pointers remain valid while the underlying objects are alive.
     *
     * @return Vector of raw pointers to active objects.
     */
    [[nodiscard]] std::vector<Object*> GetAllRawPtrObjects() { return rawPtrObjects; }

private:
    /**
     * @brief Integrates pending objects into active containers.
     *
     * @details
     * Moves unique_ptrs from @ref pendingObjects to @ref objects, refreshes
     * @ref rawPtrObjects, and updates @ref objectMap entries for tagged objects.
     * Calls Init()/LateInit() for newly added objects if required by the current phase.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    void AddAllPendingObjects(const EngineContext& engineContext);

    /**
     * @brief Removes dead objects and updates indices/maps.
     *
     * @details
     * Erases objects that reported death (e.g., IsAlive()==false), removes
     * their entries from @ref rawPtrObjects and @ref objectMap, and compacts
     * @ref objects to keep iteration cache-friendly.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    void EraseDeadObjects(const EngineContext& engineContext);

    /**
     * @brief Issues collider debug draw commands for visible objects.
     *
     * @param rm Render manager used to enqueue debug line draws.
     * @param cam Camera used for debug overlay transform (nullable for HUD).
     */
    void DrawColliderDebug(RenderManager* rm, Camera2D* cam);

    std::vector<std::unique_ptr<Object>> objects;              ///< Active, owned objects.
    std::vector<std::unique_ptr<Object>> pendingObjects;       ///< Deferred additions to be integrated.
    std::unordered_map<std::string, Object*> objectMap;        ///< Tag->object (first or latest) map.
    std::vector<Object*> rawPtrObjects;                        ///< Cached raw-pointer view of @ref objects.
    SpatialHashGrid broadPhaseGrid;                            ///< Broad-phase spatial hash.
    CollisionGroupRegistry collisionGroupRegistry;             ///< Collision filtering (groups/masks).
};
