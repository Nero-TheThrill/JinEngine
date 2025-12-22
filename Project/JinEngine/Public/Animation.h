#pragma once
#include <unordered_map>

#include "vec2.hpp"
#include "Texture.h"

/**
 * @brief Describes a single sprite frame region within a sprite sheet.
 *
 * @details
 * SpriteFrame stores precomputed UV coordinates and pixel-space properties
 * for one frame in a texture atlas/sprite sheet.
 *
 * The engine can use uvTopLeft/uvBottomRight to sample the correct region,
 * while pixelSize/offset can be used for layout or pivot adjustments.
 */
struct SpriteFrame
{
    glm::vec2 uvTopLeft;
    glm::vec2 uvBottomRight;
    glm::ivec2 pixelSize;
    glm::ivec2 offset;
};

/**
 * @brief Describes an animation clip as an ordered sequence of frame indices.
 *
 * @details
 * SpriteClip represents a named animation:
 * - frameIndices: the sequence of sprite sheet frame indices to play
 * - frameDuration: time per frame in seconds
 * - looping: whether the clip loops when reaching the end
 */
struct SpriteClip
{
    std::vector<int> frameIndices;
    float frameDuration;
    bool looping;
};

/**
 * @brief Utility class that maps a texture to a uniform grid of sprite frames.
 *
 * @details
 * SpriteSheet provides UV offset/scale data for a sprite sheet that is divided
 * into frameW x frameH cells (in pixels).
 *
 * The sheet also supports named animation clips via AddClip/GetClip. Clips store
 * sequences of frame indices and timing settings but do not advance time by themselves.
 *
 * SpriteSheet does not own the Texture pointer; it assumes the texture lifetime
 * is managed elsewhere (e.g., by RenderManager/resource registry).
 */
class SpriteSheet
{

public:
    /**
     * @brief Constructs a sprite sheet helper for a texture divided into a uniform grid.
     *
     * @details
     * The implementation is expected to:
     * - Cache texture dimensions (texWidth/texHeight)
     * - Compute grid columns/rows based on frameWidth/frameHeight
     *
     * @param texture_ Texture that contains the sprite sheet.
     * @param frameW Width of a single frame in pixels.
     * @param frameH Height of a single frame in pixels.
     */
    SpriteSheet(Texture* texture_, int frameW, int frameH);

    /**
     * @brief Returns the UV offset for the given frame index.
     *
     * @details
     * The returned value is typically the top-left UV coordinate of the frame cell.
     * Combined with GetUVScale(), this enables computing per-frame UVs in shaders:
     * uv = baseUV * scale + offset.
     *
     * @param frameIndex Index of the frame in row-major order.
     * @return UV offset for sampling this frame.
     */
    [[nodiscard]] glm::vec2 GetUVOffset(int frameIndex) const;

    /**
     * @brief Returns the uniform UV scale for one frame cell.
     *
     * @details
     * For a uniform grid, scale is typically:
     * { frameWidth / texWidth, frameHeight / texHeight }.
     *
     * @return UV scale for a single frame cell.
     */
    [[nodiscard]] glm::vec2 GetUVScale() const;

    /**
     * @brief Returns the underlying texture used by this sprite sheet.
     *
     * @return Raw pointer to the texture (not owned).
     */
    [[nodiscard]] Texture* GetTexture() const { return texture; }

    /**
     * @brief Returns the total number of frames in the grid.
     *
     * @details
     * Typically equals columns * rows.
     *
     * @return Total frame count.
     */
    [[nodiscard]] int GetFrameCount() const;

    /**
     * @brief Adds a named animation clip using explicit frame indices.
     *
     * @details
     * Stores a SpriteClip in animationClips under the provided name.
     * If a clip with the same name already exists, it is replaced.
     *
     * @param name Clip identifier used for playback lookup.
     * @param frames Ordered list of frame indices.
     * @param frameDuration Duration of each frame in seconds.
     * @param looping Whether the clip loops (default: true).
     *
     * @code
     * SpriteSheet* sheet = ...;
     * sheet->AddClip("Run", { 0, 1, 2, 3, 4, 5 }, 0.08f, true);
     * @endcode
     */
    void AddClip(const std::string& name, const std::vector<int>& frames, float frameDuration, bool looping = true);

