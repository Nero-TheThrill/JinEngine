#pragma once
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <variant>
#include "Shader.h"
#include "Texture.h"

/**
 * @brief Job for asynchronously loading a texture from disk.
 */
struct TextureJob
{
    std::string tag;              ///< Identifier tag for the texture.
    std::string filePath;         ///< Path to the texture file.
    TextureSettings settings{};   ///< Texture sampling/wrapping/mipmap settings.
};

/**
 * @brief A single shader source file description.
 */
struct ShaderFile
{
    ShaderStage stage; ///< Shader stage (vertex, fragment, etc.).
    FilePath    path;  ///< Path to the shader file on disk.
};

/**
 * @brief Job for asynchronously loading a shader program from disk.
 */
struct ShaderJob
{
    std::string tag;               ///< Identifier tag for the shader.
    std::vector<ShaderFile> sources; ///< List of shader source files.
};

/**
 * @brief Job for asynchronously loading a font.
 */
struct FontJob
{
    std::string tag;        ///< Identifier tag for the font.
    std::string ttfPath;    ///< Path to TTF file.
    uint32_t    pixelSize = 32; ///< Font pixel size.
};

/**
 * @brief Job for asynchronously loading a sound file.
 */
struct SoundJob
{
    std::string tag;       ///< Identifier tag for the sound.
    std::string filePath;  ///< Path to the sound file.
    bool        loop = false; ///< Whether the sound should loop by default.
};

/**
 * @brief Job for asynchronously creating a sprite sheet from a texture.
 */
struct SpriteSheetJob
{
    std::string tag;       ///< Identifier tag for the sprite sheet.
    std::string textureTag; ///< Existing texture tag to base the sheet on.
    int frameW = 1;        ///< Frame width in pixels.
    int frameH = 1;        ///< Frame height in pixels.
};

/** @brief Union of all supported asynchronous load job requests. */
using LoadRequest = std::variant<TextureJob, ShaderJob, FontJob, SoundJob, SpriteSheetJob>;

/**
 * @brief Result of an asynchronous texture load.
 */
struct TextureResult
{
    std::string tag;       ///< Identifier tag.
    TextureSettings settings{}; ///< Original texture settings.
    int width = 0;         ///< Width in pixels.
    int height = 0;        ///< Height in pixels.
    int channels = 0;      ///< Number of color channels.
    std::vector<unsigned char> pixels; ///< Raw pixel data.
    bool ok = false;       ///< Whether loading succeeded.
};

/**
 * @brief In-memory shader source after load.
 */
struct ShaderSource
{
    ShaderStage stage;     ///< Shader stage type.
    std::string source;    ///< Loaded source code.
};

/**
 * @brief Result of an asynchronous shader load.
 */
struct ShaderResult
{
    std::string tag;              ///< Identifier tag.
    std::vector<ShaderSource> sources; ///< Loaded sources.
    bool ok = false;              ///< True if all files loaded successfully.
};

/**
 * @brief Result of a font load request.
 */
struct FontResult
{
    std::string tag;       ///< Identifier tag.
    std::string ttfPath;   ///< TTF path.
    uint32_t    pixelSize = 32; ///< Font size in pixels.
    bool ok = true;        ///< Always true (validation deferred to renderer).
};

/**
 * @brief Result of a sound load request.
 */
struct SoundResult
{
    std::string tag;       ///< Identifier tag.
    std::string filePath;  ///< Path to sound file.
    bool loop = false;     ///< Whether looping was requested.
    bool ok = true;        ///< Always true (validation deferred to audio system).
};

/** @brief Union of all possible completed load results. */
using LoadResult = std::variant<TextureResult, ShaderResult, FontResult, SoundResult>;

/**
 * @brief Background worker that asynchronously loads engine resources (textures, shaders, fonts, sounds).
 *
 * @details
 * AsyncResourceLoader runs a background thread that processes queued @ref LoadRequest jobs.
 * Each completed load produces a @ref LoadResult which can be fetched and uploaded to GPU or audio engine.
 *
 * Typical flow:
 * - Call @ref Start to launch the worker thread.
 * - Enqueue resource jobs with @ref Enqueue.
 * - Periodically call @ref FetchCompleted to retrieve finished results and register them with engine managers.
 * - Call @ref Stop when shutting down or leaving the loading state.
 *
 * Thread safety:
 * - Public API is thread-safe for enqueueing jobs and fetching results.
 * - Internal synchronization uses a mutex + condition variable to wake the worker thread.
 */
class AsyncResourceLoader
{
public:
    AsyncResourceLoader() = default;
    ~AsyncResourceLoader();

    /**
     * @brief Start the worker thread.
     *
     * @note Safe to call once before enqueueing jobs.
     */
    void Start();

    /**
     * @brief Stop the worker thread and wait for it to finish.
     *
     * @note Should be called before engine shutdown.
     */
    void Stop();

    /**
     * @brief Enqueue a new asynchronous load request.
     *
     * @param req The load request to add.
     *
     * @details
     * Increments @ref totalCount and wakes the worker thread to process jobs.
     */
    void Enqueue(const LoadRequest& req);

    /**
     * @brief Get overall progress ratio.
     *
     * @return Float in [0,1] representing fraction of jobs completed.
     */
    [[nodiscard]] float GetProgress() const;

    /**
     * @brief Check if all jobs have finished.
     *
     * @return True if loadedCount == totalCount.
     */
    [[nodiscard]] bool HasFinished() const;

    /**
     * @brief Retrieve all completed load results since last call.
     *
     * @return Vector of LoadResult values.
     *
     * @note Clears internal completed list.
     */
    [[nodiscard]] std::vector<LoadResult> FetchCompleted();

    /**
     * @brief Total number of requests ever enqueued.
     *
     * @return Count of enqueued jobs.
     */
    [[nodiscard]] int GetTotalCount() const { return totalCount.load(); }

    /**
     * @brief Number of jobs successfully processed so far.
     *
     * @return Count of completed jobs.
     */
    [[nodiscard]] int GetLoadedCount() const { return loadedCount.load(); }

private:
    /**
     * @brief Worker thread entry point.
     *
     * @details
     * Continuously waits on @ref cv until requests are available or stop is requested.
     * Processes jobs synchronously (e.g., file IO, image decoding).
     */
    void Worker();

private:
    std::thread workerThread;             ///< Dedicated background thread.
    std::atomic<bool> running{ false };   ///< Worker running flag.

    std::queue<LoadRequest> requestQueue; ///< Pending job queue.
    mutable std::mutex queueMutex;        ///< Protects @ref requestQueue.
    std::condition_variable cv;           ///< Signals when new jobs arrive.

    std::vector<LoadResult> completed;    ///< Completed job results.
    mutable std::mutex completedMutex;    ///< Protects @ref completed.

    std::atomic<int> totalCount{ 0 };     ///< Total jobs submitted.
    std::atomic<int> loadedCount{ 0 };    ///< Jobs completed so far.
};
