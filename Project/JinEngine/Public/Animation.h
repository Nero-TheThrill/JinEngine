#pragma once
#include <unordered_map>

#include "vec2.hpp"
#include "Texture.h"
struct SpriteFrame
{
    glm::vec2 uvTopLeft;
    glm::vec2 uvBottomRight;
    glm::ivec2 pixelSize;
    glm::ivec2 offset;
};

struct SpriteClip
{
    std::vector<int> frameIndices;
    float frameDuration;
    bool looping;
};
class SpriteSheet
{

public:
    SpriteSheet(std::shared_ptr<Texture> texture_, int frameW, int frameH);

    [[nodiscard]] glm::vec2 GetUVOffset(int frameIndex) const;
    [[nodiscard]] glm::vec2 GetUVScale() const;

    [[nodiscard]] std::shared_ptr<Texture> GetTexture() const { return texture; }

    [[nodiscard]] int GetFrameCount() const;

    void AddClip(const std::string& name, const std::vector<int>& frames, float frameDuration, bool looping = true);
    [[nodiscard]] const SpriteClip* GetClip(const std::string& name) const;

private:
    std::unordered_map<std::string, SpriteClip> animationClips;
    std::shared_ptr<Texture> texture;
    int frameWidth, frameHeight;
    int columns, rows;
    int texWidth = 0, texHeight = 0;

    bool flipUV_X = false;
    bool flipUV_Y = false;
};

class SpriteAnimator
{
public:
    SpriteAnimator(std::shared_ptr<SpriteSheet> sheet_, float frameTime_, bool loop_ = true);

    void PlayClip(int start, int end, bool loop_ = true);
    void PlayClip(const std::string& clipName);

    void Update(float dt);

    [[nodiscard]] glm::vec2 GetUVOffset() const;
    [[nodiscard]] glm::vec2 GetUVScale() const;

    [[nodiscard]] std::shared_ptr<Texture> GetTexture() { return sheet ? sheet->GetTexture() : nullptr; }

    void SetFrame(int frame) { currentFrame = frame; }
    [[nodiscard]] int GetCurrentFrame() const { return currentFrame; }

    [[nodiscard]] std::shared_ptr<SpriteSheet> GetSpriteSheet() const { return sheet; }

    [[nodiscard]] bool IsClipFinished() const { return isClipFinished; }
    void SetPlaybackSpeed(float speed);
    float GetPlaybackSpeed() const { return playbackSpeed; }
private:
    std::shared_ptr<SpriteSheet> sheet;
    float frameTime;
    float elapsed = 0.0f;
    int currentFrame = 0;
    int startFrame = 0;
    int endFrame = 0;
    bool loop = true;
    const SpriteClip* playingClip = nullptr;
    int clipFrameIndex = 0;
    bool isClipFinished;
    float playbackSpeed = 1.0f;


};
