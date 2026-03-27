#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "glm.hpp"


class SpatialHashGrid;
class ObjectManager;
class Camera2D;
class RenderManager;
class Object;
class CircleCollider;
class AABBCollider;

/**
 * @brief Identifies the runtime type of a collider.
 *
 * @details
 * This enum is used internally to distinguish collider shapes.
 * The current engine supports circle colliders and axis-aligned bounding box colliders.
 */
enum class ColliderType
{
    None,
    Circle,
    AABB
};

/**
 * @brief Base interface for all collider shapes used by the engine.
 *
 * @details
 * Collider provides the common interface required by the collision system.
 * Each collider is attached to an owning Object and may optionally follow the owner's transform scale.
 *
 * The class exposes point collision checks publicly, while pairwise collision checks, bounding radius queries,
 * transform synchronization, and debug drawing are kept protected because they are intended to be driven by
 * engine systems such as ObjectManager and SpatialHashGrid.
 *
 * Collision dispatch is implemented using double dispatch. The generic CheckCollision() entry point forwards
 * the test to the other collider, which then resolves the exact shape-vs-shape routine through DispatchAgainst().
 *
 * @note
 * Collider does not own the Object pointer. The owner must outlive the collider.
 *
 * @note
 * When useTransformScale is enabled, the collider shape is updated from the owner's world scale during engine update.
 */
class Collider
{
    friend ObjectManager;
    friend CircleCollider;
    friend AABBCollider;
    friend SpatialHashGrid;
public:
    Collider() = delete;

    /**
     * @brief Constructs a collider and binds it to an owning object.
     *
     * @param owner_ Pointer to the object that owns this collider.
     *
     * @note
     * The offset is initialized to zero.
     */
    Collider(Object* owner_) : owner(owner_), offset() {}

    /**
     * @brief Virtual destructor for polymorphic collider deletion.
     */
    virtual ~Collider() = default;

    /**
     * @brief Enables or disables automatic scaling from the owner's transform.
     *
     * @details
     * When enabled, the collider shape is recalculated from the owner's world scale through SyncWithTransformScale().
     * For CircleCollider, the largest absolute scale axis is used.
     * For AABBCollider, each axis is scaled independently. :contentReference[oaicite:1]{index=1}
     *
     * @param use True to follow transform scale, false to use manually assigned shape values.
     */
    void SetUseTransformScale(bool use) { useTransformScale = use; }

    /**
     * @brief Returns whether this collider currently follows the owner's transform scale.
     *
     * @return True if transform scale synchronization is enabled.
     */
    [[nodiscard]] bool IsUsingTransformScale() const { return useTransformScale; }

    /**
     * @brief Sets a local positional offset for the collider.
     *
     * @details
     * The offset is added to the owner's world position during collision checks and debug rendering.
     *
     * @param pos Local offset from the owner's origin.
     */
    void SetOffset(const glm::vec2& pos) { offset = pos; }

    /**
     * @brief Returns the collider's local offset.
     *
     * @return Reference to the current local offset.
     */
    [[nodiscard]] const glm::vec2& GetOffset() const { return offset; }

    /**
     * @brief Checks whether a world-space point overlaps this collider.
     *
     * @param point Point in world space.
     * @return True if the point lies inside or on the collider.
     */
    [[nodiscard]] virtual bool CheckPointCollision(const glm::vec2& point) const = 0;

protected:
    /**
     * @brief Returns the owning object.
     *
     * @return Pointer to the owner object.
     */
    [[nodiscard]] Object* GetOwner() const { return owner; }

    /**
     * @brief Returns the concrete collider type.
     *
     * @return Runtime collider type.
     */
    [[nodiscard]] virtual ColliderType GetType() const = 0;

    /**
     * @brief Returns a broad-phase bounding radius for this collider.
     *
     * @details
     * This value is used by SpatialHashGrid to estimate cell coverage and insertion area.
     *
     * @return Bounding radius in world scale.
     */
    [[nodiscard]] virtual float GetBoundingRadius() const = 0;

    /**
     * @brief Performs a collision test against another collider.
     *
     * @details
     * This is the generic entry point used by the collision system.
     * Implementations forward the call to the other collider's DispatchAgainst() overload
     * to resolve the exact shape combination. :contentReference[oaicite:2]{index=2}
     *
     * @param other Pointer to the other collider.
     * @return True if the two colliders overlap.
     */
    [[nodiscard]] virtual bool CheckCollision(const Collider* other) const = 0;

    /**
     * @brief Resolves collision against a circle collider.
     *
     * @param other The circle collider on the other side of the test.
     * @return True if the colliders overlap.
     */
    [[nodiscard]] virtual bool DispatchAgainst(const CircleCollider& other) const = 0;

