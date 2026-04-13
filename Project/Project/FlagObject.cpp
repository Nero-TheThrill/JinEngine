#include "FlagObject.h"
#include "Engine.h"
void FlagObject::Init(const EngineContext& engineContext)
{
	SetMesh(engineContext, "[EngineMesh]default");
	SetMaterial(engineContext, "[Material]Animation");
	std::shared_ptr<SpriteSheet> sheet = engineContext.renderManager->GetSpriteSheetByTag("[SpriteSheet]Flag");
	//sheet->AddClip("[Clip]Idle", { 0,1,2,3,4 }, 0.15f, true);
	AttachAnimator(sheet, 0.2f);
	GetSpriteAnimator()->PlayClip(0, 4);
	auto collider = std::make_unique<AABBCollider>(this, glm::vec2(1.0, 1.0));
	collider->SetUseTransformScale(false);
	collider->SetSize({ 55,83 });
	collider->SetOffset({ glm::vec2(-15,-5.f) });
	SetCollider(std::move(collider));
	SetCollision(engineContext.stateManager->GetCurrentState()->GetObjectManager(), "[CollisionTag]flag", { "[CollisionTag]player" });

	textObject = static_cast<TextObject*>( engineContext.stateManager->GetCurrentState()->GetObjectManager().AddObject(
		std::make_unique<TextObject>(engineContext.renderManager->GetFontByTag("[Font]defaultkr"),guide,TextAlignH::Center,TextAlignV::Middle),
		"[Object]flagText"));
	textObject->SetRenderLayer("[Layer]UIText");
	textObject->GetTransform2D().SetPosition(GetWorldPosition()+glm::vec2{-20,150});
	textObject->GetTransform2D().SetScale({0.3f,0.3f});

	pannelObject = static_cast<GameObject*>(engineContext.stateManager->GetCurrentState()->GetObjectManager().AddObject(std::make_unique<GameObject>(), "[Object]pannelObject"));
	pannelObject->SetMaterial(engineContext, "[Material]ColorOnly");
	pannelObject->SetMesh(engineContext, "[EngineMesh]default");
	pannelObject->SetRenderLayer("[Layer]UI");
	pannelObject->GetTransform2D().SetScale(textObject->GetWorldScale()*glm::vec2(1.5f));
	pannelObject->GetTransform2D().SetPosition(textObject->GetWorldPosition());
	pannelObject->SetColor({0.3,0.3,0.3,1.f});
}

void FlagObject::LateInit(const EngineContext& engineContext)
{
	GameObject::LateInit(engineContext);
}

void FlagObject::Update(float dt, const EngineContext& engineContext)
{
	textObject->SetVisibility(showGuide);
	pannelObject->SetVisibility(showGuide);
	showGuide = false;
}

void FlagObject::Draw(const EngineContext& engineContext)
{
	GameObject::Draw(engineContext);
}

void FlagObject::Free(const EngineContext& engineContext)
{
	GameObject::Free(engineContext);
}

void FlagObject::LateFree(const EngineContext& engineContext)
{
	GameObject::LateFree(engineContext);
}

void FlagObject::OnCollision(Object* other, const EngineContext& engineContext)
{
	if (other->GetTag()=="[Object]player")
	{
		showGuide = true;
	}
}

void FlagObject::SetGuide(std::string guide_)
{
	guide = std::move(guide_);
}
