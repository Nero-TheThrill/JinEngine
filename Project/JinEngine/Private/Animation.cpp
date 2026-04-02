#include "Engine.h"

SpriteSheet::SpriteSheet(std::shared_ptr<Texture> texture_, int frameW, int frameH)
    : texture(std::move(texture_)), frameWidth(frameW), frameHeight(frameH)
{
    texWidth = texture->GetWidth();
    texHeight = texture->GetHeight();
    columns = texWidth / frameWidth;
    rows = texHeight / frameHeight;
    if (columns == 0) columns = 1;
    if (rows == 0) rows = 1;
}

glm::vec2 SpriteSheet::GetUVOffset(int frameIndex) const
{
    int col = frameIndex % columns;
    int row = frameIndex / columns;
    int flippedRow = (rows - 1) - row;
    float u = static_cast<float>(col * frameWidth) / texWidth;
    float v = static_cast<float>(flippedRow * frameHeight) / texHeight;
    return { u, v };
}

glm::vec2 SpriteSheet::GetUVScale() const
{
    return {
        static_cast<float>(frameWidth) / texWidth,
        static_cast<float>(frameHeight) / texHeight
    };
}

int SpriteSheet::GetFrameCount() const
{
    return columns * rows;
}

void SpriteSheet::AddClip(const std::string& name, const std::vector<int>& frames, float frameDuration, bool looping)
{
    SpriteClip clip;
    clip.frameIndices = frames;
    clip.frameDuration = frameDuration;
    clip.looping = looping;

    animationClips[name] = clip;
}

const SpriteClip* SpriteSheet::GetClip(const std::string& name) const
{
    auto it = animationClips.find(name);
    if (it != animationClips.end())
        return &it->second;
    return nullptr;
}

SpriteAnimator::SpriteAnimator(std::shared_ptr<SpriteSheet> sheet_, float frameTime_, bool loop_)
    : sheet(std::move(sheet_)), frameTime(frameTime_), loop(loop_), isClipFinished(false)
{
    if (frameTime == 0.f) frameTime = 0.001f;
}

void SpriteAnimator::PlayClip(int start, int end, bool loop_)
{
    playingClip = nullptr;
    currentClipName.clear();

    this->startFrame = start;
    this->endFrame = end;
    this->loop = loop_;
    currentFrame = start;
    clipFrameIndex = 0;
    elapsed = 0.0f;
    isClipFinished = false;

    TriggerAllCallbacksForCurrentFrame();
}

void SpriteAnimator::PlayClip(const std::string& clipName)
{
    if (!sheet)
    {
        JIN_ERR("Can't play clip: Sprite sheet is nullptr");
        return;
    }

    const auto* clip = sheet->GetClip(clipName);
    if (!clip || clip->frameIndices.empty())
    {
        JIN_WRN("Can't play clip: There is no clip named \"" << clipName << "\".");
        return;
    }

    currentClipName = clipName;
    playingClip = clip;
    clipFrameIndex = 0;
    elapsed = 0.0f;
    currentFrame = clip->frameIndices[clipFrameIndex];
    isClipFinished = false;

    TriggerAllCallbacksForCurrentFrame();
}

void SpriteAnimator::Update(float dt)
{
    float speedMultiplier = 1.0f;

    if (playingClip && !currentClipName.empty())
    {
        speedMultiplier = GetClipPlaybackSpeed(currentClipName);
    }

    elapsed += dt * speedMultiplier * playbackSpeed;

    if (playingClip)
    {
        const float dur = playingClip->frameDuration > 0.f ? playingClip->frameDuration : 0.0001f;

        while (elapsed >= dur)
        {
            elapsed -= dur;
            ++clipFrameIndex;

            if (clipFrameIndex >= static_cast<int>(playingClip->frameIndices.size()))
            {
                if (playingClip->looping)
                {
                    clipFrameIndex = 0;
                }
                else
                {
                    isClipFinished = true;
                    clipFrameIndex = static_cast<int>(playingClip->frameIndices.size()) - 1;
                    currentFrame = playingClip->frameIndices[clipFrameIndex];
                    break;
                }
            }

            currentFrame = playingClip->frameIndices[clipFrameIndex];
            TriggerAllCallbacksForCurrentFrame();
        }
    }
    else
    {
        const float dur = frameTime > 0.f ? frameTime : 0.0001f;

        while (elapsed >= dur)
        {
            elapsed -= dur;
            ++currentFrame;

            if (currentFrame > endFrame)
            {
                if (loop)
                {
                    currentFrame = startFrame;
                }
                else
                {
                    isClipFinished = true;
                    currentFrame = endFrame;
                    break;
                }
            }

            TriggerAllCallbacksForCurrentFrame();
        }
    }
}

