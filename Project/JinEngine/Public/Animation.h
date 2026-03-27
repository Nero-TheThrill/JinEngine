#pragma once
#include <unordered_map>
#include <functional>
#include <vector>

#include "vec2.hpp"
#include "Texture.h"

/**
 * @brief Stores per-frame UV data and optional layout metadata for a sprite frame.
 *
 * @details
 * This structure represents one logical frame inside a sprite sheet.
 * It stores the UV rectangle used to sample the frame from the texture,
 * along with optional pixel-space size and offset values that may be used
 * for alignment, layout correction, or future frame-specific rendering logic.
 *
 * In the current animation flow, the engine mainly relies on frame indices
 * and grid-based UV lookup through SpriteSheet. This structure is still useful
 * as a general-purpose frame description container.
 */
struct SpriteFrame
{
    glm::vec2 uvTopLeft;
    glm::vec2 uvBottomRight;
    glm::ivec2 pixelSize;
    glm::ivec2 offset;
};

/**
 * @brief Describes a named sprite animation clip.
 *
 * @details
 * A SpriteClip groups multiple frame indices into a reusable animation sequence.
 * The animation advances through frameIndices in order using frameDuration as the
 * time interval for each step.
 *
 * If looping is enabled, playback returns to the first local frame after reaching
 * the end of the sequence. Otherwise, playback remains on the last frame and the
 * animator marks the clip as finished.
 */
struct SpriteClip
{
    std::vector<int> frameIndices;
    float frameDuration;
    bool looping;
};

/**
 * @brief Represents a grid-based sprite sheet and stores named animation clips.
 *
 * @details
 * SpriteSheet interprets a texture as a regular grid of frames of equal size.
 * The constructor calculates the number of columns and rows from the texture
 * dimensions and the frame dimensions. If the calculated values become zero,
 * the implementation clamps them to at least one so that frame lookup remains valid.
 *
 * This class is responsible for:
 * - converting frame indices into UV offsets,
 * - returning normalized UV scale for one frame,
 * - storing named SpriteClip entries for clip-based animation playback.
 *
 * The UV calculation flips the row index vertically so that sprite frame lookup
 * matches the engine's texture coordinate convention. This behavior is implemented
 * in GetUVOffset() rather than being handled externally. :contentReference[oaicite:1]{index=1}
 *
 * @note
 * This class does not manage playback time or frame advancement.
 * It only provides frame layout data and clip lookup.
 */
class SpriteSheet
{
public:
    /**
     * @brief Creates a sprite sheet from a texture and fixed frame dimensions.
     *
     * @details
     * Stores the texture, reads its full width and height, and computes how many
     * frames fit horizontally and vertically based on the given frame size.
     *
     * The constructor also guards against zero column or row counts by forcing them
     * to at least one. This prevents invalid division or indexing behavior when the
     * frame size is larger than the texture size. :contentReference[oaicite:2]{index=2}
     *
     * @param texture_ Shared texture used as the source image for the sprite sheet.
     * @param frameW Width of a single frame in pixels.
     * @param frameH Height of a single frame in pixels.
     */
    SpriteSheet(std::shared_ptr<Texture> texture_, int frameW, int frameH);

    /**
     * @brief Returns the UV offset of the specified frame.
     *
     * @details
     * Converts a zero-based frame index into a column and row position inside the
     * sprite sheet grid. The row is then vertically flipped before converting the
     * result into normalized UV coordinates.
     *
     * The returned value represents the UV origin of the frame region. The per-frame
     * UV size is provided separately by GetUVScale().
     *
     * @param frameIndex Zero-based frame index within the sheet.
     * @return Top-left UV offset of the requested frame.
     *
     * @note
     * This function does not validate whether the given frame index is within range.
     * Callers are expected to use valid indices.
     */
    [[nodiscard]] glm::vec2 GetUVOffset(int frameIndex) const;

