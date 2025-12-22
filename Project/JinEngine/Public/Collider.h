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
 * @brief Identifies the concrete collider shape type at runtime.
 *
 * @details
 * Used internally for type checks and debugging, and as a lightweight
 * descriptor for collider behavior. The collision system itself uses
 * double dispatch (DispatchAgainst) rather than switching solely on this enum.
 */
enum class ColliderType
{
    None,
    Circle,
    AABB
};

/**
 * @brief Abstract base collider that supports broad-phase + narrow-phase collision checks.
 *
 * @details
 * Collider represents a shape attached to an Object and provides the core
 * interfaces used by ObjectManager and SpatialHashGrid:
 * - point containment test (CheckPointCollision)
 * - shape-vs-shape collision test (CheckCollision + DispatchAgainst)
 * - broad-phase approximation (GetBoundingRadius)
 * - optional transform scale synchronization (SyncWithTransformScale)
 * - debug visualization (DrawDebug)
 *
 * Narrow-phase collision is implemented using double dispatch:
 * CheckCollision(other) calls other->DispatchAgainst(*thisConcrete) or an equivalent path,
 * allowing Circle-vs-AABB and AABB-vs-Circle to be resolved without RTTI/dynamic_cast.
 *
 * Offset is stored separately from the owner's transform and is typically used to
 * shift the collider relative to the owner's origin.
 *
 * This class is not default-constructible; an owner must be provided.
 */
class Collider
{
    friend ObjectManager;
    friend CircleCollider;
    friend AABBCollider;
    friend SpatialHashGrid;
public:
    /**
     * @brief Deleted default constructor to enforce owner association.
     */
    Collider() = delete;

    /**
     * @brief Constructs a collider bound to an owner object.
     *
     * @param owner_ Object that owns this collider.
     */
    Collider(Object* owner_) : owner(owner_), offset() {}

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~Collider() = default;

    /**
     * @brief Enables or disables using the owner's transform scale to size the collider.
     *
     * @details
     * When enabled, derived colliders are expected to update their scaled shape
     * in SyncWithTransformScale.
     *
     * @param use True to apply transform scale to the collider shape.
     */
    void SetUseTransformScale(bool use) { useTransformScale = use; }

    /**
     * @brief Returns whether this collider uses the owner's transform scale.
     */
    [[nodiscard]] bool IsUsingTransformScale() const { return useTransformScale; }

    /**
     * @brief Sets the local-space offset of the collider relative to the owner's origin.
     *
     * @param pos Offset in the same coordinate space as the owner's transform.
     */
    void SetOffset(const glm::vec2& pos) { offset = pos; }

    /**
     * @brief Returns the local-space offset of the collider.
     */
    [[nodiscard]] const glm::vec2& GetOffset() const { return offset; }

    /**
     * @brief Tests whether a point lies within the collider shape.
     *
     * @param point Point to test, typically in world space.
     * @return true if the point is inside or on the collider; false otherwise.
     */
    [[nodiscard]] virtual bool CheckPointCollision(const glm::vec2& point) const = 0;

protected:
    /**
     * @brief Returns the object that owns this collider.
     *
     * @return Pointer to the owner object.
     */
    [[nodiscard]] Object* GetOwner() const { return owner; }

    /**
     * @brief Returns the concrete collider type.
     */
    [[nodiscard]] virtual ColliderType GetType() const = 0;

    /**
     * @brief Returns a radius that bounds the collider for broad-phase checks.
     *
     * @details
     * SpatialHashGrid may use this to classify large objects and/or to estimate
     * which cells should contain an object.
     */
    [[nodiscard]] virtual float GetBoundingRadius() const = 0;

    /**
     * @brief Performs narrow-phase collision test against another collider.
     *
     * @details
     * Implementations typically use double dispatch by calling DispatchAgainst
     * with the appropriate concrete type.
     *
     * @param other Other collider to test against.
     * @return true if colliding; false otherwise.
     */
    [[nodiscard]] virtual bool CheckCollision(const Collider* other) const = 0;

    /**
     * @brief Double-dispatch entry for testing against a circle collider.
     *
     * @param other Other collider as CircleCollider.
     * @return true if colliding; false otherwise.
     */
    [[nodiscard]] virtual bool DispatchAgainst(const CircleCollider& other) const = 0;

    /**
     * @brief Double-dispatch entry for testing against an AABB collider.
     *
     * @param other Other collider as AABBCollider.
     * @return true if colliding; false otherwise.
     */
    [[nodiscard]] virtual bool DispatchAgainst(const AABBCollider& other) const = 0;

    /**
     * @brief Updates internal scaled shape data based on the owner's transform scale.
     *
     * @details
     * Called when useTransformScale is enabled and the owner's scale is expected
     * to affect collider dimensions (e.g., radius / half-extents).
     */
    virtual void SyncWithTransformScale() = 0;

