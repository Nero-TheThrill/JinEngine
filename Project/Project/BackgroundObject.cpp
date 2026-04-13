#include "BackgroundObject.h"

#include "EngineContext.h"
#include "GameState.h"

void BackgroundObject::Init(const EngineContext& engineContext)
{
	SetMesh(engineContext, "[EngineMesh]default");
	
	GameObject::Init(engineContext);
}

void BackgroundObject::LateInit(const EngineContext& engineContext)
{
	GameObject::LateInit(engineContext);
}

void BackgroundObject::Update(float dt, const EngineContext& engineContext)
{
	Camera2D* cam = engineContext.stateManager->GetCurrentState()->GetActiveCamera();

	if (next->GetWorldPosition().x < GetWorldPosition().x  && GetWorldPosition().x + GetWorldScale().x/2.f < cam->GetPosition().x + cam->GetScreenWidth()/2.f)
	{
		next->SetBasePos({GetWorldPosition().x + GetWorldScale().x- cam->GetPosition().x * factor, GetWorldPosition().y});
	}
	if (next->GetWorldPosition().x > GetWorldPosition().x && next->GetWorldPosition().x + next->GetWorldScale().x / 2.f < cam->GetPosition().x + cam->GetScreenWidth() / 2.f)
	{
		SetBasePos({ next->GetWorldPosition().x + next->GetWorldScale().x- cam->GetPosition().x * factor, next->GetWorldPosition().y});
	}


	if (next->GetWorldPosition().x < GetWorldPosition().x && next->GetWorldPosition().x - next->GetWorldScale().x / 2.f > cam->GetPosition().x - cam->GetScreenWidth() / 2.f)
	{
		SetBasePos({ next->GetWorldPosition().x - next->GetWorldScale().x- cam->GetPosition().x * factor, next->GetWorldPosition().y });
	}
	if (next->GetWorldPosition().x > GetWorldPosition().x && GetWorldPosition().x - GetWorldScale().x / 2.f  > cam->GetPosition().x - cam->GetScreenWidth() / 2.f)
	{
		next->SetBasePos({ GetWorldPosition().x - GetWorldScale().x - cam->GetPosition().x * factor, GetWorldPosition().y });

	}
	transform2D.SetPosition(basePosition + cam->GetPosition() * factor);


	GameObject::Update(dt, engineContext);
}

void BackgroundObject::Draw(const EngineContext& engineContext)
{
	GameObject::Draw(engineContext);
}

void BackgroundObject::Free(const EngineContext& engineContext)
{
	GameObject::Free(engineContext);
}

void BackgroundObject::LateFree(const EngineContext& engineContext)
{
	GameObject::LateFree(engineContext);
}

void BackgroundObject::OnCollision(Object* other, const EngineContext& engineContext)
{
	GameObject::OnCollision(other, engineContext);
}

void BackgroundObject::SetFactor(float ftr)
{
	factor = ftr;
}

void BackgroundObject::SetBasePos(glm::vec2 pos)
{
	basePosition = pos;
}

void BackgroundObject::SetNextBackground(BackgroundObject* obj)
{
	next = obj;
}