    /**
     * @brief Returns the normalized UV scale of a single frame.
     *
     * @details
     * Computes the width and height of one frame in normalized texture space.
     * The returned value can be combined with GetUVOffset() to sample the correct
     * region of the sprite sheet texture.
     *
     * @return UV scale where x is frameWidth / textureWidth and y is frameHeight / textureHeight.
     */
    [[nodiscard]] glm::vec2 GetUVScale() const;

    /**
     * @brief Returns the texture used by this sprite sheet.
     *
     * @details
     * Provides access to the underlying texture so that animation and rendering
     * systems can bind the correct resource during draw submission.
     *
     * @return Shared pointer to the source texture.
     */
    [[nodiscard]] std::shared_ptr<Texture> GetTexture() const { return texture; }

    /**
     * @brief Returns the total number of frames in the grid.
     *
     * @details
     * The total frame count is computed as the product of the number of columns
     * and rows derived from the texture and frame sizes.
     *
     * @return Total number of addressable frames in this sprite sheet.
     */
    [[nodiscard]] int GetFrameCount() const;

    /**
     * @brief Adds or replaces a named animation clip.
     *
     * @details
     * Creates a SpriteClip from the provided frame sequence, frame duration, and
     * looping flag, then stores it in the internal clip map under the given name.
     *
     * If a clip with the same name already exists, the new clip replaces the old one.
     * This makes it possible to redefine animation data without changing the public API. :contentReference[oaicite:3]{index=3}
     *
     * @param name Name used to identify the clip.
     * @param frames Ordered frame indices used by the clip.
     * @param frameDuration Playback duration of one frame in seconds.
     * @param looping Whether the clip restarts after reaching the last local frame.
     */
    void AddClip(const std::string& name, const std::vector<int>& frames, float frameDuration, bool looping = true);

    /**
     * @brief Returns a pointer to a named animation clip.
     *
     * @details
     * Searches the internal clip map for the requested name.
     * If found, returns a pointer to the stored clip. Otherwise returns nullptr.
     *
     * The returned pointer refers to internal storage owned by SpriteSheet.
     * It remains valid as long as the corresponding entry is not replaced or removed. :contentReference[oaicite:4]{index=4}
     *
     * @param name Name of the clip to search for.
     * @return Pointer to the clip if found, otherwise nullptr.
     */
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

/**
 * @brief Controls sprite animation playback using a SpriteSheet.
 *
 * @details
 * SpriteAnimator supports two playback modes:
 * 1. raw frame range playback using PlayClip(int start, int end, bool loop_),
 * 2. named clip playback using PlayClip(const std::string& clipName).
 *
 * During Update(), the animator advances elapsed time and changes frames when the
 * accumulated time reaches the current frame duration. For named clips, the duration
 * comes from SpriteClip::frameDuration. For raw frame range playback, the duration
 * comes from frameTime.
 *
 * This class also supports:
 * - global playback speed control,
 * - per-clip playback speed overrides,
 * - frame callbacks by absolute frame index,
 * - frame callbacks by clip-local frame index,
 * - one-shot callbacks that remove themselves after execution.
 *
 * A key behavior of this implementation is that PlayClip() immediately triggers
 * callbacks for the current starting frame instead of waiting for the next Update().
 * This means callbacks registered for the initial frame can fire as soon as playback
 * begins. :contentReference[oaicite:5]{index=5}
 *
 * @note
 * The animator does not own the texture directly. It queries the current SpriteSheet
 * when UV scale, UV offset, or texture access is needed.
 */
class SpriteAnimator
{
public:
    /**
     * @brief Creates a sprite animator bound to a sprite sheet.
     *
     * @details
     * Stores the sprite sheet and initializes default playback settings.
     * If the provided frame time is zero, the implementation replaces it with a very
     * small positive value to prevent division-like timing issues during playback. :contentReference[oaicite:6]{index=6}
     *
     * @param sheet_ Sprite sheet used to resolve frames and texture data.
     * @param frameTime_ Default playback time per frame for raw frame-range playback.
     * @param loop_ Whether raw frame-range playback loops by default.
     */
    SpriteAnimator(std::shared_ptr<SpriteSheet> sheet_, float frameTime_, bool loop_ = true);

