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
 * @brief Load request describing a texture to be loaded asynchronously.
 *
 * @details
 * TextureJob contains all parameters needed by the worker thread to load
 * raw pixel data from disk and prepare a TextureResult.
 *
 * The tag is used by the main thread to register the loaded texture
 * into the engine's resource registry.
 */
struct TextureJob
{
    std::string tag;
    std::string filePath;
    TextureSettings settings{};
};

/**
 * @brief Describes a shader stage source file for a shader job.
 *
 * @details
 * Each ShaderFile identifies which stage the file belongs to (vertex/fragment/etc.)
 * and the file path to load from.
 */
struct ShaderFile
{
    ShaderStage stage;
    FilePath    path;
};

/**
 * @brief Load request describing a shader program to be loaded asynchronously.
 *
 * @details
 * ShaderJob groups multiple stage files under a single tag.
 * The worker thread reads each file into memory and produces ShaderResult
 * containing source strings per stage.
 */
struct ShaderJob
{
    std::string tag;
    std::vector<ShaderFile> sources;
};

/**
 * @brief Load request describing a font resource to be registered.
 *
 * @details
 * In this loader design, the worker thread may validate the font path
 * or prepare metadata, while the actual GPU-facing atlas/material setup
 * typically occurs on the main thread.
 */
struct FontJob
{
    std::string tag;
    std::string ttfPath;
    uint32_t    pixelSize = 32;
};

/**
 * @brief Load request describing a sound asset to be loaded asynchronously.
 *
 * @details
 * The worker thread can load/validate audio file data or metadata.
 * The loop flag is carried through to the final SoundResult.
 */
struct SoundJob
{
    std::string tag;
    std::string filePath;
    bool        loop = false;
};

/**
 * @brief Load request describing a sprite sheet resource to be created/registered.
 *
 * @details
 * SpriteSheetJob references an existing texture by tag and specifies
 * the sprite sheet grid division (frameW, frameH).
 *
 * Note: This job type exists in LoadRequest, but there is no corresponding
 * SpriteSheetResult in LoadResult in this header. It is expected that the
 * worker/main-thread integration handles sprite sheet registration separately.
 */
struct SpriteSheetJob
{
    std::string tag;
    std::string textureTag;
    int frameW = 1;
    int frameH = 1;
};

/**
 * @brief Variant type representing any supported asynchronous load request.
 *
 * @details
 * The worker thread consumes LoadRequest values from a queue and processes them
 * using std::visit.
 *
 * Important implementation notes:
 * - SpriteSheetJob exists as a request type, but the current Worker() implementation
 *   does not handle SpriteSheetJob. Enqueuing it will still increment loadedCount,
 *   but no LoadResult will be produced for it.
 */
using LoadRequest = std::variant<TextureJob, ShaderJob, FontJob, SoundJob, SpriteSheetJob>;

/**
 * @brief Result data produced by an asynchronous texture load.
 *
 * @details
 * Contains decoded pixel data along with dimensions and channel count.
 * The ok flag indicates whether loading/decoding succeeded.
 *
 * The main thread typically consumes this result to create a GPU texture.
 */
struct TextureResult
{
    std::string tag;
    TextureSettings settings{};
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> pixels;
    bool ok = false;
};

/**
 * @brief Shader source data for a single stage.
 */
struct ShaderSource
{
    ShaderStage stage;
    std::string source;
};

/**
 * @brief Result data produced by an asynchronous shader load.
 *
 * @details
 * Contains the shader source strings per stage.
 * The ok flag indicates whether all required source files were loaded successfully.
 *
 * The main thread typically consumes this result to compile/link a GPU shader program.
 */
struct ShaderResult
{
    std::string tag;
    std::vector<ShaderSource> sources;
    bool ok = false;
};

/**
 * @brief Result data for a font job.
 *
 * @details
 * This result primarily carries registration metadata (tag/path/pixelSize).
 * ok indicates whether the job is considered valid.
 */
struct FontResult
{
    std::string tag;
    std::string ttfPath;
    uint32_t    pixelSize = 32;
    bool ok = true;
};

/**
 * @brief Result data for a sound job.
 *
 * @details
 * This result carries the tag/path and loop flag into the main thread.
 * ok indicates whether the job is considered valid.
 */
struct SoundResult
{
    std::string tag;
    std::string filePath;
    bool loop = false;
    bool ok = true;
};

/**
 * @brief Variant type representing any supported completed load result.
 *
 * @details
 * FetchCompleted() transfers ownership of the internally stored completed results
 * to the caller using swap().
 *
 * Results are appended by the worker thread under completedMutex protection.
 */
using LoadResult = std::variant<TextureResult, ShaderResult, FontResult, SoundResult>;

