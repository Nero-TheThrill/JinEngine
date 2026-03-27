#include "InstancedObject.h"
#include <cmath>
#include <random>

static inline float TriWave(float theta)
{
	// tri(theta) = (2/pi) * asin(sin(theta))  in [-1, 1]
	return (2.0f / std::acos(-1)) * asinf(sinf(theta));
}

void LeafPosition(float t, const LeafParams& p, float& outX, float& outY)
{
    // 바닥 도달 시간
    float tStop = (p.y0 + 230.0f) / p.vY; // y(t) = y0 - vY * t 기준
    float tt = std::min(t, tStop);

    float amp = p.A0 * std::exp(-p.beta * tt);
    float theta = 2.0f * static_cast<float>(std::acos(-1)) * (tt / p.period) + p.phi;

    outX = p.x0 + p.vWind * tt + amp * TriWave(theta);
    outY = p.y0 - p.vY * tt;

    if (t > tStop)
    {
        // 멈추는 구간: y = -200에서 정지
        outY = -230.0f;
        // outX는 outX(tStop) 유지
    }
}

void InstancedObject::Init(const EngineContext& engineContext)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> x0Dist(-700.0f, -600.0f);
    static std::uniform_real_distribution<float> y0Dist(0.f, 10.0f);
    static std::uniform_real_distribution<float> vYDist(30.f, 80.0f);
    static std::uniform_real_distribution<float> vwindDist(-10.0f, 10.0f);
    static std::uniform_real_distribution<float> a0Dist(50.0f, 100.0f);
    static std::uniform_real_distribution<float> betaDist(0.1f, 0.8f);
    static std::uniform_real_distribution<float> periodDist(1.1f, 2.0f);

    leafParams.x0 = x0Dist(gen);
    leafParams.y0 = y0Dist(gen);
    leafParams.vY = vYDist(gen);
    leafParams.vWind = vwindDist(gen);
    leafParams.A0 = a0Dist(gen);
    leafParams.beta = betaDist(gen);
    leafParams.period = periodDist(gen);
    leafParams.phi = 0.0f;

    SetMaterial(engineContext, "[Material]Instancing");
    SetMesh(engineContext, "[EngineMesh]default");

    auto materialPtr = material.lock();
    auto meshPtr = mesh.lock();

    if (materialPtr && meshPtr)
    {
        materialPtr->EnableInstancing(true, meshPtr);
    }

    transform2D.SetScale({ 50,50 });
    SetRenderLayer("[Layer]UIText");

    float x = 0, y = 0;
    LeafPosition(timer, leafParams, x, y);
    transform2D.SetPosition({ x,y });

    SetColor({ 1,1,1,timer });
}
void InstancedObject::LateInit(const EngineContext& engineContext)
{
	GameObject::LateInit(engineContext);
}

void InstancedObject::Update(float dt, const EngineContext& engineContext)
{
    float x=0, y=0;
    LeafPosition(timer, leafParams, x, y);
    transform2D.SetPosition({ x,y });
    if (timer <= 1)
    {
        SetColor({ 1,1,1,timer });
    }
    if (timer>=8)
    {
        float alpha = 2.5f - timer * 0.15f;
        if (alpha >= 0)
            SetColor({ 1,1,1,2.5f - timer * 0.15f });
        else
            Kill();
    }
	timer += dt;
}

void InstancedObject::Draw(const EngineContext& engineContext)
{
	GameObject::Draw(engineContext);
}

void InstancedObject::Free(const EngineContext& engineContext)
{
	GameObject::Free(engineContext);
}

void InstancedObject::LateFree(const EngineContext& engineContext)
{
	GameObject::LateFree(engineContext);
}

void InstancedObject::OnCollision(Object* other)
{
	GameObject::OnCollision(other);
}