    /**
     * @brief Resolves collision against an axis-aligned bounding box collider.
     *
     * @param other The AABB collider on the other side of the test.
     * @return True if the colliders overlap.
     */
    [[nodiscard]] virtual bool DispatchAgainst(const AABBCollider& other) const = 0;

    /**
     * @brief Synchronizes the collider's effective size with the owner's transform scale.
     *
     * @details
     * This is called by ObjectManager during the update loop for alive objects that own a collider.
     */
    virtual void SyncWithTransformScale() = 0;

    /**
     * @brief Draws a debug visualization of the collider.
     *
     * @details
     * The current implementation uses RenderManager debug line commands.
     * Circle colliders are drawn as segmented line loops.
     * AABB colliders are drawn as four line edges.
     *
     * @param rm Render manager used to queue debug lines.
     * @param cam Camera used for debug line rendering.
     * @param color Debug draw color.
     */
    virtual void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color = { 1,0,0,1 }) const = 0;

    Object* owner;
    bool useTransformScale = true;
    glm::vec2 offset;
};


/**
 * @brief Circle-shaped collider implementation.
 *
 * @details
 * CircleCollider stores both a base radius and a scaled radius.
 * When transform scaling is enabled, GetRadius() derives the effective size from the owner's world scale
 * using the larger absolute axis. Otherwise, the manually stored scaledRadius value is used. :contentReference[oaicite:5]{index=5}
 *
 * Circle-vs-circle checks are resolved by comparing squared distance against the squared sum of radii.
 * Circle-vs-AABB checks are forwarded to AABBCollider for mixed-shape resolution.
 */
class CircleCollider : public Collider
{
    friend AABBCollider;
    friend SpatialHashGrid;
public:
    /**
     * @brief Constructs a circle collider from a diameter value.
     *
     * @param owner Pointer to the owning object.
     * @param size Circle diameter.
     *
     * @note
     * The constructor stores size / 2 as both baseRadius and scaledRadius.
     */
    CircleCollider(Object* owner, float size)
        : Collider(owner), baseRadius(size / 2.f), scaledRadius(size / 2.f) {
    }

    /**
     * @brief Returns the effective radius of the circle.
     *
     * @details
     * If transform scaling is enabled, the radius is computed as:
     * baseRadius * max(abs(worldScale.x), abs(worldScale.y)).
     * Otherwise, the stored scaledRadius is returned. :contentReference[oaicite:6]{index=6}
     *
     * @return Effective world-space radius.
     */
    [[nodiscard]] float GetRadius() const;

    /**
     * @brief Returns the effective diameter of the circle.
     *
     * @return Radius * 2.
     */
    [[nodiscard]] float GetSize() const;

    /**
     * @brief Sets the radius manually.
     *
     * @details
     * This only changes the stored radius when transform scaling is disabled.
     * If transform scaling is enabled, the effective radius continues to follow the owner's scale. :contentReference[oaicite:7]{index=7}
     *
     * @param r New radius value.
     */
    void SetRadius(float r);

    /**
     * @brief Checks whether a world-space point lies inside the circle.
     *
     * @param point Point in world space.
     * @return True if the point is inside or on the circle.
     */
    [[nodiscard]] bool CheckPointCollision(const glm::vec2& point) const override;

private:
    /**
     * @brief Returns ColliderType::Circle.
     *
     * @return Circle collider type.
     */
    [[nodiscard]] ColliderType GetType() const override { return ColliderType::Circle; }

    /**
     * @brief Returns the broad-phase bounding radius for this collider.
     *
     * @return The same value as GetRadius().
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Performs generic collision against another collider.
     *
     * @details
     * This forwards to other->DispatchAgainst(*this) to resolve the actual shape pair. :contentReference[oaicite:8]{index=8}
     *
     * @param other Other collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool CheckCollision(const Collider* other) const override;

    /**
     * @brief Resolves circle-vs-circle collision.
     *
     * @details
     * Uses squared distance comparison for efficiency.
     *
     * @param other Other circle collider.
     * @return True if the circles overlap.
     */
    [[nodiscard]] bool DispatchAgainst(const CircleCollider& other) const override;

    /**
     * @brief Resolves circle-vs-AABB collision.
     *
     * @details
     * This implementation forwards the mixed-shape test to the AABB collider side.
     *
     * @param other Other AABB collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool DispatchAgainst(const AABBCollider& other) const override;

    /**
     * @brief Updates the scaled radius from the owner's transform.
     *
     * @details
     * When transform scaling is enabled, scaledRadius is recalculated using the largest absolute world scale axis. :contentReference[oaicite:9]{index=9}
     */
    void SyncWithTransformScale() override;

