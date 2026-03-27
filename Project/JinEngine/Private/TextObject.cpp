#include "Engine.h"

TextObject::TextObject(std::shared_ptr<Font> font, const std::string& text, TextAlignH alignH_, TextAlignV alignV_) : Object(ObjectType::TEXT)
{
    alignH = alignH_;
    alignV = alignV_;

    textInstance.font = font;
    textInstance.text = text;
    material = font->GetMaterial();

    UpdateMesh();
}

void TextObject::Init(const EngineContext& engineContext)
{
}

void TextObject::LateInit(const EngineContext& engineContext)
{
}

void TextObject::Update(float dt, const EngineContext& engineContext)
{
}

void TextObject::Draw(const EngineContext& engineContext)
{
}

void TextObject::Free(const EngineContext& engineContext)
{
}

void TextObject::LateFree(const EngineContext& engineContext)
{
}

float TextObject::GetBoundingRadius() const
{
    auto meshPtr = mesh.lock();
    if (!meshPtr) return 0.0f;

    auto fontPtr = textInstance.font.lock();
    if (!fontPtr) return 0.0f;

    glm::vec2 size = fontPtr->GetTextSize(textInstance.text);
    glm::vec2 scaled = size * transform2D.GetScale();

    return glm::length(scaled);
}

void TextObject::SetText(const std::string& text)
{
    if (textInstance.text == text)
        return;

    textInstance.text = text;

    UpdateMesh();
}

void TextObject::SetTextInstance(const TextInstance& textInstance_)
{
    auto currentFont = textInstance.font.lock();
    auto newFont = textInstance_.font.lock();

    if (textInstance.text == textInstance_.text && currentFont == newFont)
        return;

    textInstance = textInstance_;

    UpdateMesh();
}

void TextObject::SetAlignH(TextAlignH alignH_)
{
    if (alignH == alignH_)
        return;

    alignH = alignH_;

    UpdateMesh();
}

void TextObject::SetAlignV(TextAlignV alignV_)
{
    if (alignV == alignV_)
        return;

    alignV = alignV_;

    UpdateMesh();
}

TextInstance* TextObject::GetTextInstance()
{
    return &textInstance;
}

glm::vec2 TextObject::GetWorldPosition() const
{
    glm::vec2 screenSpace = transform2D.GetPosition();
    glm::vec2 offset(0.0f);

    auto fontPtr = textInstance.font.lock();

    if (fontPtr && !(alignH == TextAlignH::Center && alignV == TextAlignV::Middle))
    {
        glm::vec2 size = fontPtr->GetTextSize(textInstance.text) / glm::vec2(2, 2);

        if (alignH == TextAlignH::Left)
            offset.x = size.x;
        else if (alignH == TextAlignH::Right)
            offset.x = -size.x;

        if (alignV == TextAlignV::Top)
            offset.y = size.y;
        else if (alignV == TextAlignV::Bottom)
            offset.y = -size.y;
    }

    glm::vec2 alignedScreenPos = screenSpace + offset;

    if (ShouldIgnoreCamera() && referenceCamera)
    {
        float zoom = referenceCamera->GetZoom();
        glm::vec2 camPos = referenceCamera->GetPosition();
        return (camPos + alignedScreenPos) / zoom;
    }
    else
    {
        return alignedScreenPos;
    }
}

glm::vec2 TextObject::GetWorldScale() const
{
    auto fontPtr = textInstance.font.lock();
    if (!fontPtr)
        return glm::vec2(0.0f);

    glm::vec2 textSize = fontPtr->GetTextSize(textInstance.text);

    if (ShouldIgnoreCamera() && referenceCamera)
        return transform2D.GetScale() * textSize / referenceCamera->GetZoom();
    else
        return transform2D.GetScale() * textSize;
}

void TextObject::CheckFontAtlasAndMeshUpdate()
{
    auto fontPtr = textInstance.font.lock();
    if (!fontPtr)
    {
        mesh.reset();
        textMesh.reset();
        textAtlasVersionTracker = 0;
        return;
    }

    if (textAtlasVersionTracker == fontPtr->GetTextAtlasVersion())
        return;

    textAtlasVersionTracker = fontPtr->GetTextAtlasVersion();

    std::shared_ptr<Mesh> newMesh = fontPtr->GenerateTextMesh(textInstance.text, alignH, alignV);
    mesh = newMesh;
    textMesh = std::move(newMesh);
}

void TextObject::UpdateMesh()
{
    auto fontPtr = textInstance.font.lock();
    if (!fontPtr)
    {
        mesh.reset();
        textMesh.reset();
        textAtlasVersionTracker = 0;
        return;
    }

    std::shared_ptr<Mesh> newMesh = fontPtr->GenerateTextMesh(textInstance.text, alignH, alignV);
    mesh = newMesh;
    textMesh = std::move(newMesh);
    textAtlasVersionTracker = fontPtr->GetTextAtlasVersion();
}