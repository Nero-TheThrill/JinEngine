#include "Engine.h"

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>
#include <queue>
#include <string>
#include <functional>

#define MA_NO_WASAPI
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"


struct SoundManager::SoundInstanceMA
{
    ma_sound sound{};
    std::string tag;
    bool looping = false;

    bool paused = false;
    ma_uint64 pausedCursor = 0;

    std::function<void(SoundInstanceID, const std::string&)> onFinished;
};

SoundManager::SoundManager()
    : engine(nullptr)
    , nextInstanceID(1)
{
}

SoundManager::~SoundManager()
{
    Free();
}

bool SoundManager::Init()
{
    if (engine == nullptr)
    {
        engine = new ma_engine();
    }

    ma_result res = ma_engine_init(nullptr, engine);
    if (res != MA_SUCCESS)
    {
        JIN_ERR("miniaudio engine init failed: " << (int)res);
        delete engine;
        engine = nullptr;
        return false;
    }
    return true;
}

void SoundManager::Free()
{
    for (auto& [id, instPtr] : instanceMap)
    {
        if (instPtr != nullptr)
        {
            ma_sound_uninit(&instPtr->sound);
            delete instPtr;
        }
    }
    instanceMap.clear();

    for (auto& [tag, vec] : activeChannels)
    {
        vec.clear();
    }
    activeChannels.clear();

    sounds.clear();

    while (!reusableIDs.empty())
    {
        reusableIDs.pop();
    }
    nextInstanceID = 1;

    if (engine != nullptr)
    {
        ma_engine_uninit(engine);
        delete engine;
        engine = nullptr;
    }
}

void SoundManager::LoadSound(const std::string& tag, const std::string& filepath, bool loop)
{
    sounds[tag] = SoundInfo{ filepath, loop };
}

SoundInstanceID SoundManager::GenerateID()
{
    if (!reusableIDs.empty())
    {
        SoundInstanceID id = reusableIDs.front();
        reusableIDs.pop();
        return id;
    }
    return nextInstanceID++;
}

[[nodiscard]] static bool MA_GetSoundSampleRate(ma_sound* snd, ma_uint32& outSR)
{
    ma_format fmt{};
    ma_uint32 ch{};
    ma_uint32 sr{};
    if (ma_sound_get_data_format(snd, &fmt, &ch, &sr, nullptr, 0) == MA_SUCCESS && sr > 0)
    {
        outSR = sr;
        return true;
    }
    return false;
}

SoundInstanceID SoundManager::Play(const std::string& tag, float volume, float startTimeSec)
{
    return Play(tag, volume, startTimeSec, nullptr);
}

SoundInstanceID SoundManager::Play(const std::string& tag,
    float volume,
    float startTimeSec,
    std::function<void(SoundInstanceID, const std::string&)> onFinished)
{
    if (engine == nullptr)
        return 0;

    auto it = sounds.find(tag);
    if (it == sounds.end())
        return 0;

    const std::string& path = it->second.filepath;
    const bool looping = it->second.loop;

    SoundInstanceMA* inst = new SoundInstanceMA();
    inst->tag = tag;
    inst->looping = looping;
    inst->onFinished = std::move(onFinished);

    ma_result res = ma_sound_init_from_file(engine, path.c_str(), 0, nullptr, nullptr, &inst->sound);
    if (res != MA_SUCCESS)
    {
        JIN_ERR("Failed to load sound: " << path);
        delete inst;
        return 0;
    }

    ma_sound_set_volume(&inst->sound, volume);
    ma_sound_set_looping(&inst->sound, looping ? MA_TRUE : MA_FALSE);

    if (startTimeSec > 0.0f)
    {
        ma_uint32 sr = 0;
        if (!MA_GetSoundSampleRate(&inst->sound, sr))
        {
            sr = ma_engine_get_sample_rate(engine);
        }
        const ma_uint64 frame = (ma_uint64)(startTimeSec * (double)sr);
        ma_sound_seek_to_pcm_frame(&inst->sound, frame);
    }

    res = ma_sound_start(&inst->sound);
    if (res != MA_SUCCESS)
    {
        ma_sound_uninit(&inst->sound);
        delete inst;
        return 0;
    }

    SoundInstanceID id = GenerateID();
    instanceMap[id] = inst;
    activeChannels[tag].push_back(inst);
    return id;
}

void SoundManager::SetVolumeByID(SoundInstanceID id, float volume)
{
    auto it = instanceMap.find(id);
    if (it != instanceMap.end() && it->second != nullptr)
    {
        ma_sound_set_volume(&it->second->sound, volume);
    }
}

void SoundManager::SetVolumeByTag(const std::string& tag, float volume)
{
    auto it = activeChannels.find(tag);
    if (it == activeChannels.end())
        return;

    for (SoundInstanceMA* inst : it->second)
    {
        if (inst != nullptr)
        {
            ma_sound_set_volume(&inst->sound, volume);
        }
    }
}

void SoundManager::SetVolumeAll(float volume)
{
    for (const auto& [tag, _] : activeChannels)
    {
        SetVolumeByTag(tag, volume);
    }
}

