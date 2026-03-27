#pragma once
#include "BackgroundObject.h"
#include "FlagObject.h"
#include"GameState.h"
#include "Player.h"
#include "InstancedObject.h"
namespace TutorialState
{
    void AsyncLoad(const EngineContext& engineContext, LoadingState* loading);
}
class Tutorial :public GameState
{
public:
    void Load(const EngineContext& engineContext) override;

    void Init(const EngineContext& engineContext) override;

    void LateInit(const EngineContext& engineContext) override;

    void Update(float dt, const EngineContext& engineContext) override;

    void LateUpdate(float dt, const EngineContext& engineContext) override;

    void Draw(const EngineContext& engineContext) override;

    void PostProcessing(const EngineContext& engineContext) override;

    void Free(const EngineContext& engineContext) override;

    void Unload(const EngineContext& engineContext) override;

private:
    Player* player;
    BackgroundObject* bgObj00, * bgObj01, * bgObj02, * bgObj03, * bgObj04, * bgObj05, * bgObj06, * bgObj07, * bgObj08;
    BackgroundObject* bgObj00Sub, * bgObj01Sub, * bgObj02Sub, * bgObj03Sub, * bgObj04Sub, * bgObj05Sub, * bgObj06Sub, * bgObj07Sub, * bgObj08Sub;
    FlagObject* flag00, *flag01, *flag02, *flag03, *flag04;
    TextObject* text;
    GameObject* fbTexture, *cursor;
    float leafSpawnTimer = 0.0f;
    float timer = 0;
    bool enableGlitch = false;
    bool enableWaterDrop = true;

    std::shared_ptr<ComputeMaterial> computeMat;
    std::shared_ptr<Material> glitchMat;
    struct RippleCPU {
        glm::vec2 centerPx;
        float     startTime;
        float     amplitude;
        float     wavelength;
        float     speed;
        float     damping;
        float     life;
    };

    std::vector<RippleCPU> g_ripples;
};


