#pragma once
#include <memory>
#include <string>

#include "Animation.h"
#include "Collider.h"
#include "Mesh.h"
#include "Transform.h"
class FrustumCuller;
struct EngineContext;

/**
 * @brief Runtime type tag for objects handled by the engine.
 */
enum class ObjectType
{
    GAME,  ///< Regular game object rendered with material/mesh.
    TEXT   ///< Text-only object (e.g., TextObject).
};

/**
 * @brief Base class for all renderable and updatable objects.
 *
 * @details
 * Object provides the common lifecycle (Init, Update, Draw, Free), transform,
 * material/mesh binding, color, animation, collision, and camera-ignoring/HUD
 * features. Derived classes must implement the lifecycle functions and may
 * override world-space queries and bounding metrics for culling and physics.
 *
 * @note
 * Instances are owned by ObjectManager (unique_ptr). Rendering is coordinated
 * by RenderManager. Collider membership and broad-phase participation are
 * configured via SetCollision() and the CollisionGroupRegistry.
 */
class Object
{
    friend FrustumCuller;
public:
    /**
     * @brief Default constructor is not allowed; derived classes must choose a type.
     */
    Object() = delete;

    /**
     * @brief Initializes resources and state for the object.
     *
     * @details
     * Called exactly once after the object is added to the scene and before the first update.
     * Use this to bind materials/meshes, set initial transform, and allocate transient data.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    virtual void Init([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Late initialization hook.
     *
     * @details
     * Invoked after Init() for all objects. Use this for cross-object references
     * or operations that assume other objects are already initialized.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    virtual void LateInit([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Per-frame update.
     *
     * @details
     * Use this to advance gameplay state, run timers, update animation state,
     * and modify the transform. Avoid GPU calls here; prefer RenderManager for rendering.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Read-only access to engine subsystems.
     */
    virtual void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Per-frame draw hook.
     *
     * @details
     * Submit draw data (material, mesh, transform, per-object uniforms) to RenderManager.
     * Heavy GPU state changes should be avoided. For text objects, the mesh is internally owned.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    virtual void Draw([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Pre-destruction cleanup.
     *
     * @details
     * Called once when the object is about to be removed from the scene. Release
     * transient resources and unregister any callbacks.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    virtual void Free([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Late cleanup hook.
     *
     * @details
     * Invoked after Free() for all objects. Use this to break remaining references
     * or flush deferred tasks.
     *
     * @param engineContext Read-only access to engine subsystems.
     */
    virtual void LateFree([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Virtual destructor.
     */
    virtual ~Object() = default;

    /**
     * @brief Collision callback.
     *
     * @details
     * Called during the narrow-phase when this object is confirmed to collide with @p other.
     * Override to implement game-specific responses.
     *
     * @param other The other object involved in the collision.
     */
    virtual void OnCollision(Object* other) {}

    /**
     * @brief Checks whether the object is alive (eligible for update/draw).
     *
     * @return Reference to the alive flag (true = alive).
     */
    [[nodiscard]] const bool& IsAlive() const;

    /**
     * @brief Checks whether the object should be rendered.
     *
     * @return Reference to the visibility flag (true = visible).
     */
    [[nodiscard]] const bool& IsVisible() const;

    /**
     * @brief Enables or disables object visibility.
     *
     * @param _isVisible True to show, false to hide (still updated).
     */
    void SetVisibility(bool _isVisible);

    /**
     * @brief Marks the object as dead and schedules its removal.
     *
     * @details
     * After Kill(), ObjectManager will erase the object during cleanup.
     */
    void Kill();

    /**
     * @brief Assigns a string tag to the object for lookups.
     *
     * @param tag Arbitrary identifier (e.g., "[Object]Player").
     */
    void SetTag(const std::string& tag);

    /**
     * @brief Returns the object's tag.
     *
     * @return const std::string& Tag string.
     */
    [[nodiscard]] const std::string& GetTag() const;

    /**
     * @brief Returns the render-layer tag used for batching/sorting.
     *
     * @return const std::string& Registered render-layer tag.
     */
    [[nodiscard]] const std::string& GetRenderLayerTag() const;

    /**
     * @brief Assigns the object to a named render layer.
     *
     * @details
     * The tag must be registered in RenderLayerManager. Objects on the same layer
     * are batched/sorted together (layer first, then per-layer depth).
     *
     * @param tag Render-layer tag (e.g., "[Layer]UI").
     */
    void SetRenderLayer(const std::string& tag);

    /**
     * @brief Binds a material by tag from RenderManager.
     *
     * @param engineContext Engine context used to access RenderManager.
     * @param tag Registered material tag.
     */
    void SetMaterial(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Directly assigns a material pointer.
     *
     * @param material_ Non-owning material pointer (must outlive this object).
     */
    void SetMaterial(Material* material_) { material = material_; }

    /**
     * @brief Returns the currently assigned material.
     *
     * @return Material pointer (nullable).
     */
    [[nodiscard]] Material* GetMaterial() const;

    /**
     * @brief Binds a mesh by tag from RenderManager.
     *
     * @param engineContext Engine context used to access RenderManager.
     * @param tag Registered mesh tag.
     */
    void SetMesh(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Directly assigns a mesh pointer.
     *
     * @param mesh_ Non-owning mesh pointer (must outlive this object).
     */
    void SetMesh(Mesh* mesh_) { mesh = mesh_; }

    /**
     * @brief Returns the currently assigned mesh.
     *
     * @return Mesh pointer (nullable).
     */
    [[nodiscard]] Mesh* GetMesh() const;

    /**
     * @brief Indicates whether this object can be drawn via GPU instancing.
     *
     * @details
     * True when the assigned material/shader and mesh support instancing and the
     * object does not require per-instance unique state that prevents batching.
     *
     * @return True if instancing is supported.
     */
    [[nodiscard]] bool CanBeInstanced() const;

    /**
     * @brief Returns the 2D model matrix composed from Transform2D.
     *
     * @details
     * Lazily recomputes the T*R*S matrix when transform properties change.
     *
     * @return 4x4 transform matrix.
     */
    [[nodiscard]] glm::mat4 GetTransform2DMatrix();

    /**
     * @brief Access to the 2D transform (position/rotation/scale/depth).
     *
     * @return Reference to the Transform2D component.
     */
    [[nodiscard]] Transform2D& GetTransform2D();

    /**
     * @brief Sets a per-object color (e.g., tint/alpha).
     *
     * @param color_ RGBA color.
     */
    void SetColor(const  glm::vec4& color_);

    /**
     * @brief Returns the currently assigned per-object color.
     *
     * @return const glm::vec4& RGBA color reference.
     */
    [[nodiscard]] const glm::vec4& GetColor();

    /**
     * @brief Indicates if this object has a sprite animator attached.
     *
     * @return True if SpriteAnimator exists.
     */
    [[nodiscard]] virtual bool HasAnimation() const { return spriteAnimator != nullptr; }

    /**
     * @brief Returns the attached SpriteAnimator (if any).
     *
     * @return Pointer to SpriteAnimator or nullptr.
     */
    [[nodiscard]] virtual SpriteAnimator* GetSpriteAnimator() const { return spriteAnimator.get(); }

    /**
     * @brief Attaches an existing SpriteAnimator.
     *
     * @param anim Unique ownership of a SpriteAnimator.
     */
    void AttachAnimator(std::unique_ptr<SpriteAnimator> anim) { spriteAnimator = std::move(anim); }

    /**
     * @brief Creates and attaches a SpriteAnimator from a sheet.
     *
     * @param sheet Sprite sheet describing frames.
     * @param frameTime Seconds per frame.
     * @param loop Whether the animation loops (default true).
     */
    void AttachAnimator(SpriteSheet* sheet, float frameTime, bool loop = true) { spriteAnimator = std::make_unique<SpriteAnimator>(sheet, frameTime, loop); }

    /**
     * @brief Detaches and destroys the current SpriteAnimator.
     */
    void DetachAnimator() { spriteAnimator = nullptr; }

    /**
     * @brief Assigns a collider to this object.
     *
     * @param c Unique ownership of a Collider.
     */
    void SetCollider(std::unique_ptr<Collider> c) { collider = std::move(c); }

    /**
     * @brief Returns the collider (if any).
     *
     * @return Collider pointer or nullptr.
     */
    [[nodiscard]] Collider* GetCollider() const { return collider.get(); }

    /**
     * @brief Registers this object into collision groups and masks.
     *
     * @details
     * Looks up bitmasks by tag in CollisionGroupRegistry and applies @p checkCollisionList
     * as the mask filter. Required to participate in collision queries.
     *
     * @param objectManager Manager providing access to the registry.
     * @param tag Group tag/category for this object.
     * @param checkCollisionList List of group names to collide against.
     */
    void SetCollision(ObjectManager& objectManager, const std::string& tag, const std::vector<std::string>& checkCollisionList);

    /**
     * @brief Returns the collision bitmask of groups this object collides with.
     */
    [[nodiscard]] uint32_t GetCollisionMask() const { return collisionMask; }

    /**
     * @brief Returns the collision category bit for this object.
     */
    [[nodiscard]] uint32_t GetCollisionCategory() const { return collisionCategory; }

    /**
     * @brief Internal check: whether this object ignores the camera transform.
     *
     * @details
     * Intended for engine-side logic (RenderManager, culling). External code
     * usually does not need to call this directly.
     *
     * @return True if draw should ignore camera (HUD).
     */
    [[nodiscard]] bool ShouldIgnoreCamera() const;

    /**
     * @brief Enables HUD-style rendering that ignores the camera transform.
     *
     * @details
     * When enabled, positions/scales are interpreted in screen space using the
     * provided @p cameraForTransformCalc to resolve DPI/zoom if necessary.
     *
     * @param shouldIgnoreCamera True to ignore camera.
     * @param cameraForTransformCalc Optional camera reference to compute transforms.
     */
    void SetIgnoreCamera(bool shouldIgnoreCamera, Camera2D* cameraForTransformCalc = nullptr);

    /**
     * @brief Returns the runtime object type (GAME/TEXT).
     */
    [[nodiscard]] ObjectType GetType() const { return type; }

    /**
     * @brief Returns the reference camera used when ignoring camera transforms.
     *
     * @return Camera pointer or nullptr.
     */
    [[nodiscard]] Camera2D* GetReferenceCamera() const { return referenceCamera; }

    /**
     * @brief Returns the world-space position of the object anchor.
     *
     * @details
     * Default implementation forwards Transform2D position (world space).
     * TextObject overrides to account for alignment/HUD mode.
     *
     * @return World position.
     */
    [[nodiscard]] virtual glm::vec2 GetWorldPosition() const;

    /**
     * @brief Returns the world-space scale of the object.
     *
     * @details
     * Default implementation forwards Transform2D scale (world scale).
     * TextObject overrides to account for camera ignoring and pixel size.
     *
     * @return World scale (extent).
     */
    [[nodiscard]] virtual glm::vec2 GetWorldScale() const;

    /**
     * @brief Flips the object's UVs horizontally for sprite mirroring.
     *
     * @param shouldFlip True to flip on X.
     */
    void SetFlipUV_X(bool shouldFlip) { flipUV_X = shouldFlip; }

    /**
     * @brief Flips the object's UVs vertically for sprite mirroring.
     *
     * @param shouldFlip True to flip on Y.
     */
    void SetFlipUV_Y(bool shouldFlip) { flipUV_Y = shouldFlip; }

    /**
     * @brief Returns a vector encoding UV flip (-1 or +1 per axis).
     *
     * @return vec2 where x<0 flips horizontally, y<0 flips vertically.
     */
    [[nodiscard]] glm::vec2 GetUVFlipVector() const;

protected:
    /**
     * @brief Protected constructor; derived classes must specify an object type.
     *
     * @param objectType Runtime type tag (GAME or TEXT).
     */
    Object(ObjectType objectType) : type(objectType) {}

    ObjectType type;          ///< Object type (GAME, TYPE)

    /**
     * @brief Approximates a conservative bounding radius for culling/physics.
     *
     * @details
     * Default implementation derives radius from Transform2D scale and mesh bounds.
     * Override for tighter bounds (e.g., text width/height or capsules).
     *
     * @return Bounding radius in world units.
     */
    [[nodiscard]] virtual float GetBoundingRadius() const;

    bool isAlive = true;      ///< Alive flag; dead objects are removed by ObjectManager.
    bool isVisible = true;    ///< Visibility flag; hidden objects still Update().

    bool ignoreCamera = false;          ///< If true, draw ignores camera transform (HUD).
    Camera2D* referenceCamera = nullptr;///< Reference camera for HUD transform calculations.

    std::string objectTag;              ///< User-defined identifier.
    std::string renderLayerTag;         ///< Name-based render layer.

    Transform2D transform2D;            ///< 2D transform (position/rotation/scale/depth).
    Material* material = nullptr;       ///< Non-owning material pointer.
    Mesh* mesh = nullptr;               ///< Non-owning mesh pointer.

    glm::vec4 color = glm::vec4(1);     ///< Per-object color (tint/alpha).

    std::unique_ptr<SpriteAnimator> spriteAnimator; ///< Optional sprite animation component.
    std::unique_ptr<Collider> collider;             ///< Optional collision component.

    uint32_t collisionCategory = 0;     ///< Collision category bit.
    uint32_t collisionMask = 0;         ///< Collision mask bits (which groups to test against).

    bool flipUV_X = false;              ///< Flip UV horizontally.
    bool flipUV_Y = false;              ///< Flip UV vertically.
};
