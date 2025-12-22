#pragma once
#include <functional>
#include "Engine.h"
#include "AsyncResourceLoader.h"

/**
 * @brief A transitional GameState that asynchronously loads resources and then switches to the next GameState.
 *
 * @details
 * LoadingState owns an AsyncResourceLoader and provides a queue-style API (QueueTexture/QueueShader/QueueFont/QueueSound)
 * for scheduling resource loads on worker threads. Each frame, it fetches completed load results and uploads them to
 * engine-managed systems (RenderManager/SoundManager).
 *
 * Flow (based on LoadingState.cpp):
 * - Load():
 *   - Starts the loader.
 *   - Ensures a UI font is registered (uiFontTag), then marks UI as ready.
 * - Init():
 *   - If UI is ready, creates a screen-space TextObject showing loading progress and registers it via ObjectManager.
 * - Update():
 *   - Fetches completed results and uploads them to GPU / systems.
 *   - Updates the progress text.
 *   - When loading finishes, fetches any remaining results, uploads them, then requests StateManager to change to nextFactory().
 * - Draw():
 *   - Draws a simple progress bar using RenderManager::ClearBackground and draws all objects (including the progress text).
 * - Free():
 *   - Stops the loader.
 *
 * nextFactory:
 * - A factory callback that creates the next GameState once loading has completed.
 */
class LoadingState : public GameState
{
public:
    /**
     * @brief Constructs a LoadingState that will transition to the GameState produced by nextFactory.
     *
     * @details
     * nextFactory is stored and invoked once the AsyncResourceLoader reports completion.
     * If nextFactory is null, the state will not automatically transition.
     *
     * @param nextFactory Factory function that returns the next GameState to run after loading completes.
     */
    explicit LoadingState(std::function<std::unique_ptr<GameState>()> nextFactory);

    /**
     * @brief Starts asynchronous loading and prepares UI resources used during the loading screen.
     *
     * @details
     * Starts AsyncResourceLoader. Ensures the loading UI font is registered in RenderManager under uiFontTag.
     * Sets uiReady to true so Init() can spawn UI objects.
     *
     * @param engineContext Engine-wide context providing access to RenderManager.
     */
    void Load(const EngineContext& engineContext) override;

    /**
     * @brief Initializes loading-screen objects (progress text).
     *
     * @details
     * If uiReady is true and the UI font can be retrieved, creates a TextObject displaying
     * "Loading... 0%" and configures it as screen-space by calling SetIgnoreCamera(true, GetActiveCamera()).
     * The created TextObject is added to the ObjectManager and its alignment is set to centered.
     *
     * @param engineContext Engine-wide context.
     */
    void Init(const EngineContext& engineContext) override;

    /**
     * @brief Late initialization hook for the loading state.
     *
     * @details
     * Forwards to GameState::LateInit(engineContext).
     *
     * @param engineContext Engine-wide context.
     */
    void LateInit(const EngineContext& engineContext) override;

    /**
     * @brief Fetches completed load jobs, uploads results to GPU/systems, updates UI, and transitions when finished.
     *
     * @details
     * Each frame:
     * - Calls loader.FetchCompleted() and uploads each LoadResult via UploadToGPU().
     * - Updates the progressText with the current percentage from GetProgress01().
     *
     * When loader.HasFinished() becomes true and nextFactory is valid:
     * - Fetches completed results again to flush any remaining uploads.
     * - Calls engineContext.stateManager->ChangeState(nextFactory()) to transition.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Engine-wide context.
     */
    void Update(float dt, const EngineContext& engineContext) override;

    /**
     * @brief Late update hook for the loading state.
     *
     * @details
     * Forwards to GameState::LateUpdate(dt, engineContext).
     *
     * @param dt Delta time in seconds.
     * @param engineContext Engine-wide context.
     */
    void LateUpdate(float dt, const EngineContext& engineContext) override;

    /**
     * @brief Draws the loading progress bar and renders loading-screen objects.
     *
     * @details
     * Draw() computes a centered progress bar rectangle based on the current window size.
     * It draws:
     * - a dark background bar
     * - a filled bar whose width is barW * GetProgress01()
     *
     * Then it draws all objects via ObjectManager::DrawAll(engineContext) (including progressText).
     *
     * @param engineContext Engine-wide context providing access to WindowManager and RenderManager.
     */
    void Draw(const EngineContext& engineContext) override;