/**
 * @brief Background worker that processes resource load requests asynchronously.
 *
 * @details
 * AsyncResourceLoader maintains:
 * - A worker thread that runs Worker()
 * - A thread-safe request queue guarded by queueMutex + condition_variable
 * - A completed result list guarded by completedMutex
 * - Atomic counters (totalCount / loadedCount) for progress reporting
 *
 * Typical usage pattern:
 * 1) Start()
 * 2) Enqueue(...) one or more LoadRequest
 * 3) Periodically poll GetProgress / HasFinished and FetchCompleted()
 * 4) Stop() (or rely on destructor to stop/join)
 *
 * FetchCompleted is expected to transfer completed results to the caller
 * (and clear the internal list), allowing the main thread to register/compile
 * GPU resources safely.
 */
class AsyncResourceLoader
{
public:
    /**
     * @brief Constructs an AsyncResourceLoader in a stopped state.
     */
    AsyncResourceLoader() = default;

    /**
     * @brief Destroys the loader and stops the worker thread if running.
     *
     * @details
     * The destructor is expected to ensure the worker thread is not left running.
     * A typical implementation calls Stop() and joins the thread.
     */
    ~AsyncResourceLoader();

    /**
     * @brief Starts the worker thread if it is not already running.
     *
     * @details
     * If running is already true, Start() returns immediately without creating a new thread.
     * When started, workerThread runs Worker() in a lambda.
     */
    void Start();

    /**
     * @brief Requests the worker thread to stop and waits for it to finish.
     *
     * @details
     * Stop() performs the following:
     * - If running is false, returns immediately.
     * - Under queueMutex, sets running to false.
     * - Notifies all waiters via cv.notify_all().
     * - Joins workerThread if joinable.
     *
     * Worker() exits when:
     * - running is false AND requestQueue is empty.
     * If running becomes false but there are still queued requests,
     * Worker() will continue processing until the queue is drained.
     */
    void Stop();

    /**
     * @brief Enqueues a load request and increases the total request count.
     *
     * @details
     * Pushes the request into requestQueue under queueMutex protection and increments totalCount.
     * Then notifies the worker thread via cv.notify_one().
     *
     * @param req Load request variant to enqueue.
     */
    void Enqueue(const LoadRequest& req);

    /**
     * @brief Returns progress as loadedCount / totalCount.
     *
     * @details
     * Implementation policy:
     * - If totalCount <= 0, this function returns 1.0f.
     * - Otherwise returns done / tot as float.
     *
     * @return Progress ratio. Returns 1.0f when no requests have been enqueued.
     */
    [[nodiscard]] float GetProgress() const;

    /**
     * @brief Returns whether the loader considers all work finished.
     *
     * @details
     * Current implementation returns true only when:
     * - loadedCount >= totalCount
     * - totalCount > 0
     *
     * @return true if finished according to counters; false otherwise.
     */
    [[nodiscard]] bool HasFinished() const;

    /**
     * @brief Fetches completed results produced by the worker and clears internal storage.
     *
     * @details
     * This function:
     * - Locks completedMutex
     * - Swaps internal completed vector with a local vector
     * - Returns the local vector
     *
     * Because swap() is used, this operation is efficient and leaves the internal
     * completed vector empty after the call.
     *
     * @return List of completed results since the last fetch.
     */
    [[nodiscard]] std::vector<LoadResult> FetchCompleted();

    /**
     * @brief Returns the total number of requests enqueued so far.
     */
    [[nodiscard]] int GetTotalCount() const { return totalCount.load(); }

    /**
     * @brief Returns the number of requests that have finished processing.
     */
    [[nodiscard]] int GetLoadedCount() const { return loadedCount.load(); }

private:
    /**
     * @brief Worker thread main loop that processes queued requests.
     *
     * @details
     * Worker() performs:
     * - stbi_set_flip_vertically_on_load(true) once at the start of the thread.
     * - Waits on cv until (running is false) OR (requestQueue is not empty).
     * - Exits only when running is false AND the queue is empty.
     * - Processes one request at a time by visiting the LoadRequest variant.
     *
     * For each processed request:
     * - A corresponding LoadResult is appended to completed (except SpriteSheetJob, which is not handled).
     * - loadedCount is incremented exactly once per request, regardless of success.
     *
     * Thread-safety:
     * - requestQueue is protected by queueMutex.
     * - completed is protected by completedMutex.
     */
    void Worker();

private:
    std::thread workerThread;
    std::atomic<bool> running{ false };

    std::queue<LoadRequest> requestQueue;
    mutable std::mutex queueMutex;
    std::condition_variable cv;

    std::vector<LoadResult> completed;
    mutable std::mutex completedMutex;

    std::atomic<int> totalCount{ 0 };
    std::atomic<int> loadedCount{ 0 };
};