    /**
     * @brief Starts playback using an inclusive raw frame range.
     *
     * @details
     * Disables named clip playback, clears the current clip name, resets playback state,
     * and begins animating from the start frame toward the end frame.
     *
     * The animator immediately sets currentFrame to start, clears elapsed time,
     * resets the clip-local index, and marks the clip as not finished.
     * After the state is reset, callbacks for the current frame are triggered immediately. :contentReference[oaicite:7]{index=7}
     *
     * @param start First frame index of the playback range.
     * @param end Last frame index of the playback range.
     * @param loop_ Whether playback should wrap back to start after reaching end.
     */
    void PlayClip(int start, int end, bool loop_ = true);

    /**
     * @brief Starts playback using a named sprite clip.
     *
     * @details
     * Looks up the requested clip from the bound SpriteSheet.
     * If the sheet is null, or if the clip does not exist or contains no frames,
     * playback is not started and the function returns early.
     *
     * On success, the animator stores the clip pointer, resets local clip playback
     * state, sets currentFrame to the first frame in the clip, clears elapsed time,
     * and immediately triggers callbacks for that starting frame. :contentReference[oaicite:8]{index=8}
     *
     * @param clipName Name of the clip to play.
     *
     * @note
     * Clip-local callbacks use the clip-local frame index, not the absolute frame number.
     */
    void PlayClip(const std::string& clipName);

    /**
     * @brief Advances animation playback using delta time.
     *
     * @details
     * Accumulates elapsed time and advances frames whenever the active frame duration
     * has been exceeded.
     *
     * Behavior differs depending on playback mode:
     * - If a named clip is active, Update() advances clipFrameIndex through the clip's
     *   frameIndices vector using the clip's frameDuration.
     * - If no named clip is active, Update() advances currentFrame from startFrame to
     *   endFrame using the animator's default frameTime.
     *
     * Playback speed is affected by:
     * - the global playbackSpeed,
     * - the per-clip playback speed override if a named clip is active.
     *
     * When playback reaches the end:
     * - looping playback wraps to the first frame,
     * - non-looping playback stays on the last frame and sets isClipFinished to true.
     *
     * Whenever a new current frame becomes active, the animator triggers the relevant
     * callbacks for that frame. :contentReference[oaicite:9]{index=9}
     *
     * @param dt Delta time in seconds.
     */
    void Update(float dt);

    /**
     * @brief Returns the UV offset of the current frame.
     *
     * @details
     * Delegates the lookup to the bound SpriteSheet using currentFrame.
     * If no sprite sheet is assigned, returns a zero vector.
     *
     * @return UV offset of the current frame, or (0, 0) if no sheet exists.
     */
    [[nodiscard]] glm::vec2 GetUVOffset() const;

    /**
     * @brief Returns the UV scale of the active sprite sheet.
     *
     * @details
     * Delegates the lookup to the bound SpriteSheet.
     * If no sprite sheet is assigned, returns a unit vector.
     *
     * @return UV scale of one frame, or (1, 1) if no sheet exists.
     */
    [[nodiscard]] glm::vec2 GetUVScale() const;

    /**
     * @brief Returns the texture currently associated with the animator.
     *
     * @details
     * This is a convenience accessor that forwards the request to the bound
     * SpriteSheet if one exists.
     *
     * @return Shared pointer to the sprite texture, or nullptr if no sheet exists.
     */
    [[nodiscard]] std::shared_ptr<Texture> GetTexture() { return sheet ? sheet->GetTexture() : nullptr; }

