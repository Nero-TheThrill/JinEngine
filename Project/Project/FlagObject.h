#pragma once
#include "GameObject.h"
#include "TextObject.h"

class FlagObject : public GameObject
{
public:
    void Init(const EngineContext& engineContext) override;
    void LateInit(const EngineContext& engineContext) override;
    void Update(float dt, const EngineContext& engineContext) override;
    void Draw(const EngineContext& engineContext) override;
    void Free(const EngineContext& engineContext) override;
    void LateFree(const EngineContext& engineContext) override;
    void OnCollision(Object* other, const EngineContext& engineContext) override;
    void SetGuide(std::string guide_);

private:
    bool showGuide = false;
    std::string guide="hello\nworld!";
    TextObject* textObject;
    GameObject* pannelObject;
};
