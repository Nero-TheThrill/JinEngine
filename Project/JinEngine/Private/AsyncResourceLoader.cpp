#include "AsyncResourceLoader.h"
#include <fstream>
#include <sstream>

#include "stb_image.h"
#include "Engine.h"

namespace
{
    std::string ReadTextFile(const FilePath& path)
    {
        std::ifstream ifs(path);
        if (!ifs.is_open())
            return {};
        std::stringstream ss;
        ss << ifs.rdbuf();
        return ss.str();
    }
}

AsyncResourceLoader::~AsyncResourceLoader()
{
    Stop();
}

void AsyncResourceLoader::Start()
{
    if (running.load())
        return;
    running = true;
    workerThread = std::thread([this]()
        {
            Worker();
        });
}

void AsyncResourceLoader::Stop()
{
    if (!running.load())
        return;

    {
        std::lock_guard<std::mutex> lk(queueMutex);
        running = false;
    }
    cv.notify_all();

    if (workerThread.joinable())
        workerThread.join();
}

void AsyncResourceLoader::Enqueue(const LoadRequest& req)
{
    {
        std::lock_guard<std::mutex> lk(queueMutex);
        requestQueue.push(req);
        totalCount.fetch_add(1);
    }
    cv.notify_one();
}

float AsyncResourceLoader::GetProgress() const
{
    int tot = totalCount.load();
    int done = loadedCount.load();
    if (tot <= 0) return 1.0f;
    return static_cast<float>(done) / static_cast<float>(tot);
}

bool AsyncResourceLoader::HasFinished() const
{
    return loadedCount.load() >= totalCount.load();
}

std::vector<LoadResult> AsyncResourceLoader::FetchCompleted()
{
    std::vector<LoadResult> out;
    std::lock_guard<std::mutex> lk(completedMutex);
    out.swap(completed);
    return out;
}

void AsyncResourceLoader::Worker()
{
    stbi_set_flip_vertically_on_load(true);

    while (true)
    {
        LoadRequest req;

        {
            std::unique_lock<std::mutex> lk(queueMutex);
            cv.wait(lk, [this]()
                {
                    return !running.load() || !requestQueue.empty();
                });

            if (!running.load() && requestQueue.empty())
                break;

            req = requestQueue.front();
            requestQueue.pop();
        }

        std::visit([this](auto&& job)
            {
                using T = std::decay_t<decltype(job)>;

                if constexpr (std::is_same_v<T, TextureJob>)
                {
                    TextureResult r;
                    r.tag = job.tag;
                    r.settings = job.settings;

                    int w = 0, h = 0, ch = 0;
                    unsigned char* data = stbi_load(job.filePath.c_str(), &w, &h, &ch, 0);
                    if (data)
                    {
                        r.width = w;
                        r.height = h;
                        r.channels = ch;
                        r.pixels.assign(data, data + (w * h * ch));
                        stbi_image_free(data);
                        r.ok = true;
                    }
                    else
                    {
                        JIN_ERR("Async texture load failed: " << job.filePath);
                        r.ok = false;
                    }

                    std::lock_guard<std::mutex> lk(completedMutex);
                    completed.emplace_back(std::move(r));
                }
                else if constexpr (std::is_same_v<T, ShaderJob>)
                {
                    ShaderResult r;
                    r.tag = job.tag;
                    r.ok = true;

                    for (const auto& f : job.sources)
                    {
                        ShaderSource src;
                        src.stage = f.stage;
                        src.source = ReadTextFile(f.path);
                        if (src.source.empty())
                        {
                            JIN_ERR("Async shader file read failed: " << f.path);
                            r.ok = false;
                        }
                        r.sources.emplace_back(std::move(src));
                    }

                    std::lock_guard<std::mutex> lk(completedMutex);
                    completed.emplace_back(std::move(r));
                }
                else if constexpr (std::is_same_v<T, FontJob>)
                {
                    FontResult r;
                    r.tag = job.tag;
                    r.ttfPath = job.ttfPath;
                    r.pixelSize = job.pixelSize;
                    r.ok = true;

                    std::lock_guard<std::mutex> lk(completedMutex);
                    completed.emplace_back(std::move(r));
                }
                else if constexpr (std::is_same_v<T, SoundJob>)
                {
                    SoundResult r;
                    r.tag = job.tag;
                    r.filePath = job.filePath;
                    r.loop = job.loop;
                    r.ok = true;

                    std::lock_guard<std::mutex> lk(completedMutex);
                    completed.emplace_back(std::move(r));
                }
               
                loadedCount.fetch_add(1);

            }, req);
    }
}
