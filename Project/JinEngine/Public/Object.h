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
 * @brief Identifies the runtime category of an engine object.
 *
 * @details
 * This value is used to distinguish between general scene/renderable objects
 * and text-specific objects. Some engine systems, such as resource validation
 * or specialized rendering paths, may branch their behavior based on this type.
 */
enum class ObjectType
{
    GAME,
    TEXT
};

/**
 * @brief Base abstract object class for all engine-managed runtime objects.
 *
 * @details
 * Object is the common polymorphic foundation used by the engine to manage
 * scene entities such as gameplay objects and text objects.
 *
 * This class stores and exposes the core shared state needed by multiple systems:
 * - lifecycle state (alive / visible)
 * - transform data
 * - material and mesh references
 * - render layer tag
 * - tint color
 * - optional sprite animation
 * - optional collider
 * - collision category / mask bits
 * - camera-ignore behavior for HUD or screen-space style objects
 * - UV flip settings for mirrored rendering
 *
 * The renderer uses this class heavily during culling and draw submission.
 * For example:
 * - FrustumCuller checks IsAlive(), IsVisible(), ShouldIgnoreCamera(),
 *   GetWorldPosition(), and GetBoundingRadius()
 * - RenderManager reads material, mesh, animation, color, transform,
 *   layer tag, and UV flip state when building render batches and issuing draws
 *
 * Derived classes must implement the full lifecycle interface and may override
 * world-space queries when their positioning logic differs from the default
 * transform-based behavior.
 *
 * @note
 * This class is abstract and cannot be instantiated directly.
 */
class Object
{
    friend FrustumCuller;

public:
    /**
     * @brief Deleted default constructor.
     *
     * @details
     * Every Object must be created with an explicit ObjectType through the
     * protected constructor so the engine can distinguish its runtime category.
     */
    Object() = delete;