    /**
     * @brief Sets the current absolute frame directly.
     *
     * @details
     * This function directly overwrites currentFrame without validating the range
     * and without changing clip playback state, elapsed time, or callback state.
     *
     * @param frame Absolute frame index to assign.
     *
     * @note
     * This is a direct state override. It does not automatically fire callbacks.
     */
    void SetFrame(int frame) { currentFrame = frame; }

    /**
     * @brief Returns the current absolute frame index.
     *
     * @return Current frame index used for UV lookup.
     */
    [[nodiscard]] int GetCurrentFrame() const { return currentFrame; }

    /**
     * @brief Returns the sprite sheet used by this animator.
     *
     * @return Shared pointer to the bound sprite sheet.
     */
    [[nodiscard]] std::shared_ptr<SpriteSheet> GetSpriteSheet() const { return sheet; }

    /**
     * @brief Returns whether the current non-looping playback has finished.
     *
     * @details
     * This flag becomes true when a non-looping clip or raw frame range reaches
     * its last frame and can no longer advance.
     *
     * @return True if the current playback has finished, otherwise false.
     */
    [[nodiscard]] bool IsClipFinished() const { return isClipFinished; }

    /**
     * @brief Sets the global playback speed multiplier.
     *
     * @details
     * The given speed affects all playback modes. If the value is less than or equal
     * to zero, the implementation clamps it to a very small positive value instead of
     * allowing zero or negative playback speed. :contentReference[oaicite:10]{index=10}
     *
     * @param speed Global playback speed multiplier.
     */
    void SetPlaybackSpeed(float speed);

    /**
     * @brief Returns the global playback speed multiplier.
     *
     * @return Current global playback speed.
     */
    float GetPlaybackSpeed() const { return playbackSpeed; }

    /**
     * @brief Assigns a playback speed override for a specific named clip.
     *
     * @details
     * When the animator is currently playing a named clip, Update() checks whether
     * that clip has an override speed in clipPlaybackSpeeds. If one exists, it is
     * multiplied into the final playback speed.
     *
     * Values less than or equal to a very small threshold are clamped upward to avoid
     * invalid or frozen timing behavior. :contentReference[oaicite:11]{index=11}
     *
     * @param clipName Name of the clip to modify.
     * @param speed Playback speed multiplier for that clip.
     */
    void SetClipPlaybackSpeed(const std::string& clipName, float speed);

    /**
     * @brief Returns the playback speed override of a specific clip.
     *
     * @details
     * If no override has been assigned, the function returns 1.0f.
     *
     * @param clipName Name of the clip to query.
     * @return Clip-specific playback speed multiplier, or 1.0f if no override exists.
     */
    float GetClipPlaybackSpeed(const std::string& clipName) const;

    /**
     * @brief Removes the playback speed override of a specific clip.
     *
     * @param clipName Name of the clip whose override should be removed.
     */
    void ClearClipPlaybackSpeed(const std::string& clipName);

    /**
     * @brief Registers a persistent callback for an absolute frame.
     *
     * @details
     * The callback is stored in frameCallbacks and is triggered whenever the animator
     * enters the specified absolute frame index. Persistent callbacks remain registered
     * until explicitly cleared.
     *
     * @param frame Absolute frame index that should trigger the callback.
     * @param callback Function to execute when that frame becomes active.
     */
    void AddFrameCallback(int frame, std::function<void()> callback);

    /**
     * @brief Registers a one-shot callback for an absolute frame.
     *
     * @details
     * This callback behaves like AddFrameCallback(), but removes itself immediately
     * after its first successful execution.
     *
     * @param frame Absolute frame index that should trigger the callback.
     * @param callback Function to execute once when that frame becomes active.
     */
    void AddFrameCallbackOnce(int frame, std::function<void()> callback);

