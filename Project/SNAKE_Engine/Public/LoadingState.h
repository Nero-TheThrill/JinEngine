#pragma once
#include <functional>
#include "Engine.h"
#include "AsyncResourceLoader.h"

/**
 * @brief Transitional game state that asynchronously loads resources before entering the next state.
 *
 * @details
 * LoadingState owns an AsyncResourceLoader and exposes queue functions to register textures, shaders,
 * fonts, and sounds. When all jobs are done it constructs the next GameState using @c nextFactory and hands off control to it.
 */
class LoadingState : public GameState
{
public:
    /**
     * @brief Construct a LoadingState with a factory for the next GameState.
     *
     * @details
     * The @p nextFactory is invoked after all queued resources have been loaded and uploaded.
     * It should return a fully constructed GameState instance that becomes active immediately.
     *
     * @param nextFactory A callable that returns std::unique_ptr<GameState> for the next state.
     */
    explicit LoadingState(std::function<std::unique_ptr<GameState>()> nextFactory);

    /**
     * @brief Prepare this state before it becomes active (enqueue initial jobs, pre-create UI).
     *
     * @details
     * Typical responsibilities:
     * - Prime the AsyncResourceLoader with jobs via QueueTexture/QueueShader/QueueFont/QueueSound.
     * - Optionally set up a progress TextObject using @c uiFontTag to display percentage.
     *
     * @param engineContext Engine-wide services and registries required by the loader and UI.
     */
    void Load(const EngineContext& engineContext) override;

    /**
     * @brief Initialize runtime objects (e.g., create TextObject for progress).
     *
     * @details
     * Create or acquire any engine objects the loading UI needs (fonts, text material, etc.).
     * Avoid heavy I/O here—prefer queuing in Load().
     *
     * @param engineContext Engine services used to create/register runtime objects.
     */
    void Init(const EngineContext& engineContext) override;

    /**
     * @brief Perform late initialization once this state is the current active state.
     *
     * @details
     * Use this to finalize UI placement, viewport dependent sizing, or to kick the loader thread
     * after graphics context is fully active.
     *
     * @param engineContext Engine services available after activation.
     */
    void LateInit(const EngineContext& engineContext) override;

    /**
     * @brief Advance asynchronous loading and upload finished assets; trigger state transition when done.
     *
     * @details
     * Steps usually include:
     * 1) Pump the AsyncResourceLoader to progress background jobs.
     * 2) Collect finished LoadResult items and call UploadToGPU() for each.
     * 3) Update progress text (0–100%) via GetProgress01().
     * 4) If all jobs are complete, construct the next state via @c nextFactory and request a state change.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Engine services used during polling and uploads.
     */
    void Update(float dt, const EngineContext& engineContext) override;

    /**
     * @brief Perform post-update work (optional), such as deferring GPU registrations or UI adjustments.
     *
     * @details
     * If your loader delivers results that must be staged and then committed, do the commit here.
     *
     * @param dt Delta time in seconds.
     * @param engineContext Engine services for any finalization tasks.
     */
    void LateUpdate(float dt, const EngineContext& engineContext) override;

    /**
     * @brief Render the loading screen (background, spinner, progress text).
     *
     * @details
     * Draw any background image, simple progress bar, and a TextObject showing percentage using @c uiFontTag.
     *
     * @param engineContext Engine rendering interfaces and registries.
     */
    void Draw(const EngineContext& engineContext) override;

    /**
     * @brief Release runtime objects created by this state (but keep shared engine systems alive).
     *
     * @details
     * Destroy UI nodes such as @c progressText and any temporary resources owned solely by this state.
     *
     * @param engineContext Engine services for destruction and deregistration.
     */
    void Free(const EngineContext& engineContext) override;

    /**
     * @brief Cancel outstanding jobs and detach from loader threads/resources.
     *
     * @details
     * Ensure the AsyncResourceLoader is drained or safely stopped before leaving the state stack.
     *
     * @param engineContext Engine services used for safe teardown.
     */
    void Unload(const EngineContext& engineContext) override;

