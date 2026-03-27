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
 * @brief Describes a texture loading request for the asynchronous resource loader.
 *
 * @details
 * TextureJob stores all information required for the worker thread to load an image
 * file from disk and prepare raw pixel data for later GPU upload.
 *
 * The asynchronous loader reads the file using stb_image on the worker thread,
 * fills a TextureResult with decoded pixel data, and pushes that result into the
 * completed result list for consumption on the main thread. :contentReference[oaicite:1]{index=1}
 *
 * @note
 * This job does not create a GPU texture object directly.
 * It only carries the information required to decode the source image.
 */
struct TextureJob
{
    std::string tag;
    std::string filePath;
    TextureSettings settings{};
};

/**
 * @brief Describes one shader source file used by a shader loading request.
 *
 * @details
 * ShaderFile pairs a shader stage with the file path containing the source code
 * for that stage. A ShaderJob may contain multiple ShaderFile entries so that
 * a complete shader program can be assembled from multiple source files.
 */
struct ShaderFile
{
    ShaderStage stage;
    FilePath    path;
};

/**
 * @brief Describes a shader loading request for the asynchronous resource loader.
 *
 * @details
 * ShaderJob stores the resource tag and a list of shader source files to read.
 * The worker thread reads each file into memory and converts it into a ShaderResult
 * containing stage/source string pairs.
 *
 * Actual shader compilation and program linking are not performed on the worker
 * thread in the current design. Instead, the loaded source strings are passed back
 * to the main thread for final resource creation. :contentReference[oaicite:2]{index=2}
 */
struct ShaderJob
{
    std::string tag;
    std::vector<ShaderFile> sources;
};

/**
 * @brief Describes a font loading request.
 *
 * @details
 * FontJob stores the data needed to register a font later, including the font tag,
 * the TTF file path, and the target pixel size.
 *
 * In the current implementation, the worker thread does not parse or rasterize the
 * font file. It only forwards the metadata into a FontResult so the actual font
 * creation can occur in a later stage on the main thread. :contentReference[oaicite:3]{index=3}
 */
struct FontJob
{
    std::string tag;
    std::string ttfPath;
    uint32_t    pixelSize = 32;
};

/**
 * @brief Describes a sound loading request.
 *
 * @details
 * SoundJob stores the information required to register a sound resource later,
 * including its tag, source file path, and looping default.
 *
 * In the current implementation, the worker thread does not decode or initialize
 * the sound resource itself. It simply packages this information into a SoundResult
 * for later processing on the main thread. :contentReference[oaicite:4]{index=4}
 */
struct SoundJob
{
    std::string tag;
    std::string filePath;
    bool        loop = false;
};

/**
 * @brief Describes a sprite sheet creation request.
 *
 * @details
 * SpriteSheetJob stores the tag of the sprite sheet to create, the texture tag to
 * reference, and the frame dimensions used to interpret the texture as a grid.
 *
 * This type is already included in LoadRequest so the loader interface can be extended
 * to support asynchronous sprite sheet preparation. However, in the current worker
 * implementation, SpriteSheetJob is not processed yet. :contentReference[oaicite:5]{index=5}
 *
 * @note
 * Because the current Worker() implementation does not handle this request type,
 * enqueuing this job without adding handling logic would leave it unsupported.
 */
struct SpriteSheetJob
{
    std::string tag;
    std::string textureTag;
    int frameW = 1;
    int frameH = 1;
};

/**
 * @brief Represents a single queued asynchronous load request.
 *
 * @details
 * LoadRequest is a variant that allows the loader queue to store different resource
 * job types in a single container. The worker thread resolves the actual job type
 * using std::visit and processes it accordingly. :contentReference[oaicite:6]{index=6}
 */
using LoadRequest = std::variant<TextureJob, ShaderJob, FontJob, SoundJob, SpriteSheetJob>;