    /**
     * @brief Draws the collider shape for debugging purposes.
     *
     * @details
     * Derived types should issue debug draw commands through the RenderManager
     * and transform into camera space using the provided Camera2D.
     *
     * @param rm Render manager used for debug draw submission.
     * @param cam Camera used for world-to-screen/view transform.
     * @param color Debug draw color (default: red).
     */
    virtual void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color = { 1,0,0,1 }) const = 0;

    Object* owner;
    bool useTransformScale = true;
    glm::vec2 offset;
};

/**
 * @brief Circle collider implementation.
 *
 * @details
 * CircleCollider stores both a base radius and a scaled radius.
 * When transform scale syncing is enabled, scaledRadius is updated
 * relative to baseRadius in SyncWithTransformScale.
 *
 * The constructor receives a size and converts it to radius by dividing by 2.
 */
class CircleCollider : public Collider
{
    friend AABBCollider;
    friend SpatialHashGrid;
public:
    /**
     * @brief Constructs a circle collider from a diameter-like size value.
     *
     * @param owner Object that owns this collider.
     * @param size Full size (diameter) used to initialize radius as size/2.
     */
    CircleCollider(Object* owner, float size)
        : Collider(owner), baseRadius(size / 2.f), scaledRadius(size / 2.f) {
    }

    /**
     * @brief Returns the current scaled radius used for collision tests.
     */
    [[nodiscard]] float GetRadius() const;

    /**
     * @brief Returns the current scaled diameter-like size (radius * 2).
     */
    [[nodiscard]] float GetSize() const;

    /**
     * @brief Sets the base radius and updates the scaled radius accordingly.
     *
     * @param r New base radius.
     */
    void SetRadius(float r);

    /**
     * @brief Tests whether a point lies within the circle area.
     *
     * @param point Point to test.
     * @return true if inside or on the circle; false otherwise.
     */
    [[nodiscard]] bool CheckPointCollision(const glm::vec2& point) const override;

private:
    /**
     * @brief Returns ColliderType::Circle.
     */
    [[nodiscard]] ColliderType GetType() const override { return ColliderType::Circle; }

    /**
     * @brief Returns a broad-phase bounding radius for the circle.
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Performs narrow-phase collision test using double dispatch.
     */
    [[nodiscard]] bool CheckCollision(const Collider* other) const override;

    /**
     * @brief Circle-vs-circle narrow-phase collision.
     */
    [[nodiscard]] bool DispatchAgainst(const CircleCollider& other) const override;

    /**
     * @brief Circle-vs-AABB narrow-phase collision.
     */
    [[nodiscard]] bool DispatchAgainst(const AABBCollider& other) const override;

    /**
     * @brief Updates scaledRadius based on owner transform scale when enabled.
     */
    void SyncWithTransformScale() override;

    /**
     * @brief Draws the circle collider for debugging.
     */
    void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color) const override;

    float baseRadius = 0.5f;
    float scaledRadius = 0.5f;
};

/**
 * @brief Axis-aligned bounding box (AABB) collider implementation.
 *
 * @details
 * AABBCollider stores both base and scaled half-extents.
 * When transform scale syncing is enabled, scaledHalfSize is updated
 * relative to baseHalfSize in SyncWithTransformScale.
 *
 * The constructor receives full size and stores half size internally.
 */
class AABBCollider : public Collider
{
    friend CircleCollider;
    friend SpatialHashGrid;
public:
    /**
     * @brief Constructs an AABB collider from a full size.
     *
     * @param owner Object that owns this collider.
     * @param size Full width/height used to initialize half size as size/2.
     */
    AABBCollider(Object* owner, const glm::vec2& size)
        : Collider(owner), baseHalfSize(size / glm::vec2(2)), scaledHalfSize(size / glm::vec2(2)) {
    }

    /**
     * @brief Returns the current scaled half size used for collision tests.
     */
    [[nodiscard]] glm::vec2 GetHalfSize() const;

    /**
     * @brief Returns the current scaled full size (halfSize * 2).
     */
    [[nodiscard]] glm::vec2 GetSize() const;

    /**
     * @brief Sets the base half size (or equivalent) and updates scaled values accordingly.
     *
     * @param hs New size vector (interpretation depends on implementation; typically full size or half size).
     */
    void SetSize(const glm::vec2& hs);

    /**
     * @brief Tests whether a point lies within the AABB region.
     *
     * @param point Point to test.
     * @return true if inside or on the box; false otherwise.
     */
    [[nodiscard]] bool CheckPointCollision(const glm::vec2& point) const override;

private:
    /**
     * @brief Returns ColliderType::AABB.
     */
    [[nodiscard]] ColliderType GetType() const override { return ColliderType::AABB; }

    /**
     * @brief Returns a broad-phase bounding radius for the AABB.
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Performs narrow-phase collision test using double dispatch.
     */
    [[nodiscard]] bool CheckCollision(const Collider* other) const override;

