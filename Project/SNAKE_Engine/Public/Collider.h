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
 * @brief Collision shape categories for type-based dispatch.
 *
 * @details
 * Identifies which concrete collider a shape belongs to so the collision
 * system can route pair tests using double dispatch (e.g., Circle vs AABB).
 * Extend this enum when introducing new collider types and implement the
 * corresponding overrides in each subclass.
 */
enum class ColliderType
{
    None,
    Circle,
    AABB
};

/**
 * @brief Abstract base for 2D colliders attached to an Object.
 *
 * @details
 * A Collider defines a 2D collision shape for its owning Object and exposes
 * a uniform API for point tests, pairwise collision tests, and debug drawing.
 *
 * Integration & flow:
 * - Ownership: a Collider is owned by an Object (see @ref owner). Lifetime is
 *   tied to the Object's lifetime.
 * - Broad phase: @ref SpatialHashGrid partitions Objects and enumerates candidate
 *   pairs; @ref ObjectManager then calls @ref CheckCollision on those pairs.
 * - Narrow phase: @ref CheckCollision uses double dispatch via @ref DispatchAgainst
 *   to invoke the correct shape–shape test.
 * - Transform: if @ref useTransformScale is true, @ref SyncWithTransformScale is
 *   called from ObjectManager each frame to update internal scaled dimensions.
 * - Debug: @ref DrawDebug renders the shape using @ref RenderManager in world space.
 *
 * Notes:
 * - All non-const behavioral APIs are virtual; concrete shapes (circle, AABB)
 *   must implement them.
 * - Point coordinate spaces: point and rendering are in world space; shapes
 *   should incorporate the owner's transform and @ref offset when testing.
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
     * @brief Construct with an owning Object.
     * @param owner_ Non-null owner that provides world transform and tags.
     *
     * @note The collider uses the owner's Transform for position/scale.
     *       Local @ref offset can translate the shape relative to the owner.
     */
    Collider(Object* owner_) : owner(owner_), offset() {}

    virtual ~Collider() = default;

    /**
     * @brief Enable/disable syncing the shape with the owner's Transform scale.
     * @param use If true, shape dimensions are derived from the owner's scale.
     *
     * @note When enabled, @ref SyncWithTransformScale is expected to refresh
     *       cached, scaled values every frame before collision tests.
     */
    void SetUseTransformScale(bool use) { useTransformScale = use; }

    /**
     * @brief Whether transform-scale synchronization is enabled.
     * @return True if the collider reads scale from the owner's Transform.
     */
    [[nodiscard]] bool IsUsingTransformScale() const { return useTransformScale; }

    /**
     * @brief Set local shape offset in world units relative to the owner.
     * @param pos Local XY offset applied to the collider center.
     */
    void SetOffset(const glm::vec2& pos) { offset = pos; }

    /**
     * @brief Get local shape offset.
     * @return Local XY offset from the owner's origin.
     */
    [[nodiscard]] const glm::vec2& GetOffset() const { return offset; }

    /**
     * @brief Point-in-shape test in world space.
     * @param point World-space point.
     * @return True if the point lies inside or on the shape boundary.
     */
    [[nodiscard]] virtual bool CheckPointCollision(const glm::vec2& point) const = 0;

