#include "Player.h"
#include "Engine.h"

void Player::Init(const EngineContext& engineContext)
{
    transform2D.SetScale(glm::vec2(100.f));
    SetMesh(engineContext, "[EngineMesh]default");
    SetMaterial(engineContext, "[Material]Animation");
    std::shared_ptr<SpriteSheet> sheet = engineContext.renderManager->GetSpriteSheetByTag("[SpriteSheet]MainCharacter");
    sheet->AddClip("[Clip]Idle", { 0,1,2,3,4,5,6,7 }, 0.15f, true);
    sheet->AddClip("[Clip]Running", {8,9,10,11,12,13}, 0.08f, true);
    sheet = engineContext.renderManager->GetSpriteSheetByTag("[SpriteSheet]MainCharacter1");
    sheet->AddClip("[Clip]Idle", { 0,1,2,3,4,5,6,7 }, 0.15f, true);
    sheet->AddClip("[Clip]Running", { 8,9,10,11,12,13 }, 0.08f, true);
    AttachAnimator(sheet, 0.08f);
    GetSpriteAnimator()->AddClipFrameCallback("[Clip]Idle", 0, [this]()
        {
            
            JIN_LOG("idle motion clip callback");
        });
    auto collider = std::make_unique<AABBCollider>(this, glm::vec2(1.0, 1.0));
    collider->SetUseTransformScale(false);
    collider->SetSize({ 90,70 });
    collider->SetOffset({ glm::vec2(0,10.f) });
    SetCollider(std::move(collider));
    SetCollision(engineContext.stateManager->GetCurrentState()->GetObjectManager(), "[CollisionTag]player", { "[CollisionTag]flag" });
}

void Player::LateInit(const EngineContext& engineContext)
{

}

void Player::Update(float dt, const EngineContext& engineContext)
{
    if (engineContext.inputManager->IsKeyReleased(KEY_Z))
    {
        AttachAnimator(engineContext.renderManager->GetSpriteSheetByTag("[SpriteSheet]MainCharacter1"), 0.08f);
        checkIdle_prevFrame = false;
        if (engineContext.inputManager->IsKeyDown(KEY_A) || engineContext.inputManager->IsKeyDown(KEY_D))
        {
            spriteAnimator->PlayClip("[Clip]Running");
        }
    }
    if (engineContext.inputManager->IsKeyReleased(KEY_X))
    {
        AttachAnimator(engineContext.renderManager->GetSpriteSheetByTag("[SpriteSheet]MainCharacter"), 0.08f);
        checkIdle_prevFrame = false;
        if (engineContext.inputManager->IsKeyDown(KEY_A) || engineContext.inputManager->IsKeyDown(KEY_D))
        {
            spriteAnimator->PlayClip("[Clip]Running");
        }
    }
    checkIdle = true;
    if (engineContext.inputManager->IsKeyDown(KEY_A))
    {
        checkIdle = false;
        checkIdle_prevFrame = false;
        transform2D.AddPosition(glm::vec2(-350 * dt, 0));
    }
    if (engineContext.inputManager->IsKeyDown(KEY_D))
    {
        checkIdle = false;
        checkIdle_prevFrame = false;
        transform2D.AddPosition(glm::vec2(350 * dt, 0));
    }

    if (spriteAnimator && engineContext.inputManager->IsKeyPressed(KEY_A))
    {
        static_cast<AABBCollider*>(collider.get())->SetSize({ 90,70 });
        collider->SetOffset({ glm::vec2(0,10.f) });
        SetFlipUV_X(true);
        spriteAnimator->PlayClip("[Clip]Running");
    }
    if (spriteAnimator && engineContext.inputManager->IsKeyPressed(KEY_D))
    {
        static_cast<AABBCollider*>(collider.get())->SetSize({ 90,70 });
        collider->SetOffset({ glm::vec2(0,10.f) });
        SetFlipUV_X(false);
        spriteAnimator->PlayClip("[Clip]Running");
    }

    if (spriteAnimator && checkIdle && !checkIdle_prevFrame)
    {
        static_cast<AABBCollider*>(collider.get())->SetSize({ 70,70 });
        collider->SetOffset({ glm::vec2(0,10.f) });
        checkIdle_prevFrame = true;
        spriteAnimator->PlayClip("[Clip]Idle");
    }
    Camera2D* cam = engineContext.stateManager->GetCurrentState()->GetActiveCamera();
    if (cam->GetPosition().x + cam->GetScreenWidth() / 2.f - 350.f < GetWorldPosition().x)
    {
        cam->AddPosition({ 350 * dt ,0 });
    }
    if (cam->GetPosition().x - cam->GetScreenWidth() / 2.f + 350.f > GetWorldPosition().x)
    {
        cam->AddPosition({ -350 * dt ,0 });
    }

}

void Player::Draw(const EngineContext& engineContext)
{
    //GetMaterial()->SetUniform("u_Color", glm::vec4(1.0, 1.0, 1.0, 1.0));
}

void Player::Free(const EngineContext& engineContext)
{
    JIN_LOG("Player Free Called");
}

void Player::LateFree(const EngineContext& engineContext)
{
    JIN_LOG("Player LateFree Called");
}

void Player::OnCollision(Object* other)
{
    //if (other->GetTag() == "enemyBullet")
    //{
    //    other->Kill();
    //}
    //if (other->GetTag() == "enemy" && this < other)
    //{
    //    glm::vec2 halfSize = GetWorldScale() / glm::vec2(2);
    //    glm::vec2 otherHalfSize = other->GetWorldScale() / glm::vec2(2);

    //    glm::vec2 center = GetWorldPosition() + halfSize;
    //    glm::vec2 otherCenter = other->GetWorldPosition() + otherHalfSize;

    //    glm::vec2 delta = center - otherCenter;
    //    glm::vec2 overlap = halfSize + otherHalfSize - glm::abs(delta);

    //    if (overlap.x > 0 && overlap.y > 0)
    //    {
    //        glm::vec2 correction = { 0, 0 };

    //        if (overlap.x < overlap.y)
    //        {
    //            correction.x = (delta.x > 0 ? overlap.x : -overlap.x) * 0.5f;
    //        }
    //        else
    //        {
    //            correction.y = (delta.y > 0 ? overlap.y : -overlap.y) * 0.5f;
    //        }
    //        if (!static_cast<Enemy*>(other)->CheckIdle())
    //            other->GetTransform2D().AddPosition(-correction);
    //        if (!checkIdle)
    //            GetTransform2D().AddPosition(correction);
				//
    //    }
    //}
    //if (other->GetTag() == "StartButton" || other->GetTag() == "QuitButton")
    //{
    //    other->SetColor({ 0.3,0.3,0.3,1.0 });
    //}
}

bool Player::CheckIdle()
{
    return checkIdle;
}
