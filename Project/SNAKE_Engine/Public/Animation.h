#pragma once
#include <unordered_map>

#include "vec2.hpp"
#include "Texture.h"

/**
 * @brief Represents a single frame of a sprite sheet.
 *
 * @details
 * Each SpriteFrame stores UV coordinates for texture sampling,
 * the pixel size of the frame, and an optional offset for alignment.
 */
struct SpriteFrame
{
    glm::vec2 uvTopLeft;       ///< Top-left UV coordinate of the frame.
    glm::vec2 uvBottomRight;   ///< Bottom-right UV coordinate of the frame.
    glm::ivec2 pixelSize;      ///< Pixel dimensions of this frame.
    glm::ivec2 offset;         ///< Alignment offset in pixels.
};

/**
 * @brief Represents an animation clip within a sprite sheet.
 *
 * @details
 * Stores a sequence of frame indices, the duration of each frame,
 * and whether the animation loops.
 */
struct SpriteClip
{
    std::vector<int> frameIndices; ///< Ordered list of frame indices.
    float frameDuration;           ///< Duration per frame (seconds).
    bool looping;                  ///< True if the animation should repeat.
};

/**
 * @brief Represents a grid-based sprite sheet backed by a texture.
 *
 * @details
 * The SpriteSheet divides a texture into frames of fixed width and height,
 * supports querying UV offsets/scales for a given frame, and can store
 * named animation clips referencing frame sequences.
 */
class SpriteSheet
{
public:
    /**
     * @brief Construct a sprite sheet.
     *
     * @param texture_ Pointer to the backing texture.
     * @param frameW   Width of each frame in pixels.
     * @param frameH   Height of each frame in pixels.
     */
    SpriteSheet(Texture* texture_, int frameW, int frameH);

    /**
     * @brief Get UV offset for a frame index.
     *
     * @param frameIndex Index of the frame in the grid.
     * @return UV offset (top-left coordinate).
     */
    [[nodiscard]] glm::vec2 GetUVOffset(int frameIndex) const;

    /**
     * @brief Get UV scaling factor for a single frame.
     *
     * @return UV scale size relative to texture dimensions.
     */
    [[nodiscard]] glm::vec2 GetUVScale() const;

    /**
     * @brief Access the backing texture.
     *
     * @return Non-null texture pointer.
     */
    [[nodiscard]] Texture* GetTexture() const { return texture; }

    /**
     * @brief Get total number of frames in the sheet.
     *
     * @return Count of frames (rows * columns).
     */
    [[nodiscard]] int GetFrameCount() const;

    /**
     * @brief Add a named animation clip.
     *
     * @param name Name of the clip.
     * @param frames Frame indices included in the clip.
     * @param frameDuration Duration per frame (seconds).
     * @param looping Whether the clip should loop (default true).
     */
    void AddClip(const std::string& name, const std::vector<int>& frames, float frameDuration, bool looping = true);

    /**
     * @brief Retrieve a clip by name.
     *
     * @param name Name of the clip.
     * @return Pointer to the SpriteClip if found, otherwise nullptr.
     */
    [[nodiscard]] const SpriteClip* GetClip(const std::string& name) const;

private:
    std::unordered_map<std::string, SpriteClip> animationClips; ///< Map of clip name → SpriteClip.
    Texture* texture; ///< Backing texture.
    int frameWidth, frameHeight; ///< Frame dimensions in pixels.
    int columns, rows; ///< Grid subdivision counts.
    int texWidth = 0, texHeight = 0; ///< Backing texture dimensions.

    bool flipUV_X = false; ///< Whether to flip horizontally.
    bool flipUV_Y = false; ///< Whether to flip vertically.
};

/**
 * @brief Time-based animator for playing sprite sheet frames and clips.
 *
 * @details
 * The SpriteAnimator advances frames based on elapsed time and plays
 * either a continuous frame range or a named SpriteClip.
 */
class SpriteAnimator
{
public:
    /**
     * @brief Construct a sprite animator.
     *
     * @param sheet_ Pointer to the sprite sheet.
     * @param frameTime_ Duration of each frame when not using a clip.
     * @param loop_ Whether playback should loop (default true).
     */
    SpriteAnimator(SpriteSheet* sheet_, float frameTime_, bool loop_ = true);

    /**
     * @brief Play a frame range as an animation.
     *
     * @param start Start frame index (inclusive).
     * @param end   End frame index (inclusive).
     * @param loop_ Whether to loop this range (default true).
     */
    void PlayClip(int start, int end, bool loop_ = true);

    /**
     * @brief Play a named SpriteClip.
     *
     * @param clipName Name of the clip in the sprite sheet.
     */
    void PlayClip(const std::string& clipName);

    /**
     * @brief Advance animation based on elapsed time.
     *
     * @param dt Delta time (seconds).
     */
    void Update(float dt);

    /**
     * @brief Get UV offset of the current frame.
     *
     * @return UV offset vector.
     */
    [[nodiscard]] glm::vec2 GetUVOffset() const;

    /**
     * @brief Get UV scale of the current frame.
     *
     * @return UV scale vector.
     */
    [[nodiscard]] glm::vec2 GetUVScale() const;

    /**
     * @brief Access the backing texture.
     *
     * @return Pointer to the texture from the sprite sheet.
     */
    [[nodiscard]] Texture* GetTexture() { return sheet->GetTexture(); }

    /**
     * @brief Force the current frame index.
     *
     * @param frame New frame index.
     */
    void SetFrame(int frame) { currentFrame = frame; }

    /**
     * @brief Get the current frame index.
     *
     * @return Frame index.
     */
    [[nodiscard]] int GetCurrentFrame() const { return currentFrame; }

    /**
     * @brief Access the sprite sheet.
     *
     * @return Pointer to the sprite sheet.
     */
    [[nodiscard]] SpriteSheet* GetSpriteSheet() const { return sheet; }

private:
    SpriteSheet* sheet;       ///< Backing sprite sheet.
    float frameTime;          ///< Duration per frame when playing ranges.
    float elapsed = 0.0f;     ///< Elapsed time accumulator.
    int currentFrame = 0;     ///< Current frame index.
    int startFrame = 0;       ///< Range start frame index.
    int endFrame = 0;         ///< Range end frame index.
    bool loop = true;         ///< Whether playback should loop.
    const SpriteClip* playingClip = nullptr; ///< Currently playing clip (if any).
    int clipFrameIndex = 0;   ///< Index into the playing clip's frame sequence.
};
