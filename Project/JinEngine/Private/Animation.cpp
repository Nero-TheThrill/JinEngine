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
    : sheet(std::move(sheet_)), frameTime(frameTime_), loop(loop_), isClipFinished(false) {
    if (frameTime == 0.f) frameTime = 0.001f;

}

void SpriteAnimator::PlayClip(int start, int end, bool loop_)
{
    playingClip = nullptr;
    this->startFrame = start;
    this->endFrame = end;
    this->loop = loop_;
    currentFrame = start;
    elapsed = 0.0f;
    isClipFinished = false;
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
        JIN_WRN("Can't play clip: There is no clip named \"" <<clipName<< "\".");
        return;
    }
    playingClip = clip;
    clipFrameIndex = 0;
    elapsed = 0.0f;
    currentFrame = clip->frameIndices[clipFrameIndex];
    isClipFinished = false;
}

void SpriteAnimator::Update(float dt)
{
    elapsed += dt;

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
                    break;
                }
            }
        }

        currentFrame = playingClip->frameIndices[clipFrameIndex];
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
