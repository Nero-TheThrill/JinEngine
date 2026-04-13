#pragma once
#include "GameObject.h"

class BackgroundObject : public GameObject
{
public:
	void Init(const EngineContext& engineContext) override;
	void LateInit(const EngineContext& engineContext) override;

	void Update(float dt, const EngineContext& engineContext) override;
	void Draw(const EngineContext& engineContext) override;

	void Free(const EngineContext& engineContext) override;
	void LateFree(const EngineContext& engineContext) override;

	void OnCollision(Object* other, const EngineContext& engineContext) override;

	void SetFactor(float ftr);

	void SetBasePos(glm::vec2 pos);

	void SetNextBackground(BackgroundObject* obj);
private:
	glm::vec2 basePosition;
	float factor = 0.0f;
	BackgroundObject* next;
};

