#pragma once
#include "GameObject.h"
struct LeafParams
{
    float x0 = 0.0f;
    float y0 = 0.0f;
    float vY = 80.0f;            // 내려가는 속도 (>0)
    float vWind = 10.0f;         // 가로 드리프트 속도
    float A0 = 60.0f;            // 초기 진폭
    float beta = 0.4f;           // 감쇠율 (0이면 무감쇠)
    float period = 0.8f;         // T_osc
    float phi = 0.0f;            // 초기 위상
};

class InstancedObject : public GameObject
{
public:
    void Init(const EngineContext& engineContext) override;
    void LateInit(const EngineContext& engineContext) override;
    void Update(float dt, const EngineContext& engineContext) override;
    void Draw(const EngineContext& engineContext) override;
    void Free(const EngineContext& engineContext) override;
    void LateFree(const EngineContext& engineContext) override;
    void OnCollision(Object* other, const EngineContext& engineContext) override;
private:
    float timer = 0.0f;
    LeafParams leafParams;
};