/**
 * @brief Stores the completed result of an asynchronous texture load.
 *
 * @details
 * TextureResult contains decoded image information produced by the worker thread,
 * including dimensions, channel count, pixel bytes, and the original texture settings.
 *
 * If loading succeeds, ok is set to true and pixels contains the raw image data.
 * If loading fails, ok is false and the pixel buffer may remain empty. :contentReference[oaicite:7]{index=7}
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
 * @brief Stores one in-memory shader source produced by asynchronous file loading.
 *
 * @details
 * ShaderSource pairs a shader stage with the full source text read from disk.
 * A ShaderResult may contain multiple ShaderSource entries that are later used
 * to create and link a shader program on the main thread.
 */
struct ShaderSource
{
    ShaderStage stage;
    std::string source;
};

/**
 * @brief Stores the completed result of an asynchronous shader source load.
 *
 * @details
 * ShaderResult contains the shader tag, the in-memory source code for each shader
 * stage, and a success flag indicating whether all requested files were read
 * successfully.
 *
 * The worker thread marks ok as false if any shader source file could not be read. :contentReference[oaicite:8]{index=8}
 */
struct ShaderResult
{
    std::string tag;
    std::vector<ShaderSource> sources;
    bool ok = false;
};

/**
 * @brief Stores the completed result of a font loading request.
 *
 * @details
 * FontResult currently acts as a metadata pass-through object.
 * It contains the font tag, TTF path, and target pixel size so that the main thread
 * can later register or construct the actual font resource.
 *
 * In the current implementation, this result is produced without parsing the font file,
 * so ok defaults to true unless future logic changes that behavior. :contentReference[oaicite:9]{index=9}
 */
struct FontResult
{
    std::string tag;
    std::string ttfPath;
    uint32_t    pixelSize = 32;
    bool ok = true;
};

/**
 * @brief Stores the completed result of a sound loading request.
 *
 * @details
 * SoundResult currently carries sound metadata forward to the main thread,
 * including the resource tag, source file path, and looping setting.
 *
 * The current worker implementation does not decode the sound data itself,
 * so ok defaults to true unless the logic is extended in the future. :contentReference[oaicite:10]{index=10}
 */
struct SoundResult
{
    std::string tag;
    std::string filePath;
    bool loop = false;
    bool ok = true;
};

/**
 * @brief Represents one completed asynchronous load result.
 *
 * @details
 * LoadResult is a variant that stores one of the supported result types returned by
 * the worker thread. The main thread can inspect the concrete type with std::visit
 * and finalize resource creation appropriately.
 *
 * @note
 * The current result variant does not yet include a SpriteSheet result type,
 * matching the current Worker() implementation. :contentReference[oaicite:11]{index=11}
 */
using LoadResult = std::variant<TextureResult, ShaderResult, FontResult, SoundResult>;

/**
 * @brief Loads resource data asynchronously on a background worker thread.
 *
 * @details
 * AsyncResourceLoader provides a queue-based asynchronous loading pipeline for resource
 * requests such as textures, shaders, fonts, and sounds.
 *
 * The general workflow is:
 * 1. Start() launches the worker thread.
 * 2. Enqueue() pushes one or more LoadRequest entries into the request queue.
 * 3. Worker() waits for queued requests, processes them, and pushes completed results
 *    into an internal completed list.
 * 4. FetchCompleted() is called from the main thread to retrieve finished results and
 *    consume them safely.
 *
 * The class tracks overall loading state using:
 * - totalCount: total number of queued jobs,
 * - loadedCount: number of jobs processed by the worker.
 *
 * Progress is reported as loadedCount / totalCount, and HasFinished() simply checks
 * whether the processed job count has reached the total queued job count. :contentReference[oaicite:12]{index=12}
 *
 * Thread synchronization is handled using:
 * - queueMutex for request queue access,
 * - completedMutex for completed result access,
 * - condition_variable to wake the worker when new jobs arrive or shutdown begins.
 *
 * @note
 * This loader is responsible for background file reading and data preparation.
 * Final GPU upload or engine-side registration is expected to happen elsewhere.
 */
class AsyncResourceLoader
{
public:
    /**
     * @brief Creates an asynchronous resource loader.
     *
     * @details
     * The default constructor leaves the loader idle.
     * No worker thread is started until Start() is called.
     */
    AsyncResourceLoader() = default;