    /**
     * @brief Queues debug lines that approximate the circle.
     *
     * @details
     * The implementation currently uses 20 line segments to visualize the circle. :contentReference[oaicite:10]{index=10}
     *
     * @param rm Render manager used for debug line submission.
     * @param cam Camera used for rendering the debug lines.
     * @param color Line color.
     */
    void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color) const override;

    float baseRadius = 0.5f;
    float scaledRadius = 0.5f;
};


/**
 * @brief Axis-aligned bounding box collider implementation.
 *
 * @details
 * AABBCollider stores half extents internally.
 * When transform scaling is enabled, GetHalfSize() derives the effective extents from the owner's world scale
 * per axis. Otherwise, the manually stored scaledHalfSize is used. :contentReference[oaicite:11]{index=11}
 *
 * AABB-vs-AABB checks compare center distance against the sum of half extents.
 * AABB-vs-circle checks clamp the circle center to the AABB bounds and compare against the circle radius.
 */
class AABBCollider : public Collider
{
    friend CircleCollider;
    friend SpatialHashGrid;
public:
    /**
     * @brief Constructs an AABB collider from full size values.
     *
     * @param owner Pointer to the owning object.
     * @param size Full width and height of the box.
     *
     * @note
     * The constructor stores half of the provided size internally.
     */
    AABBCollider(Object* owner, const glm::vec2& size)
        : Collider(owner), baseHalfSize(size / glm::vec2(2)), scaledHalfSize(size / glm::vec2(2)) {
    }

    /**
     * @brief Returns the effective half extents of the box.
     *
     * @details
     * If transform scaling is enabled, baseHalfSize is multiplied by abs(owner->GetWorldScale()).
     * Otherwise, scaledHalfSize is returned. :contentReference[oaicite:12]{index=12}
     *
     * @return Effective half size in world space.
     */
    [[nodiscard]] glm::vec2 GetHalfSize() const;

    /**
     * @brief Returns the full effective size of the box.
     *
     * @return Half size multiplied by two.
     */
    [[nodiscard]] glm::vec2 GetSize() const;

    /**
     * @brief Sets the full size manually.
     *
     * @details
     * The input value is converted to half extents internally.
     * This only changes the stored size when transform scaling is disabled. :contentReference[oaicite:13]{index=13}
     *
     * @param hs New full box size.
     */
    void SetSize(const glm::vec2& hs);

    /**
     * @brief Checks whether a world-space point lies inside the box.
     *
     * @param point Point in world space.
     * @return True if the point lies within the AABB bounds.
     */
    [[nodiscard]] bool CheckPointCollision(const glm::vec2& point) const override;

private:
    /**
     * @brief Returns ColliderType::AABB.
     *
     * @return AABB collider type.
     */
    [[nodiscard]] ColliderType GetType() const override { return ColliderType::AABB; }

    /**
     * @brief Returns a bounding radius for broad-phase insertion.
     *
     * @details
     * The implementation returns length(halfSize), which corresponds to the distance from center to a corner. :contentReference[oaicite:14]{index=14}
     *
     * @return Broad-phase bounding radius.
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Performs generic collision against another collider.
     *
     * @details
     * This forwards to other->DispatchAgainst(*this) to resolve the exact shape pair. :contentReference[oaicite:15]{index=15}
     *
     * @param other Other collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool CheckCollision(const Collider* other) const override;

    /**
     * @brief Resolves AABB-vs-circle collision.
     *
     * @details
     * The circle center is clamped to the box bounds to find the closest point,
     * then the squared distance is compared against the squared circle radius. :contentReference[oaicite:16]{index=16}
     *
     * @param other Other circle collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool DispatchAgainst(const CircleCollider& other) const override;

    /**
     * @brief Resolves AABB-vs-AABB collision.
     *
     * @details
     * Overlap is checked independently on the x and y axes using box centers and half extents. :contentReference[oaicite:17]{index=17}
     *
     * @param other Other AABB collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool DispatchAgainst(const AABBCollider& other) const override;

    /**
     * @brief Updates the scaled half extents from the owner's transform.
     *
     * @details
     * When transform scaling is enabled, each axis uses abs(owner->GetWorldScale()) independently. :contentReference[oaicite:18]{index=18}
     */
    void SyncWithTransformScale() override;