protected:
    /**
     * @brief Owning Object accessor.
     * @return Non-null owner.
     */
    [[nodiscard]] Object* GetOwner() const { return owner; }

    /**
     * @brief Concrete collider type for dispatch.
     * @return One of @ref ColliderType values.
     */
    [[nodiscard]] virtual ColliderType GetType() const = 0;

    /**
     * @brief Bounding radius for broad-phase culling.
     * @return Non-negative radius that encloses the shape.
     *
     * @note Used to quickly reject pairs in @ref SpatialHashGrid and camera culling.
     */
    [[nodiscard]] virtual float GetBoundingRadius() const = 0;

    /**
     * @brief Shape–shape test entry point (narrow phase).
     * @param other Other collider.
     * @return True if shapes overlap.
     *
     * @details
     * Implementations normally forward to @ref DispatchAgainst(*)
     * based on @ref other 's dynamic type (double dispatch).
     */
    [[nodiscard]] virtual bool CheckCollision(const Collider* other) const = 0;

    /**
     * @brief Double-dispatch target for Circle vs Circle/AABB tests.
     * @param other Other circle collider (world-space).
     * @return True if overlapping.
     */
    [[nodiscard]] virtual bool DispatchAgainst(const CircleCollider& other) const = 0;

    /**
     * @brief Double-dispatch target for AABB vs Circle/AABB tests.
     * @param other Other AABB collider (world-space).
     * @return True if overlapping.
     */
    [[nodiscard]] virtual bool DispatchAgainst(const AABBCollider& other) const = 0;

    /**
     * @brief Update cached, scaled shape dimensions from owner's Transform.
     *
     * @details
     * Called by ObjectManager before collision checks when
     * @ref useTransformScale is enabled. Implementations should read the
     * owner's current scale and update internal fields used in tests and debug draw.
     */
    virtual void SyncWithTransformScale() = 0;

    /**
     * @brief Draw the shape's outline for debugging.
     * @param rm Render manager used to enqueue line geometry.
     * @param cam Active camera used for view/projection.
     * @param color RGBA of the debug lines (default: red).
     *
     * @note Rendering must occur in world space, matching collision coordinates.
     */
    virtual void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color = { 1,0,0,1 }) const = 0;

    /** @brief Owning Object; non-null after construction. */
    Object* owner;

    /** @brief Whether to read scale from the owner's Transform (default: true). */
    bool useTransformScale = true;

    /** @brief Local XY offset from the owner's origin, applied to shape center. */
    glm::vec2 offset;
};


/**
 * @brief Circle collider with radius centered at owner's position plus offset.
 *
 * @details
 * Maintains both the base (authoring) radius and the scaled radius derived from
 * the owner's Transform scale when @ref useTransformScale is enabled.
 */
class CircleCollider : public Collider
{
    friend AABBCollider;
    friend SpatialHashGrid;
public:
    /**
     * @brief Construct a circle collider.
     * @param owner Non-null owning Object.
     * @param size Authoring diameter in world units (radius = size/2).
     */
    CircleCollider(Object* owner, float size)
        : Collider(owner), baseRadius(size / 2.f), scaledRadius(size / 2.f) {
    }

    /**
     * @brief Get current radius in world units (scaled if enabled).
     * @return Non-negative radius.
     */
    [[nodiscard]] float GetRadius() const;

    /**
     * @brief Get current diameter in world units (2 * radius).
     * @return Non-negative diameter.
     */
    [[nodiscard]] float GetSize() const;

    /**
     * @brief Set authoring radius (updates baseRadius; scaled on sync).
     * @param r New base radius in world units.
     */
    void SetRadius(float r);

    /**
     * @brief Point-in-circle test in world space.
     * @param point World-space point.
     * @return True if the point lies inside or on the circle.
     */
    [[nodiscard]] bool CheckPointCollision(const glm::vec2& point) const override;

private:
    [[nodiscard]] ColliderType GetType() const override { return ColliderType::Circle; }

    /**
     * @brief Bounding radius used by broad phase and camera culling.
     * @return Equal to @ref GetRadius().
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Narrow-phase entry; routes by other's dynamic type.
     * @param other Other collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool CheckCollision(const Collider* other) const override;

    /**
     * @brief Circle–Circle overlap test.
     * @param other Other circle.
     * @return True if overlapping.
     */
    [[nodiscard]] bool DispatchAgainst(const CircleCollider& other) const override;

    /**
     * @brief Circle–AABB overlap test.
     * @param other Other axis-aligned box.
     * @return True if overlapping.
     */
    [[nodiscard]] bool DispatchAgainst(const AABBCollider& other) const override;

