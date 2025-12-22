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
 * @brief Identifies the runtime category of an Object.
 *
 * @details
 * The engine uses ObjectType to distinguish between gameplay objects and text objects.
 * Some systems (render submission, resource validation, font/mesh sync, etc.) may branch
 * behavior based on this type.
 */
enum class ObjectType
{
    GAME,
    TEXT
};

/**
 * @brief Base interface for all runtime entities managed by ObjectManager and rendered by RenderManager.
 *
 * @details
 * Object defines the engine-level lifecycle and the common component set used by the renderer and
 * collision system:
 * - Lifecycle: Init -> LateInit -> Update -> Draw -> Free -> LateFree
 * - Rendering: Transform2D, Material*, Mesh*, render layer tag, per-object color, optional SpriteAnimator
 * - Collision: optional Collider and collision category/mask filtering
 *
 * Ownership notes:
 * - ObjectManager owns Object instances via std::unique_ptr<Object>.
 * - Material and Mesh are referenced as raw pointers and are owned/managed by RenderManager.
 * - SpriteAnimator and Collider are owned by the Object via std::unique_ptr.
 *
 * Camera-space behavior:
 * - Objects can be configured to ignore the active camera and render in screen space.
 * - When ignoring the camera, a referenceCamera may be used to compute world transforms consistently.
 *
 * Visibility and lifetime:
 * - isAlive controls whether the object is considered active; killed objects are freed and removed by ObjectManager.
 * - isVisible controls whether the object participates in rendering and (typically) debug visualization.
 */
class Object
{
    friend FrustumCuller;
public:
    Object() = delete;

