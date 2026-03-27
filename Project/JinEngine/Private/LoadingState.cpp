#include "LoadingState.h"
#include "Engine.h"
LoadingState::LoadingState(std::function<std::unique_ptr<GameState>()> nextFactory_)
    : nextFactory(std::move(nextFactory_))
{
}

void LoadingState::Load(const EngineContext& engineContext)
{
    loader.Start();
    if (!engineContext.renderManager->HasFont(uiFontTag))
    {
        engineContext.renderManager->RegisterFont(uiFontTag, "Fonts/font1.ttf", 32);
    }
    uiReady = true;
}

void LoadingState::Init(const EngineContext& engineContext)
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

void LoadingState::LateInit(const EngineContext& engineContext)
{
    GameState::LateInit(engineContext);
}

void LoadingState::Update(float dt, const EngineContext& engineContext)
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

void LoadingState::LateUpdate(float dt, const EngineContext& engineContext)
{
    GameState::LateUpdate(dt, engineContext);
}

void LoadingState::Draw(const EngineContext& engineContext)
{
    const int w = engineContext.windowManager->GetWidth();
    const int h = engineContext.windowManager->GetHeight();

    const int barW = static_cast<int>(w * 0.6f);
    const int barH = 24;
    const int x = (w - barW) / 2;
    const int y = h / 2 - barH;

    engineContext.renderManager->ClearBackground(x, y, barW, barH, { 0.2f, 0.2f, 0.2f, 1.0f });

    int filled = static_cast<int>(barW * GetProgress01());
    if (filled > 0)
        engineContext.renderManager->ClearBackground(x, y, filled, barH, { 0.1f, 0.4f, 1.0f, 1.0f });
    objectManager.DrawAll(engineContext);
}

void LoadingState::Unload(const EngineContext& engineContext)
{
}

void LoadingState::Free(const EngineContext& engineContext)
{
    loader.Stop();
}

// ---------------- Queue API ----------------

void LoadingState::QueueTexture(const EngineContext& engineContext, const std::string& tag, const std::string& filePath, const TextureSettings& settings)
{
    if (engineContext.renderManager->HasTexture(tag))
        return;
    loader.Enqueue(TextureJob{ tag, filePath, settings });
}

void LoadingState::QueueShader(const EngineContext& engineContext, const std::string& tag, const std::vector<std::pair<ShaderStage, FilePath>>& files)
{
    if (engineContext.renderManager->HasShader(tag))
        return;
    ShaderJob job;
    job.tag = tag;
    for (auto& f : files)
        job.sources.push_back(ShaderFile{ f.first, f.second });
    loader.Enqueue(job);
}

void LoadingState::QueueFont(const EngineContext& engineContext, const std::string& tag, const std::string& ttfPath, uint32_t pixelSize)
{
    if (engineContext.renderManager->HasFont(tag))
        return;
    loader.Enqueue(FontJob{ tag, ttfPath, pixelSize });
}

void LoadingState::QueueSound(const std::string& tag, const std::string& path, bool loop)
{
    loader.Enqueue(SoundJob{ tag, path, loop });
}


// ---------------- GPU Upload ----------------

void LoadingState::UploadToGPU(const LoadResult& r, const EngineContext& engineContext)
{
    std::visit([&](auto&& res)
        {
            using T = std::decay_t<decltype(res)>;
            if constexpr (std::is_same_v<T, TextureResult>)
            {
                if (!res.ok)
                    return;

                auto tex = std::make_shared<Texture>(res.pixels.data(), res.width, res.height, res.channels, res.settings);
                engineContext.renderManager->RegisterTexture(res.tag, std::move(tex));
            }
            else if constexpr (std::is_same_v<T, ShaderResult>)
            {
                if (!res.ok)
                    return;

                auto shader = std::make_shared<Shader>();
                for (auto& s : res.sources)
                {
                    shader->AttachFromSource(s.stage, s.source);
                }
                if (!shader->Link())
                {
                    JIN_ERR("Shader link failed (tag=" << res.tag << ")");
                    return;
                }
                engineContext.renderManager->RegisterShader(res.tag, std::move(shader));
            }
            else if constexpr (std::is_same_v<T, FontResult>)
            {
                engineContext.renderManager->RegisterFont(res.tag, res.ttfPath, res.pixelSize);
            }
            else if constexpr (std::is_same_v<T, SoundResult>)
            {
                engineContext.soundManager->LoadSound(res.tag, res.filePath, res.loop);
            }
        }, r);
}