    /**
     * @brief Refresh @ref scaledRadius from owner's Transform scale.
     */
    void SyncWithTransformScale() override;

    /**
     * @brief Debug-draw circle outline.
     */
    void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color) const override;

    /** @brief Authoring radius (before applying owner scale). */
    float baseRadius = 0.5f;

    /** @brief Cached world-space radius after scale sync. */
    float scaledRadius = 0.5f;
};


/**
 * @brief Axis-aligned bounding box collider centered at owner plus offset.
 *
 * @details
 * Stores authoring half-size and a cached, scale-applied half-size for fast
 * overlap tests, point tests, and debug visualization.
 */
class AABBCollider : public Collider
{
    friend CircleCollider;
    friend SpatialHashGrid;
public:
    /**
     * @brief Construct an AABB collider.
     * @param owner Non-null owning Object.
     * @param size Full width/height in world units (half stored internally).
     */
    AABBCollider(Object* owner, const glm::vec2& size)
        : Collider(owner), baseHalfSize(size / glm::vec2(2)), scaledHalfSize(size / glm::vec2(2)) {
    }

    /**
     * @brief Current half-size (scaled if enabled).
     * @return Half extents in world units.
     */
    [[nodiscard]] glm::vec2 GetHalfSize() const;

    /**
     * @brief Current full size (2 * half-size).
     * @return Width/height in world units.
     */
    [[nodiscard]] glm::vec2 GetSize() const;

    /**
     * @brief Set full size (updates baseHalfSize; scaled on sync).
     * @param hs New full width/height in world units.
     */
    void SetSize(const glm::vec2& hs);

    /**
     * @brief Point-in-AABB test in world space.
     * @param point World-space point.
     * @return True if inside or on the box.
     */
    [[nodiscard]] bool CheckPointCollision(const glm::vec2& point) const override;

private:
    [[nodiscard]] ColliderType GetType() const override { return ColliderType::AABB; }

    /**
     * @brief Bounding radius used by broad phase and camera culling.
     * @return Circle radius that encloses the AABB.
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Narrow-phase entry; routes by other's dynamic type.
     * @param other Other collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool CheckCollision(const Collider* other) const override;

    /**
     * @brief AABB–Circle overlap test.
     * @param other Circle collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool DispatchAgainst(const CircleCollider& other) const override;

    /**
     * @brief AABB–AABB overlap test.
     * @param other Box collider.
     * @return True if overlapping.
     */
    [[nodiscard]] bool DispatchAgainst(const AABBCollider& other) const override;

    /**
     * @brief Refresh @ref scaledHalfSize from owner's Transform scale.
     */
    void SyncWithTransformScale() override;

    /**
     * @brief Debug-draw box outline.
     */
    void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color) const override;

    /** @brief Authoring half-size (before applying owner scale). */
    glm::vec2 baseHalfSize = { 0.5f, 0.5f };

    /** @brief Cached world-space half-size after scale sync. */
    glm::vec2 scaledHalfSize = { 0.5f, 0.5f };
};

/**
 * @brief Hash functor for glm::ivec2 to use in unordered containers.
 *
 * @details
 * Uses a pair of large primes and XOR to mix 2D integer coordinates.
 * Suitable for sparse 2D grids such as spatial hashing buckets.
 */
struct Vec2Hash
{
    size_t operator()(const glm::ivec2& v) const
    {
        return std::hash<int>()(v.x * 73856093 ^ v.y * 19349663);
    }
};