glm::vec2 SpriteAnimator::GetUVOffset() const
{
    return sheet ? sheet->GetUVOffset(currentFrame) : glm::vec2(0.0f);
}

glm::vec2 SpriteAnimator::GetUVScale() const
{
    return sheet ? sheet->GetUVScale() : glm::vec2(1.0f);
}

void SpriteAnimator::SetSpriteSheet(std::shared_ptr<SpriteSheet> newSheet)
{
    if (!newSheet)
    {
        JIN_ERR("SetSpriteSheet failed: nullptr");
        return;
    }

    sheet = std::move(newSheet);

    playingClip = nullptr;
    currentClipName.clear();

    startFrame = 0;
    endFrame = sheet->GetFrameCount() - 1;
    currentFrame = 0;

    clipFrameIndex = 0;
    elapsed = 0.0f;
    isClipFinished = false;
}

void SpriteAnimator::SetPlaybackSpeed(float speed)
{
    if (speed <= 0.0f)
    {
        playbackSpeed = 0.001f;
        return;
    }

    playbackSpeed = speed;
}

void SpriteAnimator::SetClipPlaybackSpeed(const std::string& clipName, float speed)
{
    if (speed <= 0.001f)
    {
        clipPlaybackSpeeds[clipName] = 0.001f;
        return;
    }

    clipPlaybackSpeeds[clipName] = speed;
}

float SpriteAnimator::GetClipPlaybackSpeed(const std::string& clipName) const
{
    auto it = clipPlaybackSpeeds.find(clipName);
    if (it != clipPlaybackSpeeds.end())
    {
        return it->second;
    }

    return 1.0f;
}

void SpriteAnimator::ClearClipPlaybackSpeed(const std::string& clipName)
{
    clipPlaybackSpeeds.erase(clipName);
}

void SpriteAnimator::AddFrameCallback(int frame, std::function<void()> callback)
{
    frameCallbacks[frame].push_back({ std::move(callback), false });
}

void SpriteAnimator::AddFrameCallbackOnce(int frame, std::function<void()> callback)
{
    frameCallbacks[frame].push_back({ std::move(callback), true });
}

void SpriteAnimator::AddClipFrameCallback(const std::string& clipName, int localFrameIndex, std::function<void()> callback)
{
    clipFrameCallbacks[clipName][localFrameIndex].push_back({ std::move(callback), false });
}

void SpriteAnimator::AddClipFrameCallbackOnce(const std::string& clipName, int localFrameIndex, std::function<void()> callback)
{
    clipFrameCallbacks[clipName][localFrameIndex].push_back({ std::move(callback), true });
}

void SpriteAnimator::ClearFrameCallbacks(int frame)
{
    frameCallbacks.erase(frame);
}

void SpriteAnimator::ClearClipFrameCallbacks(const std::string& clipName, int localFrameIndex)
{
    auto clipIt = clipFrameCallbacks.find(clipName);
    if (clipIt == clipFrameCallbacks.end())
    {
        return;
    }

    clipIt->second.erase(localFrameIndex);

    if (clipIt->second.empty())
    {
        clipFrameCallbacks.erase(clipIt);
    }
}

void SpriteAnimator::ClearAllCallbacks()
{
    frameCallbacks.clear();
    clipFrameCallbacks.clear();
}

void SpriteAnimator::TriggerFrameCallbacks(int frame)
{
    auto it = frameCallbacks.find(frame);
    if (it == frameCallbacks.end())
    {
        return;
    }

    auto& callbacks = it->second;

    for (auto iter = callbacks.begin(); iter != callbacks.end(); )
    {
        if (iter->callback)
        {
            iter->callback();
        }

        if (iter->once)
        {
            iter = callbacks.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    if (callbacks.empty())
    {
        frameCallbacks.erase(it);
    }
}

void SpriteAnimator::TriggerClipFrameCallbacks()
{
    if (!playingClip || currentClipName.empty())
    {
        return;
    }

    auto clipIt = clipFrameCallbacks.find(currentClipName);
    if (clipIt == clipFrameCallbacks.end())
    {
        return;
    }

    auto localIt = clipIt->second.find(clipFrameIndex);
    if (localIt == clipIt->second.end())
    {
        return;
    }

    auto& callbacks = localIt->second;

    for (auto iter = callbacks.begin(); iter != callbacks.end(); )
    {
        if (iter->callback)
        {
            iter->callback();
        }

        if (iter->once)
        {
            iter = callbacks.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    if (callbacks.empty())
    {
        clipIt->second.erase(localIt);
    }

    if (clipIt->second.empty())
    {
        clipFrameCallbacks.erase(clipIt);
    }
}

void SpriteAnimator::TriggerAllCallbacksForCurrentFrame()
{
    TriggerFrameCallbacks(currentFrame);
    TriggerClipFrameCallbacks();
}