    /**
     * @brief Retrieves a named animation clip if it exists.
     *
     * @param name Clip identifier.
     * @return Pointer to the stored SpriteClip, or nullptr if not found.
     */
    [[nodiscard]] const SpriteClip* GetClip(const std::string& name) const;

private:
    std::unordered_map<std::string, SpriteClip> animationClips;
    Texture* texture;
    int frameWidth, frameHeight;
    int columns, rows;
    int texWidth = 0, texHeight = 0;

    bool flipUV_X = false;
    bool flipUV_Y = false;
};

/**
 * @brief Time-based sprite sheet animation player.
 *
 * @details
 * SpriteAnimator advances a current frame over time and exposes per-frame UV data.
 *
 * Two playback modes are supported:
 * - Range-based playback via PlayClip(start, end)
 * - Named clip playback via PlayClip(clipName), using SpriteSheet::GetClip
 *
 * The animator does not own the SpriteSheet pointer.
 * It is expected that calling code pushes the returned UV offset/scale into
 * a Material/Shader uniform each frame (e.g., for atlas sampling).
 */
class SpriteAnimator
{
public:
    /**
     * @brief Constructs an animator bound to a sprite sheet.
     *
     * @param sheet_ Sprite sheet providing UV mapping.
     * @param frameTime_ Default time per frame (seconds) for range-based playback.
     * @param loop_ Whether range-based playback loops by default.
     */
    SpriteAnimator(SpriteSheet* sheet_, float frameTime_, bool loop_ = true);

    /**
     * @brief Plays a frame range animation [start, end].
     *
     * @details
     * This configures range-based playback and clears any named clip playback state.
     *
     * @param start Start frame index (inclusive).
     * @param end End frame index (inclusive).
     * @param loop_ Whether to loop when reaching the end.
     */
    void PlayClip(int start, int end, bool loop_ = true);

    /**
     * @brief Plays a named animation clip from the bound SpriteSheet.
     *
     * @details
     * This function looks up the clip using SpriteSheet::GetClip and, if found,
     * switches the animator into named-clip playback mode.
     *
     * @param clipName Name of the clip previously registered in SpriteSheet.
     */
    void PlayClip(const std::string& clipName);

    /**
     * @brief Advances the animation state by delta time.
     *
     * @details
     * Accumulates elapsed time and advances frames when enough time has passed.
     * When looping is disabled and the animation reaches the end, IsClipFinished()
     * can be used to detect completion.
     *
     * @param dt Delta time in seconds.
     */
    void Update(float dt);

    /**
     * @brief Returns the UV offset for the current frame.
     *
     * @return UV offset for the currently playing frame.
     */
    [[nodiscard]] glm::vec2 GetUVOffset() const;

    /**
     * @brief Returns the UV scale for a single frame cell.
     *
     * @return UV scale for a single frame.
     */
    [[nodiscard]] glm::vec2 GetUVScale() const;

    /**
     * @brief Returns the texture used by the bound sprite sheet.
     *
     * @return Texture pointer if sheet is valid; otherwise nullptr.
     */
    [[nodiscard]] Texture* GetTexture() { return sheet ? sheet->GetTexture() : nullptr; }

    /**
     * @brief Forces the current frame index.
     *
     * @param frame New current frame index.
     */
    void SetFrame(int frame) { currentFrame = frame; }

    /**
     * @brief Returns the current frame index.
     */
    [[nodiscard]] int GetCurrentFrame() const { return currentFrame; }

    /**
     * @brief Returns the sprite sheet currently bound to this animator.
     */
    [[nodiscard]] SpriteSheet* GetSpriteSheet() const { return sheet; }

    /**
     * @brief Returns whether the current non-looping animation has finished.
     *
     * @details
     * This flag is expected to be updated by Update() when the playback reaches the end
     * and looping is disabled.
     */
    [[nodiscard]] bool IsClipFinished() const { return isClipFinished; }
private:
    SpriteSheet* sheet;
    float frameTime;
    float elapsed = 0.0f;
    int currentFrame = 0;
    int startFrame = 0;
    int endFrame = 0;
    bool loop = true;
    const SpriteClip* playingClip = nullptr;
    int clipFrameIndex = 0;
    bool isClipFinished;
};