    /**
     * @brief Initializes the object.
     *
     * @details
     * Called once when the object is activated from the pending list.
     * Use this to request engine resources (materials, meshes, spritesheets) and initialize runtime state.
     *
     * @param engineContext Engine-wide context.
     */
    virtual void Init([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Second-phase initialization after all pending objects have completed Init().
     *
     * @details
     * This phase is intended for logic that depends on other objects existing and being initialized
     * (e.g., acquiring pointers from ObjectManager queries, registering callbacks, resolving references).
     *
     * @param engineContext Engine-wide context.
     */
    virtual void LateInit([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Per-frame update.
     *
     * @details
     * Called every frame while the object is alive. ObjectManager may perform additional engine-side
     * maintenance before/after this call (e.g., animator updates, collider scale sync).
     *
     * @param dt            Delta time in seconds.
     * @param engineContext Engine-wide context.
     */
    virtual void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Per-frame draw hook.
     *
     * @details
     * In this engine architecture, draw submission is typically handled by ObjectManager/RenderManager
     * using the object's Material/Mesh/Transform. This function exists for objects that need explicit
     * draw-time behavior or debug submission.
     *
     * @param engineContext Engine-wide context.
     */
    virtual void Draw([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Releases runtime resources owned by this object.
     *
     * @details
     * Called when the object is removed (killed) or when the owning GameState is being freed.
     * Use this to release or unregister external references and to clear owned data.
     *
     * @param engineContext Engine-wide context.
     */
    virtual void Free([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Second-phase free after all objects have completed Free().
     *
     * @details
     * Mirrors LateInit(). Use this for teardown that depends on other objects still existing during Free().
     *
     * @param engineContext Engine-wide context.
     */
    virtual void LateFree([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Virtual destructor for polymorphic deletion.
     */
    virtual ~Object() = default;

    /**
     * @brief Collision callback invoked when this object collides with another object.
     *
     * @details
     * Called by ObjectManager's collision pipeline after broad-phase and mask/category filtering,
     * and after the Collider narrow-phase test reports intersection.
     *
     * @param other The other colliding object.
     */
    virtual void OnCollision(Object* other) {}

    /**
     * @brief Returns whether the object is alive.
     *
     * @details
     * When false, ObjectManager will call Free/LateFree and remove the object from active lists.
     *
     * @return Reference to the alive flag.
     */
    [[nodiscard]] const bool& IsAlive() const;

    /**
     * @brief Returns whether the object is visible.
     *
     * @details
     * Invisible objects are typically skipped during render submission and debug drawing.
     *
     * @return Reference to the visibility flag.
     */
    [[nodiscard]] const bool& IsVisible() const;

    /**
     * @brief Sets the visibility state.
     *
     * @param _isVisible New visibility state.
     */
    void SetVisibility(bool _isVisible);

    /**
     * @brief Marks the object as dead.
     *
     * @details
     * Equivalent to setting isAlive to false. The object is not immediately destroyed; it will be
     * freed and removed by ObjectManager at the appropriate point in the frame.
     */
    void Kill();

    /**
     * @brief Sets the object tag used for lookup/grouping.
     *
     * @details
     * Tags are used by ObjectManager queries and can be used to draw or find subsets of objects.
     *
     * @param tag New tag string.
     */
    void SetTag(const std::string& tag);

    /**
     * @brief Returns the object tag.
     */
    [[nodiscard]] const std::string& GetTag() const;

    /**
     * @brief Returns the render-layer tag used for RenderLayerManager mapping.
     *
     * @details
     * RenderManager resolves this string into a numeric layer ID and uses it as the top-level ordering key.
     */
    [[nodiscard]] const std::string& GetRenderLayerTag() const;

    /**
     * @brief Sets the render-layer tag.
     *
     * @param tag Render-layer name/tag.
     */
    void SetRenderLayer(const std::string& tag);

    /**
     * @brief Sets the material by requesting it from RenderManager using a tag.
     *
     * @details
     * Uses engineContext to access RenderManager and resolves the material pointer by tag.
     * If the tag is missing, RenderManager typically provides a fallback default/error material.
     *
     * @param engineContext Engine-wide context.
     * @param tag           Registered material tag.
     */
    void SetMaterial(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Sets the material pointer directly.
     *
     * @details
     * The pointer is not owned by Object. It must remain valid for as long as the object uses it.
     *
     * @param material_ Material pointer.
     */
    void SetMaterial(Material* material_) { material = material_; }

    /**
     * @brief Returns the assigned material pointer.
     *
     * @return Material pointer (may be nullptr if not assigned).
     */
    [[nodiscard]] Material* GetMaterial() const;

    /**
     * @brief Sets the mesh by requesting it from RenderManager using a tag.
     *
     * @details
     * Uses engineContext to access RenderManager and resolves the mesh pointer by tag.
     * If the tag is missing, RenderManager typically provides a fallback default mesh.
     *
     * @param engineContext Engine-wide context.
     * @param tag           Registered mesh tag.
     */
    void SetMesh(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Sets the mesh pointer directly.
     *
     * @details
     * The pointer is not owned by Object. It must remain valid for as long as the object uses it.
     *
     * @param mesh_ Mesh pointer.
     */
    void SetMesh(Mesh* mesh_) { mesh = mesh_; }

    /**
     * @brief Returns the assigned mesh pointer.
     *
     * @return Mesh pointer (may be nullptr if not assigned).
     */
    [[nodiscard]] Mesh* GetMesh() const;

    /**
     * @brief Returns whether this object is eligible for instanced rendering.
     *
     * @details
     * Typical requirements in this engine:
     * - A valid Mesh and Material exist.
     * - The material's shader supports the instancing attribute layout (e.g., "i_Model").
     * - The object is not using per-object features that break batching.
     *
     * @return True if the object can be included in an instanced batch.
     */
    [[nodiscard]] bool CanBeInstanced() const;

    /**
     * @brief Returns the cached Transform2D matrix, updating it if necessary.
     *
     * @details
     * Uses Transform2D::GetMatrix() internally. When ignoreCamera is enabled, the returned matrix
     * may be computed relative to referenceCamera depending on SetIgnoreCamera configuration.
     *
     * @return Model matrix for this object.
     */
    [[nodiscard]] glm::mat4 GetTransform2DMatrix();

    /**
     * @brief Returns a mutable reference to the Transform2D component.
     */
    [[nodiscard]] Transform2D& GetTransform2D();

    /**
     * @brief Sets the per-object color used by the renderer.
     *
     * @param color_ New RGBA color.
     */
    void SetColor(const  glm::vec4& color_);

    /**
     * @brief Returns the current per-object color.
     */
    [[nodiscard]] const glm::vec4& GetColor();

    /**
     * @brief Returns whether the object currently has an animator attached.
     *
     * @details
     * Default implementation reports true when spriteAnimator is non-null.
     * TextObject overrides this to always return false.
     */
    [[nodiscard]] virtual bool HasAnimation() const { return spriteAnimator != nullptr; }

    /**
     * @brief Returns the active SpriteAnimator pointer, if any.
     *
     * @return SpriteAnimator pointer or nullptr.
     */
    [[nodiscard]] virtual SpriteAnimator* GetSpriteAnimator() const { return spriteAnimator.get(); }

    /**
     * @brief Attaches an animator by transferring ownership.
     *
     * @param anim Animator instance (ownership transferred).
     */
    void AttachAnimator(std::unique_ptr<SpriteAnimator> anim) { spriteAnimator = std::move(anim); }

    /**
     * @brief Creates and attaches a SpriteAnimator from a SpriteSheet and playback settings.
     *
     * @param sheet     Sprite sheet to use.
     * @param frameTime Seconds per frame.
     * @param loop      Whether to loop playback.
     */
    void AttachAnimator(SpriteSheet* sheet, float frameTime, bool loop = true) { spriteAnimator = std::make_unique<SpriteAnimator>(sheet, frameTime, loop); }

    /**
     * @brief Detaches and destroys the current animator, if any.
     */
    void DetachAnimator() { spriteAnimator = nullptr; }

    /**
     * @brief Sets the collider by transferring ownership.
     *
     * @param c Collider instance (ownership transferred).
     */
    void SetCollider(std::unique_ptr<Collider> c) { collider = std::move(c); }

    /**
     * @brief Returns the current collider pointer.
     *
     * @return Collider pointer or nullptr.
     */
    [[nodiscard]] Collider* GetCollider() const { return collider.get(); }

    /**
     * @brief Configures collision category and mask using ObjectManager's collision registry.
     *
     * @details
     * Resolves the object's collision group (category) using the provided tag, then builds a mask
     * from checkCollisionList. The resulting category/mask are used by ObjectManager to filter
     * collision candidate pairs before running narrow-phase checks.
     *
     * @param objectManager       ObjectManager providing the CollisionGroupRegistry.
     * @param tag                 Collision group tag to assign to this object.
     * @param checkCollisionList  List of collision group tags this object should test against.
     */
    void SetCollision(ObjectManager& objectManager, const std::string& tag, const std::vector<std::string>& checkCollisionList);

    /**
     * @brief Returns the collision mask used for collision filtering.
     */
    [[nodiscard]] uint32_t GetCollisionMask() const { return collisionMask; }

    /**
     * @brief Returns the collision category used for collision filtering.
     */
    [[nodiscard]] uint32_t GetCollisionCategory() const { return collisionCategory; }

    /**
     * @brief Returns whether this object should ignore the active camera.
     *
     * @details
     * When true, RenderManager treats the object as screen-space (view = identity) for rendering,
     * and FrustumCuller typically includes it without camera view tests.
     */
    [[nodiscard]] bool ShouldIgnoreCamera() const;

    /**
     * @brief Sets whether the object should ignore the active camera and optionally stores a reference camera.
     *
     * @details
     * When enabling ignoreCamera, referenceCamera can be provided so that world position/scale queries
     * remain consistent with a chosen camera basis (implementation-defined).
     *
     * @param shouldIgnoreCamera        New ignore-camera setting.
     * @param cameraForTransformCalc    Optional reference camera for transform calculations.
     */
    void SetIgnoreCamera(bool shouldIgnoreCamera, Camera2D* cameraForTransformCalc = nullptr);

    /**
     * @brief Returns the object type.
     */
    [[nodiscard]] ObjectType GetType() const { return type; }

    /**
     * @brief Returns the reference camera used when ignoreCamera is enabled.
     *
     * @return Camera pointer or nullptr.
     */
    [[nodiscard]] Camera2D* GetReferenceCamera() const { return referenceCamera; }

    /**
     * @brief Returns the object's world-space position used by systems like culling and debug drawing.
     *
     * @details
     * Default implementation reads Transform2D position and may adjust based on ignoreCamera/referenceCamera
     * depending on engine conventions.
     */
    [[nodiscard]] virtual glm::vec2 GetWorldPosition() const;

    /**
     * @brief Returns the object's world-space scale used by systems like culling and collision sync.
     *
     * @details
     * Default implementation reads Transform2D scale and may adjust based on ignoreCamera/referenceCamera
     * depending on engine conventions.
     */
    [[nodiscard]] virtual glm::vec2 GetWorldScale() const;

    /**
     * @brief Enables or disables horizontal UV flipping for this object.
     *
     * @param shouldFlip True to flip horizontally.
     */
    void SetFlipUV_X(bool shouldFlip) { flipUV_X = shouldFlip; }

    /**
     * @brief Enables or disables vertical UV flipping for this object.
     *
     * @param shouldFlip True to flip vertically.
     */
    void SetFlipUV_Y(bool shouldFlip) { flipUV_Y = shouldFlip; }

    /**
     * @brief Returns a vector encoding UV flip multipliers used by the renderer.
     *
     * @details
     * Common convention is:
     * - (1, 1) when not flipped
     * - (-1, 1) for X flip
     * - (1, -1) for Y flip
     * - (-1, -1) for both
     *
     * @return UV flip multiplier vector.
     */
    [[nodiscard]] glm::vec2 GetUVFlipVector() const;

protected:
    /**
     * @brief Constructs an Object with a concrete type.
     *
     * @details
     * Derived classes must call this constructor to set the object type.
     *
     * @param objectType Concrete object type.
     */
    Object(ObjectType objectType) : type(objectType) {}
    ObjectType type;

    /**
     * @brief Returns an approximate bounding radius used for camera visibility tests.
     *
     * @details
     * FrustumCuller uses this radius with GetWorldPosition() to decide whether the object is in view.
     * Derived classes can override to provide tighter bounds.
     *
     * @return Bounding radius in world units.
     */
    [[nodiscard]] virtual float GetBoundingRadius() const;

    bool isAlive = true;
    bool isVisible = true;

    bool ignoreCamera = false;
    Camera2D* referenceCamera = nullptr;

    std::string objectTag;
    std::string renderLayerTag;

    Transform2D transform2D;
    Material* material = nullptr;
    Mesh* mesh = nullptr;

    glm::vec4 color = glm::vec4(1);

    std::unique_ptr<SpriteAnimator> spriteAnimator;
    std::unique_ptr<Collider> collider;

    uint32_t collisionCategory = 0;
    uint32_t collisionMask = 0;

    bool flipUV_X = false;
    bool flipUV_Y = false;
};
