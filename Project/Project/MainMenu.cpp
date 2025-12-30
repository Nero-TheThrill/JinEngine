#include "MainMenu.h"
#include <iostream>

#include "CustomLoadingState.h"
#include "Debug.h"
#include "LoadingState.h"
#include "Tutorial.h"

void MainMenu::Load(const EngineContext& engineContext)
{
    JIN_LOG("[MainMenu] load called");
}

void MainMenu::Init(const EngineContext& engineContext)
{
    JIN_LOG("[MainMenu] init called");
    mainText = static_cast<TextObject*>( objectManager.AddObject(std::make_unique<TextObject>(engineContext.renderManager->GetFontByTag("[Font]defaultkr"),  u8"메인메뉴", TextAlignH::Center, TextAlignV::Middle)));
    cursor = static_cast<GameObject*>(objectManager.AddObject(std::make_unique<GameObject>()));
    cursor->SetMaterial(engineContext, "[Material]cursor");
    cursor->SetMesh(engineContext, "[EngineMesh]default");
    cursor->GetTransform2D().SetScale({30,30});
    cursor->SetRenderLayer("[Layer]Cursor");

    TextObject* tmpText = static_cast<TextObject*>(objectManager.AddObject(std::make_unique<TextObject>(engineContext.renderManager->GetFontByTag("[Font]default"), "Press N to continue", TextAlignH::Center, TextAlignV::Middle)));
    tmpText->GetTransform2D().SetPosition({ 0,-50 });
    tmpText->GetTransform2D().SetScale({ 0.35,0.35 });
}

void MainMenu::LateInit(const EngineContext& engineContext)
{
}

void MainMenu::Update(float dt, const EngineContext& engineContext)
{
    cursor->GetTransform2D().SetPosition(engineContext.inputManager->GetMouseWorldPos(cameraManager.GetActiveCamera())+glm::vec2(11,-11));
    if (engineContext.inputManager->IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || engineContext.inputManager->IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) || engineContext.inputManager->IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        cursor->SetColor({ 0.7f, 0.7f, 0.7f, 1.0f });
    }
    if (engineContext.inputManager->IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || engineContext.inputManager->IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE) || engineContext.inputManager->IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
    {
        cursor->SetColor({ 1.0f, 1.0f,1.0f,1.0f });
    }
    if (engineContext.inputManager->IsKeyReleased(KEY_N))
    {
        auto nextFactory = []() -> std::unique_ptr<GameState>
            {
                return std::make_unique<Tutorial>();
            };

        auto loading = std::make_unique<CustomLoadingState>(nextFactory);

        TutorialState::AsyncLoad(engineContext, loading.get());

        engineContext.stateManager->ChangeState(std::move(loading));
    }
    objectManager.UpdateAll(dt, engineContext);

}

void MainMenu::LateUpdate(float dt, const EngineContext& engineContext)
{
}

void MainMenu::Draw(const EngineContext& engineContext)
{
    objectManager.DrawAll(engineContext);
}


void MainMenu::Free(const EngineContext& engineContext)
{
    JIN_LOG("[MainMenu] free called");
}

void MainMenu::Unload(const EngineContext& engineContext)
{
    JIN_LOG("[MainMenu] unload called");
}