    /**
     * @brief Enqueue a texture to be loaded asynchronously and registered by tag.
     *
     * @details
     * The texture is read from @p filePath with @p settings, then upon completion UploadToGPU()
     * registers it in engine resource tables using @p tag.
     *
     * @param engineContext Engine services required for final GPU registration.
     * @param tag Resource tag used later to retrieve the texture (e.g., via RenderManager).
     * @param filePath File system path to the texture image.
     * @param settings Optional texture creation parameters (format, filtering, etc.).
     */
    void QueueTexture(const EngineContext& engineContext, const std::string& tag, const std::string& filePath, const TextureSettings& settings = {});

    /**
     * @brief Enqueue a shader program to be built from multiple stage files and registered by tag.
     *
     * @details
     * Provide a list of (stage, file) pairs. When compilation/linking finishes off-thread,
     * UploadToGPU() installs the shader into the engine with @p tag.
     *
     * @param engineContext Engine services required for GPU program creation.
     * @param tag Resource tag used to fetch the shader later.
     * @param files Vector of (ShaderStage, FilePath) pairs for all required stages.
     */
    void QueueShader(const EngineContext& engineContext, const std::string& tag, const std::vector<std::pair<ShaderStage, FilePath>>& files);

    /**
     * @brief Enqueue a font to be loaded and baked for the specified pixel size, then registered by tag.
     *
     * @details
     * The font at @p ttfPath is loaded (e.g., via FreeType), baked to an atlas at @p pixelSize,
     * and registered under @p tag for later use by TextObject/UI rendering.
     *
     * @param engineContext Engine services used for font atlas creation and registration.
     * @param tag Resource tag to retrieve the font later.
     * @param ttfPath Path to a TrueType/OpenType font file.
     * @param pixelSize Target pixel size for glyph baking.
     */
    void QueueFont(const EngineContext& engineContext, const std::string& tag, const std::string& ttfPath, uint32_t pixelSize);

    /**
     * @brief Enqueue a sound to be loaded/decoded and registered by tag.
     *
     * @details
     * The sound at @p path is loaded asynchronously and, on completion, registered in the sound system
     * under @p tag. If @p loop is true, the default playback mode will be looped.
     *
     * @param tag Resource tag to retrieve the sound later.
     * @param path Path to the audio file (e.g., .wav, .ogg).
     * @param loop Whether the sound should be looped by default.
     */
    void QueueSound(const std::string& tag, const std::string& path, bool loop);

    /**
     * @brief Current loading progress in the range [0, 1].
     *
     * @details
     * Returns the normalized progress value as reported by the AsyncResourceLoader.
     *
     * @return A float in [0, 1], where 1 means all queued jobs are complete.
     */
    [[nodiscard]] float GetProgress01() const { return loader.GetProgress(); }

private:
    /**
     * @brief Register a finished LoadResult with engine systems (e.g., upload to GPU and tag it).
     *
     * @details
     * This internal helper inspects the LoadResult type (texture, shader, font, sound),
     * performs the appropriate GPU upload or engine registration, and binds the resource to its tag.
     * It is typically called from Update()/LateUpdate() when the loader reports completed jobs.
     *
     * @param r Completed load result produced by AsyncResourceLoader.
     * @param engineContext Engine services used to create GPU objects and register resources.
     */
    void UploadToGPU(const LoadResult& r, const EngineContext& engineContext);

private:
    AsyncResourceLoader loader;                                 ///< Background loader owned by this state.
    std::function<std::unique_ptr<GameState>()> nextFactory;    ///< Factory for the next GameState.

    std::string uiFontTag = "[Loading]UIFont";                  ///< Default tag for loading UI font.
    bool uiReady = false;                                       ///< True once the progress UI has been created.
    TextObject* progressText = nullptr;                         ///< Optional text node showing progress percentage.
};