    /**
     * @brief Releases resources owned by the loading state and stops the loader.
     *
     * @details
     * Calls loader.Stop(). Actual resource ownership is handled by RenderManager/SoundManager after upload.
     *
     * @param engineContext Engine-wide context.
     */
    void Free(const EngineContext& engineContext) override;

    /**
     * @brief Unloads the loading state.
     *
     * @details
     * Currently a no-op in the provided implementation.
     *
     * @param engineContext Engine-wide context.
     */
    void Unload(const EngineContext& engineContext) override;

    /**
     * @brief Enqueues an asynchronous texture load job if the texture tag is not already registered.
     *
     * @details
     * If RenderManager already has a texture registered for tag, the request is ignored.
     * Otherwise, a TextureJob is enqueued into AsyncResourceLoader.
     *
     * @param engineContext Engine-wide context (used to query RenderManager).
     * @param tag Texture tag to register under.
     * @param filePath File path for the texture to load.
     * @param settings Texture sampling/wrap/mipmap settings applied when creating the GPU texture.
     */
    void QueueTexture(const EngineContext& engineContext, const std::string& tag, const std::string& filePath, const TextureSettings& settings = {});

    /**
     * @brief Enqueues an asynchronous shader load job if the shader tag is not already registered.
     *
     * @details
     * If RenderManager already has a shader registered for tag, the request is ignored.
     * Otherwise, a ShaderJob is built from (stage, file path) pairs and enqueued.
     *
     * @param engineContext Engine-wide context (used to query RenderManager).
     * @param tag Shader tag to register under.
     * @param files List of shader stage + file path pairs to load and compile.
     */
    void QueueShader(const EngineContext& engineContext, const std::string& tag, const std::vector<std::pair<ShaderStage, FilePath>>& files);

    /**
     * @brief Enqueues an asynchronous font load job if the font tag is not already registered.
     *
     * @details
     * If RenderManager already has a font registered for tag, the request is ignored.
     * Otherwise, a FontJob is enqueued.
     *
     * @param engineContext Engine-wide context (used to query RenderManager).
     * @param tag Font tag to register under.
     * @param ttfPath Path to a .ttf file.
     * @param pixelSize Font pixel size used when creating the Font.
     */
    void QueueFont(const EngineContext& engineContext, const std::string& tag, const std::string& ttfPath, uint32_t pixelSize);

    /**
     * @brief Enqueues an asynchronous sound load job.
     *
     * @details
     * Unlike textures/shaders/fonts, this does not check for existing registrations in the provided implementation.
     * The SoundJob result is applied during UploadToGPU() by calling SoundManager::LoadSound(tag, filePath, loop).
     *
     * @param tag Sound tag.
     * @param path Sound file path.
     * @param loop Whether the sound should loop.
     */
    void QueueSound(const std::string& tag, const std::string& path, bool loop);

    /**
     * @brief Returns overall loading progress as a normalized value in [0, 1].
     *
     * @details
     * This forwards to AsyncResourceLoader::GetProgress().
     *
     * @return Loading progress ratio.
     */
    [[nodiscard]] float GetProgress01() const { return loader.GetProgress(); }

    /**
     * @brief Applies a completed load result to engine runtime systems (GPU upload / registration).
     *
     * @details
     * Uses std::visit on the LoadResult variant:
     * - TextureResult:
     *   - Creates a Texture from raw pixel data and registers it to RenderManager under the result tag.
     * - ShaderResult:
     *   - Creates a Shader, attaches sources, links it, then registers it to RenderManager.
     * - FontResult:
     *   - Registers the font to RenderManager via RegisterFont(tag, ttfPath, pixelSize).
     * - SoundResult:
     *   - Registers the sound via SoundManager::LoadSound(tag, filePath, loop).
     *
     * Failed results (ok == false) are skipped for TextureResult and ShaderResult.
     *
     * @param r Completed load result.
     * @param engineContext Engine-wide context providing RenderManager and SoundManager.
     */
    void UploadToGPU(const LoadResult& r, const EngineContext& engineContext);

protected:
    AsyncResourceLoader loader;
    std::function<std::unique_ptr<GameState>()> nextFactory;

    std::string uiFontTag = "[Loading]UIFont";
    bool uiReady = false;
    TextObject* progressText = nullptr;
};
