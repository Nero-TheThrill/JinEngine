#pragma once
#include "EngineContext.h"
#include "Mesh.h"
#include "Object.h"
#include "Transform.h"

class Camera2D;

struct TextInstance
{
    std::weak_ptr<Font> font;
    std::string text = "";
};
class TextObject : public Object
{
public:
    TextObject(std::shared_ptr<Font> font, const std::string& text, TextAlignH alignH = TextAlignH::Left, TextAlignV alignV = TextAlignV::Top);

    ~TextObject() override = default;

    void Init([[maybe_unused]] const EngineContext& engineContext) override;

    void LateInit([[maybe_unused]] const EngineContext& engineContext) override;
    
    void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) override;
    
    void Draw([[maybe_unused]] const EngineContext& engineContext) override;
    
    void Free([[maybe_unused]] const EngineContext& engineContext) override;
    
    void LateFree([[maybe_unused]] const EngineContext& engineContext) override;

    [[nodiscard]] float GetBoundingRadius() const override;

    void SetText(const std::string& text);

    void SetTextInstance(const TextInstance& textInstance_);

    void SetAlignH(TextAlignH alignH_);

    void SetAlignV(TextAlignV alignV_);

    TextInstance* GetTextInstance();

    [[nodiscard]] bool HasAnimation() const override { return false; }

    [[nodiscard]] SpriteAnimator* GetSpriteAnimator() const override { return nullptr; }

    [[nodiscard]] glm::vec2 GetWorldPosition() const override;

    [[nodiscard]] glm::vec2 GetWorldScale() const override;

    void CheckFontAtlasAndMeshUpdate();

    void SetMaterial(const EngineContext& engineContext, const std::string& tag) = delete;

    void SetMaterial(std::shared_ptr<Material> material_) = delete;

    [[nodiscard]] std::shared_ptr<Material> GetMaterial() const = delete;

    void SetMesh(const EngineContext& engineContext, const std::string& tag) = delete;

    void SetMesh(std::shared_ptr<Mesh> mesh_) = delete;

    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const = delete;

    void AttachAnimator(std::unique_ptr<SpriteAnimator> anim) = delete;

    void AttachAnimator(SpriteSheet* sheet, float frameTime, bool loop = true) = delete;

    void DetachAnimator() = delete;
protected:

    void UpdateMesh();
    TextAlignH alignH;
    TextAlignV alignV;

    TextInstance textInstance;
    std::shared_ptr <Mesh> textMesh;

    int textAtlasVersionTracker = 0;
};