    /**
     * @brief Queues four debug lines that represent the box edges.
     *
     * @param rm Render manager used for debug line submission.
     * @param cam Camera used for rendering the debug lines.
     * @param color Line color.
     */
    void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color) const override;

    glm::vec2 baseHalfSize = { 0.5f, 0.5f };
    glm::vec2 scaledHalfSize = { 0.5f, 0.5f };
};

/**
 * @brief Hash functor for glm::ivec2 keys.
 *
 * @details
 * Used by SpatialHashGrid to store per-cell object lists in an unordered_map.
 * The implementation mixes x and y using large prime multipliers.
 */
struct Vec2Hash
{
    /**
     * @brief Computes a hash value for a grid cell coordinate.
     *
     * @param v Integer 2D grid coordinate.
     * @return Hash value for unordered_map lookup.
     */
    size_t operator()(const glm::ivec2& v) const
    {
        return std::hash<int>()(v.x * 73856093 ^ v.y * 19349663);
    }
};

/**
 * @brief Broad-phase collision structure based on a spatial hash grid.
 *
 * @details
 * SpatialHashGrid partitions world space into fixed-size cells and inserts objects according to the
 * bounding radius of their colliders. During collision checking, ObjectManager clears the grid, inserts
 * current objects, and asks the grid to emit candidate object pairs.
 *
 * Objects that cover too many cells are treated as large objects and are stored separately.
 * Those objects are compared against all inserted objects to avoid excessive cell insertion cost. :contentReference[oaicite:20]{index=20}
 *
 * @note
 * This class is intended for internal engine use through ObjectManager.
 */
class SpatialHashGrid
{
    friend ObjectManager;
private:
    /**
     * @brief Clears all stored cells and object lists.
     */
    void Clear();

    /**
     * @brief Inserts an object into the broad-phase structure.
     *
     * @details
     * The object is skipped if it is not alive or does not have a collider.
     * Otherwise, the collider bounding radius is used to determine the covered grid range.
     * Large objects are redirected into a dedicated list instead of being inserted into every cell. :contentReference[oaicite:21]{index=21}
     *
     * @param obj Object to insert.
     */
    void Insert(Object* obj);

    /**
     * @brief Enumerates potential collision pairs produced by the grid.
     *
     * @details
     * First compares large objects against all inserted objects, then enumerates unique pairs within each occupied cell.
     * The callback is responsible for the narrow-phase collision test and any duplicate filtering beyond cell-local pairing. :contentReference[oaicite:22]{index=22}
     *
     * @param onCollision Callback receiving candidate object pairs.
     */
    void ComputeCollisions(std::function<void(Object*, Object*)> onCollision);

    /**
     * @brief Converts a world-space position to a grid cell coordinate.
     *
     * @param pos Position in world space.
     * @return Integer cell coordinate.
     */
    [[nodiscard]] glm::ivec2 GetCell(const glm::vec2& pos) const;

    /**
     * @brief Inserts an object into a specific cell list.
     *
     * @param obj Object to insert.
     * @param cell Target cell coordinate.
     */
    void InsertToCell(Object* obj, const glm::ivec2& cell);

    int cellSize = 50;
    std::unordered_map<glm::ivec2, std::vector<Object*>, Vec2Hash> grid;
    std::vector<Object*> largeObjects;
    std::vector<Object*> objects;
};

/**
 * @brief Registry that assigns collision group tags to bit masks.
 *
 * @details
 * Collision categories and masks are represented as 32-bit bitfields.
 * When an object requests a collision group by tag, a new bit is assigned if the tag has not been seen before.
 * The registry also supports reverse lookup from bit value to tag string.
 *
 * @note
 * The current implementation supports up to 32 unique groups.
 */
class CollisionGroupRegistry
{
    friend Collider;
    friend Object;
private:
    /**
     * @brief Returns the bit value associated with a collision group tag.
     *
     * @details
     * If the tag is already registered, the existing bit is returned.
     * Otherwise, a new bit is assigned. If more than 32 groups are requested,
     * the implementation logs an error and returns UINT32_MAX. :contentReference[oaicite:24]{index=24}
     *
     * @param tag Collision group name.
     * @return Bit mask value for the group.
     */
    [[nodiscard]] uint32_t GetGroupBit(const std::string& tag);

    /**
     * @brief Resolves a bit value back to its registered group tag.
     *
     * @param bit Collision group bit value.
     * @return Registered tag string, or "unknown" if not found.
     */
    [[nodiscard]] std::string GetGroupTag(uint32_t bit) const;

    std::unordered_map<std::string, uint32_t> tagToBit;
    std::unordered_map<uint32_t, std::string> bitToTag;
    uint32_t currentBit = 0;
};