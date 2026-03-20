#include "CustomLoadingState.h"
#include "Engine.h"
CustomLoadingState::CustomLoadingState(std::function<std::unique_ptr<GameState>()> nextFactory_)
	:LoadingState(nextFactory_)
{
}

void CustomLoadingState::Load(const EngineContext& engineContext)
{
    loader.Start();
    if (!engineContext.renderManager->HasFont(uiFontTag))
    {
        engineContext.renderManager->RegisterFont(uiFontTag, "Fonts/font1.ttf", 32);
    }
    uiReady = true;
}

void CustomLoadingState::Init(const EngineContext& engineContext)
{
    if (uiReady)
    {
        std::shared_ptr<Font> font = engineContext.renderManager->GetFontByTag(uiFontTag);
        if (font)
        {
            auto text = std::make_unique<TextObject>(font, "Loading... 0%");
            text->SetIgnoreCamera(true, GetActiveCamera());
            text->GetTransform2D().SetPosition({ 0.0f, -50.0f });
            progressText = static_cast<TextObject*>(GetObjectManager().AddObject(std::move(text), "[Loading]ProgressText"));
            progressText->SetAlignH(TextAlignH::Center);
            progressText->SetAlignV(TextAlignV::Middle);
        }
    }
}

void CustomLoadingState::LateInit(const EngineContext& engineContext)
{
}

void CustomLoadingState::Update(float dt, const EngineContext& engineContext)
{
    auto done = loader.FetchCompleted();
    for (auto& r : done)
    {
        UploadToGPU(r, engineContext);
    }

    if (progressText)
    {
        int pct = static_cast<int>(GetProgress01() * 100.0f);
        progressText->SetText("Loading... " + std::to_string(pct) + "%");
    }

    if (loader.HasFinished() && nextFactory)
    {
        auto done = loader.FetchCompleted();
        for (auto& r : done)
        {
            UploadToGPU(r, engineContext);
        }
        engineContext.stateManager->ChangeState(nextFactory());
    }
}

void CustomLoadingState::LateUpdate(float dt, const EngineContext& engineContext)
{
}

void CustomLoadingState::Draw(const EngineContext& engineContext)
{

    const int w = engineContext.windowManager->GetWidth();
    const int h = engineContext.windowManager->GetHeight();

    const int barW = static_cast<int>(w * 0.6f);
    const int barH = 24;
    const int x = (w - barW) / 2;
    const int y = h / 2 - barH;
    engineContext.renderManager->ClearBackground(0, 0, w, h, { 0.2f, 0.2f, 0.3f, 1.0f });

    engineContext.renderManager->ClearBackground(x, y, barW, barH, { 0.5f, 0.2f, 0.2f, 1.0f });

    int filled = static_cast<int>(barW * GetProgress01());
    if (filled > 0)
        engineContext.renderManager->ClearBackground(x, y, filled, barH, { 0.2f, 0.5f, 0.8f, 1.0f});
    objectManager.DrawAll(engineContext);
}

void CustomLoadingState::Unload(const EngineContext& engineContext)
{
}

void CustomLoadingState::Free(const EngineContext& engineContext)
{
    loader.Stop();
}