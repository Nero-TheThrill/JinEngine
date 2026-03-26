#pragma once
#include <functional>
#include "Engine.h"
#include "AsyncResourceLoader.h"

/**
 * @brief A GameState responsible for asynchronous resource loading with visual progress feedback.
 *
 * @details
 * LoadingState handles background loading of resources such as textures, shaders,
 * fonts, and sounds using AsyncResourceLoader. It displays a loading UI and
 * transitions to the next GameState once all resources are loaded.
 *
 * Core responsibilities:
 * - Queue resource loading jobs (CPU-side)
 * - Process completed jobs and upload them to GPU
 * - Display loading progress UI
 * - Transition to the next GameState when loading completes
 *
 * Loading flow:
 * 1. Load() → Starts loader and prepares UI resources
 * 2. Init() → Creates loading UI (TextObject)
 * 3. Update() →
 *    - Fetch completed jobs
 *    - Upload resources to GPU
 *    - Update progress UI
 *    - Transition when loading completes
 * 4. Draw() → Renders loading bar and text
 * 5. Free() → Stops loader thread
 *
 * @note
 * Resource loading is split into:
 * - CPU loading (AsyncResourceLoader thread)
 * - GPU upload (main thread via UploadToGPU)
 */
class LoadingState : public GameState
{
public:
    /**
     * @brief Constructs a LoadingState with a factory for the next GameState.
     *
     * @param nextFactory A function that creates the next GameState after loading completes.
     */
    explicit LoadingState(std::function<std::unique_ptr<GameState>()> nextFactory);

    /**
     * @brief Starts asynchronous loading and prepares UI resources.
     *
     * @details
     * - Starts AsyncResourceLoader thread
     * - Ensures UI font is registered
     * - Marks UI as ready for initialization
     */
    void Load(const EngineContext& engineContext) override;

    /**
     * @brief Initializes loading UI objects.
     *
     * @details
     * Creates a TextObject displaying loading progress.
     * The text is centered and ignores camera transform.
     */
    void Init(const EngineContext& engineContext) override;

    /**
     * @brief Performs late initialization after all objects are created.
     *
     * @details
     * Calls base GameState LateInit logic.
     */
    void LateInit(const EngineContext& engineContext) override;

    /**
     * @brief Updates loading progress and processes completed jobs.
     *
     * @details
     * - Fetches completed resource loading results
     * - Uploads them to GPU via UploadToGPU()
     * - Updates loading progress text
     * - Transitions to next GameState when loading finishes
     *
     * @param dt Delta time
     */
    void Update(float dt, const EngineContext& engineContext) override;

    /**
     * @brief Performs late update logic.
     *
     * @details
     * Calls base GameState LateUpdate.
     */
    void LateUpdate(float dt, const EngineContext& engineContext) override;

    /**
     * @brief Renders loading UI.
     *
     * @details
     * Draws:
     * - Background loading bar
     * - Filled progress bar
     * - Progress text
     */
    void Draw(const EngineContext& engineContext) override;

    /**
     * @brief Stops the loader and releases resources.
     *
     * @details
     * Stops AsyncResourceLoader thread safely.
     */
    void Free(const EngineContext& engineContext) override;

    /**
     * @brief Unloads state-specific resources.
     *
     * @details
     * Currently unused but reserved for future resource cleanup.
     */
    void Unload(const EngineContext& engineContext) override;

    /**
     * @brief Queues a texture loading job.
     *
     * @details
     * Adds a texture load request to AsyncResourceLoader if not already registered.
     *
     * @param engineContext Engine context
     * @param tag Resource tag
     * @param filePath Path to texture file
     * @param settings Texture settings
     */
    void QueueTexture(const EngineContext& engineContext, const std::string& tag, const std::string& filePath, const TextureSettings& settings = {});

    /**
     * @brief Queues a shader loading job.
     *
     * @details
     * Adds a shader load request consisting of multiple shader stages.
     *
     * @param engineContext Engine context
     * @param tag Resource tag
     * @param files List of shader stage + file path pairs
     */
    void QueueShader(const EngineContext& engineContext, const std::string& tag, const std::vector<std::pair<ShaderStage, FilePath>>& files);

    /**
     * @brief Queues a font loading job.
     *
     * @details
     * Adds a font load request using TTF file and pixel size.
     *
     * @param engineContext Engine context
     * @param tag Resource tag
     * @param ttfPath Path to font file
     * @param pixelSize Font size in pixels
     */
    void QueueFont(const EngineContext& engineContext, const std::string& tag, const std::string& ttfPath, uint32_t pixelSize);

    /**
     * @brief Queues a sound loading job.
     *
     * @details
     * Adds a sound load request to AsyncResourceLoader.
     *
     * @param tag Resource tag
     * @param path Path to sound file
     * @param loop Whether the sound should loop
     */
    void QueueSound(const std::string& tag, const std::string& path, bool loop);

    /**
     * @brief Returns loading progress as a normalized value.
     *
     * @return Progress in range [0.0, 1.0]
     */
    [[nodiscard]] float GetProgress01() const { return loader.GetProgress(); }

    /**
     * @brief Uploads a loaded resource to GPU.
     *
     * @details
     * Converts CPU-loaded data into GPU resources:
     * - Texture → OpenGL texture
     * - Shader → Compiled shader program
     * - Font → Registered font with atlas
     * - Sound → Loaded into SoundManager
     *
     * Must be called from main thread.
     *
     * @param r Load result from AsyncResourceLoader
     */
    void UploadToGPU(const LoadResult& r, const EngineContext& engineContext);

protected:
    AsyncResourceLoader loader;
    std::function<std::unique_ptr<GameState>()> nextFactory;

    std::string uiFontTag = "[Loading]UIFont";
    bool uiReady = false;
    TextObject* progressText = nullptr;
};