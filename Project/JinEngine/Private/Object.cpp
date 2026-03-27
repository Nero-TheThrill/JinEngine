#include "Engine.h"

const bool& Object::IsAlive() const
{
    return isAlive;
}

const bool& Object::IsVisible() const
{
    return isVisible;
}

void Object::SetVisibility(bool _isVisible)
{
    isVisible = _isVisible;
}

void Object::Kill()
{
    isAlive = false;
}

void Object::SetTag(const std::string& tag)
{
    objectTag = tag;
}

const std::string& Object::GetTag() const
{
    return objectTag;
}

const std::string& Object::GetRenderLayerTag() const
{
    return renderLayerTag;
}

void Object::SetRenderLayer(const std::string& tag)
{
    renderLayerTag = tag;
}

void Object::SetMaterial(const EngineContext& engineContext, const std::string& tag)
{
    material = engineContext.renderManager->GetMaterialByTag(tag);
}


std::shared_ptr<Material> Object::GetMaterial() const
{
    return material.lock();
}

void Object::SetMesh(const EngineContext& engineContext, const std::string& tag)
{
    mesh = engineContext.renderManager->GetMeshByTag(tag);
}

std::shared_ptr<Mesh> Object::GetMesh() const
{
    return mesh.lock();
}

bool Object::CanBeInstanced() const
{
    auto meshPtr = mesh.lock();
    auto materialPtr = material.lock();

    if (!meshPtr || !materialPtr) return false;
    if (!materialPtr->IsInstancingSupported()) return false;

    return true;
}
glm::mat4 Object::GetTransform2DMatrix()
{
    return transform2D.GetMatrix();
}

Transform2D& Object::GetTransform2D()
{
    return transform2D;
}

void Object::SetColor(const  glm::vec4& color_)
{
    color = color_;
}

const glm::vec4& Object::GetColor()
{
    return color;
}

void Object::SetCollision(ObjectManager& objectManager, const std::string& tag, const std::vector<std::string>& checkCollisionList)
{
    auto& reg = objectManager.GetCollisionGroupRegistry();
    collisionCategory = reg.GetGroupBit(tag);

    collisionMask = 0;
    for (const auto& c : checkCollisionList)
        collisionMask |= reg.GetGroupBit(c);
}

bool Object::ShouldIgnoreCamera() const
{
    return ignoreCamera;
}

void Object::SetIgnoreCamera(bool shouldIgnoreCamera, Camera2D* cameraForTransformCalc)
{
    ignoreCamera = shouldIgnoreCamera;
    if (ignoreCamera)
    {
        referenceCamera = cameraForTransformCalc;
    }
}

glm::vec2 Object::GetWorldPosition() const
{
    if (ShouldIgnoreCamera() && referenceCamera)
    {
        float zoom = referenceCamera->GetZoom();
        glm::vec2 camPos = referenceCamera->GetPosition();
        glm::vec2 screenSpace = transform2D.GetPosition();

        glm::vec2 offset = glm::vec2(1.0f / zoom);
        glm::vec2 corrected = (camPos+ screenSpace) * offset;

        return corrected;
    }
    else
    {
        return transform2D.GetPosition();
    }
}


glm::vec2 Object::GetWorldScale() const
{
    if (ShouldIgnoreCamera() && referenceCamera)
        return transform2D.GetScale() / referenceCamera->GetZoom();
    else
        return transform2D.GetScale();
}

glm::vec2 Object::GetUVFlipVector() const
{
    return { flipUV_X ? -1.0f : 1.0f, flipUV_Y ? -1.0f : 1.0f };
}


float Object::GetBoundingRadius() const
{
    auto meshPtr = mesh.lock();

    glm::vec2 halfSize = meshPtr ? meshPtr->GetLocalBoundsHalfSize() : glm::vec2(0.5f);
    glm::vec2 scaled = halfSize * transform2D.GetScale();
    return glm::length(scaled);
}