void SoundManager::ControlByID(SoundControlType control, SoundInstanceID id)
{
    auto it = instanceMap.find(id);
    if (it == instanceMap.end() || it->second == nullptr)
        return;

    SoundInstanceMA* inst = it->second;

    switch (control)
    {
    case SoundControlType::Pause:
    {
        ma_uint64 cursor = 0;
        ma_sound_get_cursor_in_pcm_frames(&inst->sound, &cursor);
        inst->pausedCursor = cursor;
        ma_sound_stop(&inst->sound);
        inst->paused = true;
    }
    break;

    case SoundControlType::Resume:
    {
        if (inst->paused)
        {
            ma_sound_seek_to_pcm_frame(&inst->sound, inst->pausedCursor);
            ma_sound_start(&inst->sound);
            inst->paused = false;
        }
    }
    break;

    case SoundControlType::Stop:
    {
        ma_sound_stop(&inst->sound);
        ma_sound_uninit(&inst->sound);

        auto acIt = activeChannels.find(inst->tag);
        if (acIt != activeChannels.end())
        {
            auto& vec = acIt->second;
            vec.erase(std::remove(vec.begin(), vec.end(), inst), vec.end());
        }

        delete inst;
        instanceMap.erase(it);
        reusableIDs.push(id);
    }
    break;
    }
}

void SoundManager::ControlByTag(SoundControlType control, const std::string& tag)
{
    auto it = activeChannels.find(tag);
    if (it == activeChannels.end())
        return;

    auto& vec = it->second;

    if (control == SoundControlType::Pause)
    {
        for (auto* inst : vec)
        {
            if (!inst)
                continue;

            ma_uint64 cursor = 0;
            ma_sound_get_cursor_in_pcm_frames(&inst->sound, &cursor);
            inst->pausedCursor = cursor;
            ma_sound_stop(&inst->sound);
            inst->paused = true;
        }
        return;
    }

    if (control == SoundControlType::Resume)
    {
        for (auto* inst : vec)
        {
            if (!inst || !inst->paused)
                continue;

            ma_sound_seek_to_pcm_frame(&inst->sound, inst->pausedCursor);
            ma_sound_start(&inst->sound);
            inst->paused = false;
        }
        return;
    }

    if (control == SoundControlType::Stop)
    {
        std::vector<SoundInstanceID> toRecycle;
        toRecycle.reserve(instanceMap.size());

        for (SoundInstanceMA* inst : vec)
        {
            if (inst == nullptr)
                continue;

            ma_sound_stop(&inst->sound);
            ma_sound_uninit(&inst->sound);

            for (auto& [id, uptr] : instanceMap)
            {
                if (uptr == inst)
                {
                    toRecycle.push_back(id);
                    break;
                }
            }

            delete inst;
        }

        for (SoundInstanceID id : toRecycle)
        {
            instanceMap.erase(id);
            reusableIDs.push(id);
        }

        vec.clear();
    }
}

void SoundManager::ControlAll(SoundControlType control)
{
    std::vector<std::string> tags;
    tags.reserve(activeChannels.size());

    for (const auto& kv : activeChannels)
    {
        tags.push_back(kv.first);
    }

    for (const std::string& t : tags)
    {
        ControlByTag(control, t);
    }
}

void SoundManager::Update()
{
    Cleanup();
}

void SoundManager::Cleanup()
{
    std::vector<SoundInstanceID> finishedIDs;
    finishedIDs.reserve(instanceMap.size());

    std::vector<std::function<void()>> pendingCallbacks;
    pendingCallbacks.reserve(instanceMap.size());

    for (auto& [id, inst] : instanceMap)
    {
        if (inst == nullptr)
        {
            finishedIDs.push_back(id);
            continue;
        }

        const ma_bool32 playing = ma_sound_is_playing(&inst->sound);
        if (!playing)
        {
            if (inst->looping)
                continue;

            if (ma_sound_at_end(&inst->sound))
            {
                if (inst->onFinished)
                {
                    const std::string finishedTag = inst->tag;
                    pendingCallbacks.push_back(
                        [callback = inst->onFinished, id, finishedTag]()
                        {
                            callback(id, finishedTag);
                        });
                }

                ma_sound_uninit(&inst->sound);

                auto acIt = activeChannels.find(inst->tag);
                if (acIt != activeChannels.end())
                {
                    auto& vec = acIt->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), inst), vec.end());
                }

                delete inst;
                finishedIDs.push_back(id);
            }
        }
    }

    for (SoundInstanceID id : finishedIDs)
    {
        instanceMap.erase(id);
        reusableIDs.push(id);
    }

    for (auto& [tag, vec] : activeChannels)
    {
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [](SoundInstanceMA* inst)
                {
                    if (inst == nullptr)
                        return true;

                    if (!inst->looping && !ma_sound_is_playing(&inst->sound) && ma_sound_at_end(&inst->sound))
                        return true;

                    return false;
                }),
            vec.end());
    }

    for (auto& callback : pendingCallbacks)
    {
        if (callback)
        {
            callback();
        }
    }
}