    /**
     * @brief Performs the object's initial setup phase.
     *
     * @details
     * This function is intended for the first initialization stage after the
     * object is created and inserted into the engine flow. Derived classes should
     * allocate or bind resources, initialize state, and prepare anything needed
     * before the object begins normal updates.
     *
     * The exact call timing depends on the object/state management flow used by
     * the engine.
     *
     * @param engineContext Shared engine services and managers available during initialization.
     */
    virtual void Init([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Performs a secondary initialization phase after Init().
     *
     * @details
     * This hook exists for objects that need a follow-up initialization step
     * after the main Init() phase has completed, such as resolving dependencies
     * on other objects or resources that are expected to exist by then.
     *
     * @param engineContext Shared engine services and managers available during late initialization.
     */
    virtual void LateInit([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Updates the object's main per-frame logic.
     *
     * @details
     * Derived classes should implement gameplay logic, animation progression,
     * movement, internal timers, and other frame-based behavior here.
     *
     * @param dt Delta time for the current frame.
     * @param engineContext Shared engine services and managers available during update.
     */
    virtual void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Executes object-specific draw preparation logic before the renderer submits data.
     *
     * @details
     * This function is called from the rendering path and is intended for
     * object-specific draw-time preparation. In the current engine flow,
     * RenderManager may call Draw() before sending material data and issuing
     * the final mesh draw call, so derived classes can use this hook to update
     * uniforms or perform other per-object render setup.
     *
     * Objects should not assume this function alone performs the full OpenGL draw.
     * The renderer still controls actual batch submission and mesh rendering.
     *
     * @param engineContext Shared engine services and managers available during drawing.
     */
    virtual void Draw([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Performs the object's primary cleanup phase.
     *
     * @details
     * Derived classes should release or detach runtime resources that are no longer
     * needed before the object is fully removed.
     *
     * @param engineContext Shared engine services and managers available during cleanup.
     */
    virtual void Free([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Performs a late cleanup phase after Free().
     *
     * @details
     * This hook exists for cases where destruction work should happen after the
     * main cleanup phase, such as deferred dependency release or cleanup ordering.
     *
     * @param engineContext Shared engine services and managers available during late cleanup.
     */
    virtual void LateFree([[maybe_unused]] const EngineContext& engineContext) = 0;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~Object() = default;

    /**
     * @brief Receives a collision notification with another object.
     *
     * @details
     * The default implementation does nothing. Derived classes may override this
     * to react to collision events after the collision system determines that a
     * contact has occurred.
     *
     * @param other The other object involved in the collision event.
     */
    virtual void OnCollision(Object* other) {}

    /**
     * @brief Returns whether the object is currently alive.
     *
     * @details
     * An object marked as not alive is skipped by systems such as visibility/culling
     * and collision insertion logic.
     *
     * @return True if the object is alive; otherwise false.
     */
    [[nodiscard]] const bool& IsAlive() const;

    /**
     * @brief Returns whether the object is currently visible.
     *
     * @details
     * Invisible objects are skipped by rendering-related systems even if they
     * otherwise remain alive in the engine.
     *
     * @return True if the object is visible; otherwise false.
     */
    [[nodiscard]] const bool& IsVisible() const;

    /**
     * @brief Sets whether the object should be considered visible by the renderer.
     *
     * @param _isVisible New visibility state.
     */
    void SetVisibility(bool _isVisible);

    /**
     * @brief Marks the object as no longer alive.
     *
     * @details
     * This does not directly delete the object. It only changes the alive flag,
     * allowing engine systems to ignore the object and later remove it safely
     * through their own lifecycle management flow.
     */
    void Kill();

    /**
     * @brief Assigns a user-defined object tag.
     *
     * @details
     * This tag can be used by gameplay logic or object management code to identify
     * the object semantically.
     *
     * @param tag New object tag string.
     */
    void SetTag(const std::string& tag);

    /**
     * @brief Returns the object's tag string.
     *
     * @return Reference to the stored object tag.
     */
    [[nodiscard]] const std::string& GetTag() const;

    /**
     * @brief Returns the render layer tag assigned to this object.
     *
     * @details
     * RenderManager uses this tag to resolve a layer ID through the render layer
     * manager when building render batches.
     *
     * @return Reference to the render layer tag.
     */
    [[nodiscard]] const std::string& GetRenderLayerTag() const;

    /**
     * @brief Assigns the render layer tag used during render batch construction.
     *
     * @param tag Render layer tag to assign.
     */
    void SetRenderLayer(const std::string& tag);

    /**
     * @brief Resolves and assigns a material from RenderManager using a resource tag.
     *
     * @details
     * This function looks up a registered material through
     * engineContext.renderManager and stores the returned shared resource as a
     * weak reference inside the object.
     *
     * @param engineContext Shared engine services used to access RenderManager.
     * @param tag Material resource tag registered in the renderer.
     */
    void SetMaterial(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Assigns a material directly.
     *
     * @details
     * The material is stored internally as a weak_ptr. The object does not own
     * the resource lifetime.
     *
     * @param material_ Shared material resource to reference.
     */
    void SetMaterial(std::shared_ptr<Material> material_) { material = material_; }

    /**
     * @brief Returns the currently assigned material resource.
     *
     * @details
     * The internally stored weak_ptr is locked before being returned. If the
     * resource has already expired, this returns nullptr.
     *
     * @return Shared pointer to the assigned material, or nullptr if expired.
     */
    [[nodiscard]] std::shared_ptr<Material> GetMaterial() const;

    /**
     * @brief Resolves and assigns a mesh from RenderManager using a resource tag.
     *
     * @details
     * This function looks up a registered mesh through engineContext.renderManager
     * and stores the returned shared resource as a weak reference inside the object.
     *
     * @param engineContext Shared engine services used to access RenderManager.
     * @param tag Mesh resource tag registered in the renderer.
     */
    void SetMesh(const EngineContext& engineContext, const std::string& tag);

    /**
     * @brief Assigns a mesh directly.
     *
     * @details
     * The mesh is stored internally as a weak_ptr. The object does not own
     * the resource lifetime.
     *
     * @param mesh_ Shared mesh resource to reference.
     */
    void SetMesh(std::shared_ptr<Mesh> mesh_) { mesh = mesh_; }

    /**
     * @brief Returns the currently assigned mesh resource.
     *
     * @details
     * The internally stored weak_ptr is locked before being returned. If the
     * resource has already expired, this returns nullptr.
     *
     * @return Shared pointer to the assigned mesh, or nullptr if expired.
     */
    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const;

    /**
     * @brief Returns whether this object can participate in instanced rendering.
     *
     * @details
     * This function checks:
     * - that the mesh weak_ptr can be locked
     * - that the material weak_ptr can be locked
     * - that the material reports instancing support
     *
     * It does not guarantee that the object will always be rendered through an
     * instanced batch, but it is used by the renderer as a quick eligibility test.
     *
     * @return True if both mesh/material exist and the material supports instancing.
     */
    [[nodiscard]] bool CanBeInstanced() const;

    /**
     * @brief Returns the object's current 2D transform matrix.
     *
     * @details
     * This is typically used by the renderer when preparing the model matrix for
     * normal or instanced rendering.
     *
     * @return Current transform matrix generated from Transform2D.
     */
    [[nodiscard]] glm::mat4 GetTransform2DMatrix();

    /**
     * @brief Returns mutable access to the internal Transform2D.
     *
     * @details
     * This allows derived classes or engine-side code to modify position, scale,
     * rotation, depth, and other transform data directly.
     *
     * @return Reference to the internal Transform2D instance.
     */
    [[nodiscard]] Transform2D& GetTransform2D();

    /**
     * @brief Sets the object's tint color.
     *
     * @details
     * The renderer reads this value and forwards it into material uniform data
     * during rendering.
     *
     * @param color_ New RGBA tint color.
     */
    void SetColor(const  glm::vec4& color_);

    /**
     * @brief Returns the object's tint color.
     *
     * @return Reference to the stored RGBA tint color.
     */
    [[nodiscard]] const glm::vec4& GetColor();

    /**
     * @brief Returns whether the object currently has a sprite animator attached.
     *
     * @return True if a SpriteAnimator exists; otherwise false.
     */
    [[nodiscard]] virtual bool HasAnimation() const { return spriteAnimator != nullptr; }

    /**
     * @brief Returns the attached sprite animator.
     *
     * @return Raw pointer to the attached SpriteAnimator, or nullptr if none is attached.
     */
    [[nodiscard]] virtual SpriteAnimator* GetSpriteAnimator() const { return spriteAnimator.get(); }

    /**
     * @brief Attaches an already-created sprite animator.
     *
     * @param anim Unique ownership of the animator to attach.
     */
    void AttachAnimator(std::unique_ptr<SpriteAnimator> anim) { spriteAnimator = std::move(anim); }

    /**
     * @brief Creates and attaches a sprite animator from a sprite sheet.
     *
     * @details
     * This helper constructs a SpriteAnimator in place using the given sprite sheet,
     * frame duration, and looping policy.
     *
     * @param sheet Sprite sheet used by the animator.
     * @param frameTime Duration of each frame.
     * @param loop Whether the animation should loop.
     */
    void AttachAnimator(std::shared_ptr<SpriteSheet> sheet, float frameTime, bool loop = true) { spriteAnimator = std::make_unique<SpriteAnimator>(sheet, frameTime, loop); }

    /**
     * @brief Detaches and destroys the currently attached sprite animator.
     */
    void DetachAnimator() { spriteAnimator = nullptr; }

    /**
     * @brief Assigns a collider to this object.
     *
     * @details
     * The collider becomes owned by the object through unique_ptr.
     *
     * @param c Collider instance to attach.
     */
    void SetCollider(std::unique_ptr<Collider> c) { collider = std::move(c); }

    /**
     * @brief Returns the currently attached collider.
     *
     * @return Raw pointer to the collider, or nullptr if none is attached.
     */
    [[nodiscard]] Collider* GetCollider() const { return collider.get(); }

    /**
     * @brief Configures the object's collision category and collision mask.
     *
     * @details
     * This function queries the collision group registry from ObjectManager,
     * converts the supplied group tags into bit values, stores the object's own
     * category bit, and builds a collision mask by OR-combining all target groups.
     *
     * The resulting values are later used by the collision system to decide
     * which object pairs should be tested.
     *
     * @param objectManager Object manager that owns the collision group registry.
     * @param tag Collision group tag representing this object's category.
     * @param checkCollisionList List of collision group tags this object wants to check against.
     */
    void SetCollision(ObjectManager& objectManager, const std::string& tag, const std::vector<std::string>& checkCollisionList);

    /**
     * @brief Returns the collision mask bits for this object.
     *
     * @return Bitmask describing which categories this object can collide with.
     */
    [[nodiscard]] uint32_t GetCollisionMask() const { return collisionMask; }

    /**
     * @brief Returns the collision category bit for this object.
     *
     * @return Bit representing this object's collision category.
     */
    [[nodiscard]] uint32_t GetCollisionCategory() const { return collisionCategory; }

    /**
     * @brief Returns whether the object should ignore the active render camera.
     *
     * @details
     * Objects that ignore the camera are treated specially by rendering and
     * culling logic, which is useful for HUD-like elements or screen-space objects.
     *
     * @return True if the object should ignore camera transforms.
     */
    [[nodiscard]] bool ShouldIgnoreCamera() const;

    /**
     * @brief Enables or disables camera-ignoring behavior.
     *
     * @details
     * When enabled, the object stores an optional reference camera used to compute
     * corrected world position and scale for rendering/culling.
     *
     * In the current implementation, the reference camera pointer is assigned only
     * when enabling camera-ignore mode.
     *
     * @param shouldIgnoreCamera True to ignore the active camera during rendering logic.
     * @param cameraForTransformCalc Optional reference camera used for world-space correction.
     */
    void SetIgnoreCamera(bool shouldIgnoreCamera, Camera2D* cameraForTransformCalc = nullptr);

    /**
     * @brief Returns this object's runtime type.
     *
     * @return The ObjectType assigned at construction.
     */
    [[nodiscard]] ObjectType GetType() const { return type; }

    /**
     * @brief Returns the camera reference used for camera-ignore correction.
     *
     * @return Pointer to the reference camera, or nullptr if none is assigned.
     */
    [[nodiscard]] Camera2D* GetReferenceCamera() const { return referenceCamera; }

    /**
     * @brief Returns the object's current world position.
     *
     * @details
     * By default, this returns Transform2D::GetPosition().
     *
     * If camera-ignore mode is enabled and a reference camera exists, the current
     * implementation applies a correction using the camera position and zoom so
     * that screen-space style objects can still participate in rendering and culling
     * in a consistent way.
     *
     * Derived classes may override this if they define world position differently.
     *
     * @return Current world position used by engine systems.
     */
    [[nodiscard]] virtual glm::vec2 GetWorldPosition() const;

    /**
     * @brief Returns the object's current world scale.
     *
     * @details
     * By default, this returns Transform2D::GetScale().
     *
     * If camera-ignore mode is enabled and a reference camera exists, the current
     * implementation divides the transform scale by camera zoom so the effective
     * screen-space size remains stable relative to the view.
     *
     * Derived classes may override this if they define scale differently.
     *
     * @return Current world scale used by engine systems.
     */
    [[nodiscard]] virtual glm::vec2 GetWorldScale() const;

    /**
     * @brief Enables or disables horizontal UV flipping.
     *
     * @param shouldFlip True to flip along the X axis.
     */
    void SetFlipUV_X(bool shouldFlip) { flipUV_X = shouldFlip; }

    /**
     * @brief Enables or disables vertical UV flipping.
     *
     * @param shouldFlip True to flip along the Y axis.
     */
    void SetFlipUV_Y(bool shouldFlip) { flipUV_Y = shouldFlip; }

    /**
     * @brief Returns the UV flip multiplier vector used by the renderer.
     *
     * @details
     * The returned vector is typically either (1,1), (-1,1), (1,-1), or (-1,-1).
     * RenderManager multiplies the model matrix by a scale derived from this value
     * before drawing.
     *
     * @return UV flip multiplier vector.
     */
    [[nodiscard]] glm::vec2 GetUVFlipVector() const;

protected:
    /**
     * @brief Constructs an object with an explicit runtime type.
     *
     * @param objectType Runtime type assigned to this object.
     */
    Object(ObjectType objectType) : type(objectType) {}

    ObjectType type;

    /**
     * @brief Returns an approximate bounding radius used for visibility testing.
     *
     * @details
     * The default implementation:
     * - reads the mesh's local half-size if a mesh exists
     * - falls back to a default half-size of (0.5, 0.5) otherwise
     * - multiplies the half-size by the current transform scale
     * - returns the vector length as an approximate world-space radius
     *
     * FrustumCuller uses this value together with GetWorldPosition() when deciding
     * whether an object is inside the current camera view.
     *
     * Derived classes may override this if they need a more accurate bound.
     *
     * @return Approximate world-space bounding radius.
     */
    [[nodiscard]] virtual float GetBoundingRadius() const;

    bool isAlive = true;
    bool isVisible = true;

    bool ignoreCamera = false;
    Camera2D* referenceCamera = nullptr;

    std::string objectTag;
    std::string renderLayerTag;

    Transform2D transform2D;
    std::weak_ptr<Material> material;
    std::weak_ptr<Mesh> mesh;

    glm::vec4 color = glm::vec4(1);

    std::unique_ptr<SpriteAnimator> spriteAnimator;
    std::unique_ptr<Collider> collider;

    uint32_t collisionCategory = 0;
    uint32_t collisionMask = 0;

    bool flipUV_X = false;
    bool flipUV_Y = false;
};