    /**
     * @brief AABB-vs-circle narrow-phase collision.
     */
    [[nodiscard]] bool DispatchAgainst(const CircleCollider& other) const override;

    /**
     * @brief AABB-vs-AABB narrow-phase collision.
     */
    [[nodiscard]] bool DispatchAgainst(const AABBCollider& other) const override;

    /**
     * @brief Updates scaledHalfSize based on owner transform scale when enabled.
     */
    void SyncWithTransformScale() override;

    /**
     * @brief Draws the AABB collider for debugging.
     */
    void DrawDebug(RenderManager* rm, Camera2D* cam, const glm::vec4& color) const override;

    glm::vec2 baseHalfSize = { 0.5f, 0.5f };
    glm::vec2 scaledHalfSize = { 0.5f, 0.5f };
};

/**
 * @brief Hash function for glm::ivec2 used as a key in unordered containers.
 *
 * @details
 * Designed for SpatialHashGrid cell hashing.
 * Uses integer mixing with large primes to reduce collisions.
 */
struct Vec2Hash
{
    /**
     * @brief Computes a hash value for a grid cell coordinate.
     *
     * @param v Cell coordinate as an integer vector.
     * @return Hash value for the coordinate.
     */
    size_t operator()(const glm::ivec2& v) const
    {
        return std::hash<int>()(v.x * 73856093 ^ v.y * 19349663);
    }
};

/**
 * @brief Broad-phase collision structure using spatial hashing.
 *
 * @details
 * SpatialHashGrid divides world space into square cells (cellSize).
 * Objects are inserted into one or more cells based on their positions.
 *
 * During ComputeCollisions:
 * - The grid enumerates objects per cell and generates candidate pairs.
 * - The onCollision callback is invoked for each candidate pair
 *   (typically leading to narrow-phase collider checks).
 *
 * Large objects that exceed cell-based assumptions may be tracked separately
 * in largeObjects, allowing the system to handle them without excessive
 * multi-cell insertion.
 */
class SpatialHashGrid
{
    friend ObjectManager;
private:
    /**
     * @brief Clears all stored objects and grid cell mappings.
     */
    void Clear();

    /**
     * @brief Inserts an object into the spatial hash grid.
     *
     * @details
     * This function computes which cell(s) an object belongs to and stores it.
     * Objects that are considered large may be inserted into largeObjects instead.
     *
     * @param obj Object to insert.
     */
    void Insert(Object* obj);

    /**
     * @brief Computes broad-phase collision candidate pairs and invokes a callback.
     *
     * @details
     * The callback is responsible for deciding what to do with each candidate pair
     * (e.g., performing narrow-phase collision checks, resolving responses, etc.).
     *
     * @param onCollision Callback invoked with (Object*, Object*) candidate pair.
     */
    void ComputeCollisions(std::function<void(Object*, Object*)> onCollision);

    /**
     * @brief Converts a world position into a grid cell coordinate.
     *
     * @param pos World position.
     * @return Integer cell coordinate.
     */
    [[nodiscard]] glm::ivec2 GetCell(const glm::vec2& pos) const;

    /**
     * @brief Inserts an object into a specific cell bucket.
     *
     * @param obj Object to insert.
     * @param cell Cell coordinate.
     */
    void InsertToCell(Object* obj, const glm::ivec2& cell);

    int cellSize = 50;
    std::unordered_map<glm::ivec2, std::vector<Object*>, Vec2Hash> grid;
    std::vector<Object*> largeObjects;
    std::vector<Object*> objects;
};

/**
 * @brief Registry that assigns bit indices to collision group tags.
 *
 * @details
 * CollisionGroupRegistry maintains a stable mapping between string tags
 * (e.g., "Player", "Enemy", "Pickup") and a unique bit index.
 *
 * The registry supports:
 * - Allocating a new bit when a previously unseen tag is requested
 * - Reverse lookup from bit index back to tag for debugging/inspection
 *
 * This is typically used to build collision filtering masks
 * (group membership and interaction rules).
 */
class CollisionGroupRegistry
{
    friend Collider;
    friend Object;
private:
    /**
     * @brief Returns the bit index assigned to a group tag, allocating one if needed.
     *
     * @param tag Group tag string.
     * @return Bit index assigned to the tag.
     */
    [[nodiscard]] uint32_t GetGroupBit(const std::string& tag);

    /**
     * @brief Returns the tag string associated with a bit index.
     *
     * @param bit Bit index.
     * @return Tag string if known; otherwise an empty/unknown value depending on implementation.
     */
    [[nodiscard]] std::string GetGroupTag(uint32_t bit) const;

    std::unordered_map<std::string, uint32_t> tagToBit;
    std::unordered_map<uint32_t, std::string> bitToTag;
    uint32_t currentBit = 0;
};