/**
 * @brief Uniform spatial hash grid for broad-phase collision candidate generation.
 *
 * @details
 * Partitions world space into square cells of side @ref cellSize (in world units).
 * Objects with colliders are inserted into one or more cells based on their
 * world position and @ref Collider::GetBoundingRadius. Objects that are too large
 * relative to @ref cellSize are tracked in @ref largeObjects.
 *
 * Pipeline:
 * - @ref Clear resets internal buckets for the current frame.
 * - @ref Insert assigns each object to all cells touched by its bounding circle,
 *   or to @ref largeObjects if it spans many cells.
 * - @ref ComputeCollisions iterates buckets and invokes the supplied callback
 *   for each unique candidate pair (Object*, Object*). Mask/group filtering is
 *   performed at a higher layer (@ref ObjectManager).
 *
 * @warning The grid is rebuilt every frame; choose @ref cellSize carefully to
 *          balance insertion cost and bucket density.
 */
class SpatialHashGrid
{
    friend ObjectManager;
private:
    /**
     * @brief Remove all buckets and cached per-frame state.
     */
    void Clear();

    /**
     * @brief Insert an object based on its collider's world position and radius.
     * @param obj Object with a non-null collider.
     *
     * @note Objects exceeding a size threshold may be routed to @ref largeObjects.
     */
    void Insert(Object* obj);

    /**
     * @brief Enumerate candidate pairs inside buckets and invoke a callback.
     * @param onCollision Callback invoked with (A, B) for each candidate pair.
     *
     * @details
     * Pairs are generated per-cell and across @ref largeObjects; duplicates across
     * cells are suppressed. The callback should perform fine-grained checks
     * (masks, group tags, and @ref Collider::CheckCollision).
     */
    void ComputeCollisions(std::function<void(Object*, Object*)> onCollision);

    /**
     * @brief Convert a world position to integer cell coordinates.
     * @param pos World-space position.
     * @return Discrete cell index (x, y).
     */
    [[nodiscard]] glm::ivec2 GetCell(const glm::vec2& pos) const;

    /**
     * @brief Insert an object into a specific grid cell bucket.
     * @param obj Object pointer.
     * @param cell Discrete cell index.
     */
    void InsertToCell(Object* obj, const glm::ivec2& cell);

    /** @brief Cell side length in world units (default: 50). */
    int cellSize = 50;

    /** @brief Cell buckets: map from cell coords to resident objects. */
    std::unordered_map<glm::ivec2, std::vector<Object*>, Vec2Hash> grid;

    /** @brief Objects too large for regular buckets (tracked separately). */
    std::vector<Object*> largeObjects;

    /** @brief Staging cache used within the current frame. */
    std::vector<Object*> objects;
};

/**
 * @brief Registry mapping collision group tags to bitmasks.
 *
 * @details
 * Provides stable bit assignments for textual group tags so that
 * Objects can configure @c collisionCategory and @c collisionMask.
 * The first unseen tag is assigned the next available bit index.
 *
 * Bidirectional maps:
 * - @ref tagToBit : std::string -> bit index
 * - @ref bitToTag : bit index   -> std::string
 *
 * @note Bits are assigned monotonically via @ref currentBit; ensure consumers
 *       handle cases where the bit width limit could be reached.
 */
class CollisionGroupRegistry
{
    friend Collider;
    friend Object;
private:
    /**
     * @brief Get (or allocate) a bit for a textual group tag.
     * @param tag Group tag name.
     * @return Bit mask with the assigned bit set (1u << index).
     */
    [[nodiscard]] uint32_t GetGroupBit(const std::string& tag);

    /**
     * @brief Resolve a single-bit mask back to its textual tag.
     * @param bit Bit mask with exactly one bit set.
     * @return Group tag string, or empty if unknown/invalid.
     */
    [[nodiscard]] std::string GetGroupTag(uint32_t bit) const;

    /** @brief Text -> bit index map. */
    std::unordered_map<std::string, uint32_t> tagToBit;

    /** @brief Bit index -> text map. */
    std::unordered_map<uint32_t, std::string> bitToTag;

    /** @brief Next free bit index (monotonic). */
    uint32_t currentBit = 0;
};