    /**
     * @brief Stops the loader if necessary and releases worker thread resources.
     *
     * @details
     * The destructor calls Stop() so that an active worker thread is shut down cleanly
     * before the loader object is destroyed. :contentReference[oaicite:13]{index=13}
     */
    ~AsyncResourceLoader();

    /**
     * @brief Starts the worker thread.
     *
     * @details
     * If the loader is already running, this function returns immediately.
     * Otherwise, it marks the loader as running and spawns the background thread
     * that executes Worker(). :contentReference[oaicite:14]{index=14}
     *
     * @note
     * Calling Start() multiple times while already running has no effect.
     */
    void Start();

    /**
     * @brief Stops the worker thread and waits for it to finish.
     *
     * @details
     * If the loader is not running, this function returns immediately.
     * Otherwise, it sets running to false under the queue lock, wakes the worker
     * through the condition variable, and joins the worker thread if joinable.
     *
     * The worker exits only after it observes that loading has been stopped and
     * the request queue has been drained. :contentReference[oaicite:15]{index=15}
     *
     * @note
     * This function is synchronous and blocks until the worker thread exits.
     */
    void Stop();

    /**
     * @brief Adds a new load request to the queue.
     *
     * @details
     * Pushes the given request into the internal request queue under a mutex,
     * increments totalCount, and wakes the worker thread so processing can begin.
     *
     * @param req Load request to enqueue.
     */
    void Enqueue(const LoadRequest& req);

    /**
     * @brief Returns overall load progress as a normalized ratio.
     *
     * @details
     * Progress is computed as loadedCount / totalCount.
     * If no jobs have been queued yet, the function returns 1.0f.
     *
     * @return A value in the range [0, 1] when jobs exist, or 1.0f when totalCount is zero.
     */
    [[nodiscard]] float GetProgress() const;

    /**
     * @brief Returns whether all queued jobs have been processed.
     *
     * @details
     * This function compares loadedCount against totalCount and returns true when
     * the processed count has reached or exceeded the queued job count. :contentReference[oaicite:16]{index=16}
     *
     * @return True if all queued jobs have finished processing, otherwise false.
     */
    [[nodiscard]] bool HasFinished() const;

    /**
     * @brief Retrieves and clears all completed results accumulated so far.
     *
     * @details
     * Acquires the completed result mutex, swaps the internal completed vector into
     * a local output vector, and returns that output to the caller.
     *
     * This design allows the main thread to efficiently consume finished results
     * without holding the lock longer than necessary. :contentReference[oaicite:17]{index=17}
     *
     * @return Vector containing all currently completed results.
     */
    [[nodiscard]] std::vector<LoadResult> FetchCompleted();

    /**
     * @brief Returns the total number of queued jobs.
     *
     * @return Total queued job count.
     */
    [[nodiscard]] int GetTotalCount() const { return totalCount.load(); }

    /**
     * @brief Returns the number of jobs already processed by the worker.
     *
     * @return Processed job count.
     */
    [[nodiscard]] int GetLoadedCount() const { return loadedCount.load(); }

private:
    /**
     * @brief Worker thread entry point that processes queued requests.
     *
     * @details
     * Worker() runs in a loop while the loader is active.
     * It waits on the condition variable until either:
     * - loading is stopped, or
     * - at least one request exists in the queue.
     *
     * For each dequeued request, the worker dispatches behavior based on the concrete
     * request type using std::visit:
     *
     * - TextureJob:
     *   Loads image pixels using stb_image, fills TextureResult, and stores the result.
     *
     * - ShaderJob:
     *   Reads shader source files as text, fills ShaderResult, and stores the result.
     *
     * - FontJob:
     *   Copies metadata into FontResult and stores the result.
     *
     * - SoundJob:
     *   Copies metadata into SoundResult and stores the result.
     *
     * After finishing each job, loadedCount is incremented. :contentReference[oaicite:18]{index=18}
     *
     * @note
     * The current implementation does not yet handle SpriteSheetJob explicitly.
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