    /**
     * @brief Registers a persistent callback for a clip-local frame index.
     *
     * @details
     * This callback is only meaningful when a named clip is being played.
     * The localFrameIndex refers to the position inside the clip's frameIndices array,
     * not the absolute frame number in the sprite sheet.
     *
     * For example, localFrameIndex == 0 means the first frame of the named clip,
     * regardless of which absolute frame number that entry maps to. :contentReference[oaicite:12]{index=12}
     *
     * @param clipName Name of the clip that owns the callback.
     * @param localFrameIndex Zero-based local frame index inside the clip.
     * @param callback Function to execute when that local clip frame becomes active.
     */
    void AddClipFrameCallback(const std::string& clipName, int localFrameIndex, std::function<void()> callback);

    /**
     * @brief Registers a one-shot callback for a clip-local frame index.
     *
     * @details
     * Behaves like AddClipFrameCallback(), but removes itself after the first execution.
     *
     * @param clipName Name of the clip that owns the callback.
     * @param localFrameIndex Zero-based local frame index inside the clip.
     * @param callback Function to execute once when that local clip frame becomes active.
     */
    void AddClipFrameCallbackOnce(const std::string& clipName, int localFrameIndex, std::function<void()> callback);

    /**
     * @brief Removes all absolute-frame callbacks registered for a frame.
     *
     * @param frame Absolute frame index whose callbacks should be erased.
     */
    void ClearFrameCallbacks(int frame);

    /**
     * @brief Removes all clip-local callbacks registered for a specific clip frame.
     *
     * @details
     * Erases the callback list for the given local frame inside the given clip.
     * If the clip no longer has any remaining callback entries afterward,
     * the implementation also removes the clip entry from the callback map. :contentReference[oaicite:13]{index=13}
     *
     * @param clipName Name of the clip whose callback entry should be removed.
     * @param localFrameIndex Zero-based local frame index inside the clip.
     */
    void ClearClipFrameCallbacks(const std::string& clipName, int localFrameIndex);

    /**
     * @brief Removes all registered absolute-frame and clip-frame callbacks.
     *
     * @details
     * Clears both callback storage maps completely.
     */
    void ClearAllCallbacks();

private:
    /**
     * @brief Stores one callback entry and its one-shot behavior flag.
     *
     * @details
     * CallbackEntry is used internally by both absolute-frame and clip-local callback
     * containers. If once is true, the entry is removed immediately after execution.
     */
    struct CallbackEntry
    {
        std::function<void()> callback;
        bool once = false;
    };

    /**
     * @brief Triggers callbacks registered for the given absolute frame.
     *
     * @details
     * Looks up the callback list for the specified absolute frame index,
     * executes every valid callback, and removes entries marked as one-shot.
     * If the callback list becomes empty after processing, the frame entry itself
     * is erased from the callback map. :contentReference[oaicite:14]{index=14}
     *
     * @param frame Absolute frame index to trigger.
     */
    void TriggerFrameCallbacks(int frame);

    /**
     * @brief Triggers callbacks registered for the current clip-local frame.
     *
     * @details
     * This function only works when a named clip is currently active.
     * It looks up callback storage by currentClipName and clipFrameIndex,
     * executes all valid callbacks, removes one-shot entries, and erases empty
     * containers after execution.
     *
     * @note
     * This function does nothing when raw frame-range playback is active.
     */
    void TriggerClipFrameCallbacks();

    /**
     * @brief Triggers all callbacks relevant to the current playback position.
     *
     * @details
     * Calls TriggerFrameCallbacks(currentFrame) first, then calls
     * TriggerClipFrameCallbacks(). This allows both absolute-frame callbacks and
     * clip-local callbacks to react when the current frame changes. :contentReference[oaicite:15]{index=15}
     */
    void TriggerAllCallbacksForCurrentFrame();

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
    std::unordered_map<std::string, float> clipPlaybackSpeeds;
    std::string currentClipName;

    std::unordered_map<int, std::vector<CallbackEntry>> frameCallbacks;
    std::unordered_map<std::string, std::unordered_map<int, std::vector<CallbackEntry>>> clipFrameCallbacks;
};