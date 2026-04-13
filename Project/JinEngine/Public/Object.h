#pragma once
#include <memory>
#include <string>

#include "Animation.h"
#include "Collider.h"
#include "Mesh.h"
#include "Transform.h"
class FrustumCuller;
struct EngineContext;
enum class ObjectType
{
    GAME,
    TEXT
};
class Object
{
    friend FrustumCuller;
public:
    Object() = delete;
    virtual void Init([[maybe_unused]] const EngineContext& engineContext) = 0;

    virtual void LateInit([[maybe_unused]] const EngineContext& engineContext) = 0;

    virtual void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) = 0;

    virtual void Draw([[maybe_unused]] const EngineContext& engineContext) = 0;

    virtual void Free([[maybe_unused]] const EngineContext& engineContext) = 0;

    virtual void LateFree([[maybe_unused]] const EngineContext& engineContext) = 0;

    virtual ~Object() = default;

    virtual void OnCollision(Object* other, const EngineContext& engineContext) {}

    [[nodiscard]] const bool& IsAlive() const;

    [[nodiscard]] const bool& IsVisible() const;
    void SetVisibility(bool _isVisible);

    void Kill();

    void SetTag(const std::string& tag);
    [[nodiscard]] const std::string& GetTag() const;

    [[nodiscard]] const std::string& GetRenderLayerTag() const;
    void SetRenderLayer(const std::string& tag);

    void SetMaterial(const EngineContext& engineContext, const std::string& tag);

    void SetMaterial(std::shared_ptr<Material> material_) { material = material_; }

    [[nodiscard]] std::shared_ptr<Material> GetMaterial() const;

    void SetMesh(const EngineContext& engineContext, const std::string& tag);

    void SetMesh(std::shared_ptr<Mesh> mesh_) { mesh = mesh_; }

    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const;

    [[nodiscard]] bool CanBeInstanced() const;

    [[nodiscard]] glm::mat4 GetTransform2DMatrix();

    [[nodiscard]] Transform2D& GetTransform2D();

    void SetColor(const  glm::vec4& color_);
    [[nodiscard]] const glm::vec4& GetColor();

    [[nodiscard]] virtual bool HasAnimation() const { return spriteAnimator != nullptr; }
    [[nodiscard]] virtual SpriteAnimator* GetSpriteAnimator() const { return spriteAnimator.get(); }

    void AttachAnimator(std::unique_ptr<SpriteAnimator> anim) { spriteAnimator = std::move(anim); }
    void AttachAnimator(std::shared_ptr<SpriteSheet> sheet, float frameTime, bool loop = true) { spriteAnimator = std::make_unique<SpriteAnimator>(sheet, frameTime, loop); }
    void DetachAnimator() { spriteAnimator = nullptr; }

    void SetCollider(std::unique_ptr<Collider> c) { collider = std::move(c); }
    [[nodiscard]] Collider* GetCollider() const { return collider.get(); }
    void SetCollision(ObjectManager& objectManager, const std::string& tag, const std::vector<std::string>& checkCollisionList);

    [[nodiscard]] uint32_t GetCollisionMask() const { return collisionMask; }
    [[nodiscard]] uint32_t GetCollisionCategory() const { return collisionCategory; }

    [[nodiscard]] bool ShouldIgnoreCamera() const;
    void SetIgnoreCamera(bool shouldIgnoreCamera, Camera2D* cameraForTransformCalc = nullptr);

    [[nodiscard]] ObjectType GetType() const { return type; }

    [[nodiscard]] Camera2D* GetReferenceCamera() const { return referenceCamera; }

    [[nodiscard]] virtual glm::vec2 GetWorldPosition() const;
    [[nodiscard]] virtual glm::vec2 GetWorldScale() const;

    void SetFlipUV_X(bool shouldFlip) { flipUV_X = shouldFlip; }
    void SetFlipUV_Y(bool shouldFlip) { flipUV_Y = shouldFlip; }
    [[nodiscard]] glm::vec2 GetUVFlipVector() const;

protected:
    Object(ObjectType objectType) : type(objectType) {}
    ObjectType type;

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
