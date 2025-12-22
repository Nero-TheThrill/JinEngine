#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <cstdint>

struct ma_engine;
struct ma_sound;

class JinEngine;
using SoundInstanceID = uint64_t;

/**
 * @brief Manages sound asset registration and runtime playback instances using miniaudio.
 *
 * @details
 * SoundManager owns a miniaudio engine (ma_engine) and provides a tag-based interface for:
 * - Registering sound assets (tag -> filepath + loop setting).
 * - Spawning playback instances (each Play call creates a new ma_sound instance).
 * - Controlling active instances (pause/resume/stop) by instance ID, by tag, or globally.
 * - Cleaning up finished non-looping instances and recycling their IDs.
 *
 * Each Play() call loads the file and creates a dedicated playback instance. Active instances are
 * tracked in two ways:
 * - instanceMap: SoundInstanceID -> instance pointer
 * - activeChannels: tag -> list of active instance pointers
 *
 * IDs are recycled using a queue (reusableIDs) to avoid unbounded growth of SoundInstanceID.
 *
 * The miniaudio engine is created and initialized in Init(), and destroyed in Free().
 * Update() performs maintenance by calling Cleanup() to remove finished instances.
 */
class SoundManager
{
    friend JinEngine;

public:
    /**
     * @brief Constructs an empty SoundManager with no initialized audio engine.
     *
     * @details
     * The underlying miniaudio engine is created/initialized by Init(), which is expected to be
     * called by JinEngine during engine startup.
     */
    SoundManager();

    /**
     * @brief Destroys the SoundManager and releases all audio resources.
     *
     * @details
     * Calls Free() to stop and destroy all active instances and to uninitialize/destroy the
     * underlying miniaudio engine.
     */
    ~SoundManager();

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    /**
     * @brief Registers a sound asset under a tag.
     *
     * @details
     * This function records the filepath and default looping flag for the given tag.
     * It does not load audio data immediately; loading occurs when Play() is called.
     * If the tag already exists, the entry is overwritten.
     *
     * @param tag      Tag name used to reference the sound later.
     * @param filepath Path to the audio file to be loaded by miniaudio.
     * @param loop     Default looping behavior applied to instances created via Play().
     */
    void LoadSound(const std::string& tag, const std::string& filepath, bool loop = false);

    /**
     * @brief Plays a new instance of a previously registered sound.
     *
     * @details
     * If the engine is not initialized or the tag is not registered, this returns 0.
     *
     * A successful call:
     * - Creates a new SoundInstanceMA.
     * - Initializes a ma_sound from the file path recorded for the tag.
     * - Applies volume and looping state.
     * - Optionally seeks to startTimeSec (converted to PCM frame using the sound's sample rate
     *   if available, otherwise the engine sample rate).
     * - Starts playback and assigns a SoundInstanceID.
     * - Tracks the instance in instanceMap and activeChannels[tag].
     *
     * The returned ID can be used with ControlByID() / SetVolumeByID(). IDs are recycled after
     * instances are destroyed.
     *
     * @param tag          Registered sound tag.
     * @param volume       Initial volume applied to this instance.
     * @param startTimeSec Optional start offset in seconds.
     * @return Non-zero SoundInstanceID on success; 0 on failure.
     */
    [[maybe_unused]] SoundInstanceID Play(const std::string& tag,
        float volume = 1.0f,
        float startTimeSec = 0.0f);

    /**
     * @brief Sets the volume of a specific playback instance by ID.
     *
     * @details
     * If the ID is not found, this function does nothing.
     *
     * @param id     Playback instance ID returned by Play().
     * @param volume New volume value to apply.
     */
    void SetVolumeByID(SoundInstanceID id, float volume);

    /**
     * @brief Sets the volume of all active instances associated with a tag.
     *
     * @details
     * Only instances currently tracked in activeChannels[tag] are affected.
     * If the tag has no active instances, this function does nothing.
     *
     * @param tag    Sound tag whose active instances should be updated.
     * @param volume New volume value to apply.
     */
    void SetVolumeByTag(const std::string& tag, float volume);

    /**
     * @brief Sets the volume of all active instances across all tags.
     *
     * @details
     * Iterates all tags currently present in activeChannels and applies SetVolumeByTag().
     *
     * @param volume New volume value to apply globally.
     */
    void SetVolumeAll(float volume);

    /**
     * @brief Control command type used by ControlByID/ControlByTag/ControlAll.
     *
     * @details
     * - Pause: stores playback cursor, stops the sound, and marks the instance as paused.
     * - Resume: seeks to stored cursor and restarts if the instance is marked paused.
     * - Stop: stops, uninitializes, destroys the instance, removes tracking entries, and recycles its ID.
     */
    enum class SoundControlType { Pause, Resume, Stop };

    /**
     * @brief Applies a control command to a specific playback instance by ID.
     *
     * @details
     * Pause:
     * - Reads the cursor position in PCM frames and stores it.
     * - Stops the sound and marks the instance paused.
     *
     * Resume:
     * - If the instance is paused, seeks to the stored cursor and starts playback.
     *
     * Stop:
     * - Stops and uninitializes the ma_sound.
     * - Removes the instance pointer from activeChannels[inst->tag].
     * - Deletes the instance, erases it from instanceMap, and recycles the ID.
     *
     * If the ID does not exist, this function does nothing.
     *
     * @param control Control command.
     * @param id      Playback instance ID.
     */
    void ControlByID(SoundControlType control, SoundInstanceID id);

    /**
     * @brief Applies a control command to all active instances associated with a tag.
     *
     * @details
     * Pause/Resume:
     * - Iterates activeChannels[tag] and pauses/resumes each instance using cursor tracking.
     *
     * Stop:
     * - Stops and uninitializes each instance in activeChannels[tag], deletes them,
     *   removes corresponding entries from instanceMap, recycles IDs, and clears the vector.
     *
     * If the tag has no active instances, this function does nothing.
     *
     * @param control Control command.
     * @param tag     Sound tag whose active instances should be controlled.
     */
    void ControlByTag(SoundControlType control, const std::string& tag);

    /**
     * @brief Applies a control command to all active instances across all tags.
     *
     * @details
     * This collects a snapshot list of tags from activeChannels and calls ControlByTag()
     * for each tag, which avoids iterator invalidation while modifying activeChannels.
     *
     * @param control Control command.
     */
    void ControlAll(SoundControlType control);

private:
    /**
     * @brief Creates and initializes the miniaudio engine.
     *
     * @details
     * Allocates ma_engine if needed and calls ma_engine_init(). On failure, the engine
     * is deleted and the pointer is reset to nullptr.
     *
     * @return True on success; false on failure.
     */
    bool Init();

    /**
     * @brief Performs per-frame maintenance of sound instances.
     *
     * @details
     * Currently delegates to Cleanup() to remove finished non-looping instances
     * and to keep internal tracking containers consistent.
     */
    void Update();

    /**
     * @brief Removes finished playback instances and recycles their IDs.
     *
     * @details
     * For each tracked instance:
     * - Looping instances are kept even if not currently playing.
     * - Non-looping instances that reached the end are uninitialized, removed from activeChannels,
     *   deleted, removed from instanceMap, and their IDs are queued for reuse.
     *
     * After the main pass, activeChannels vectors are also pruned to remove nullptr entries and
     * entries that correspond to finished non-looping sounds.
     */
    void Cleanup();

    /**
     * @brief Stops all instances and destroys the miniaudio engine.
     *
     * @details
     * - Uninitializes and deletes all SoundInstanceMA objects tracked by instanceMap.
     * - Clears instanceMap, activeChannels, and the registered sound table.
     * - Clears the reusable ID queue and resets nextInstanceID.
     * - Uninitializes and deletes the ma_engine instance.
     */
    void Free();

    /**
     * @brief Produces a new SoundInstanceID, reusing IDs when available.
     *
     * @details
     * If reusableIDs contains any previously freed IDs, one is popped and returned.
     * Otherwise, nextInstanceID is returned and incremented.
     *
     * @return A non-zero SoundInstanceID.
     */
    SoundInstanceID GenerateID();

private:
    ma_engine* engine = nullptr;

    /**
     * @brief Registered sound metadata for tag-based playback.
     */
    struct SoundInfo
    {
        std::string filepath;
        bool loop = false;
    };
    std::unordered_map<std::string, SoundInfo> sounds;

    struct SoundInstanceMA;
    std::unordered_map<std::string, std::vector<SoundInstanceMA*>> activeChannels;
    std::unordered_map<SoundInstanceID, SoundInstanceMA*> instanceMap;

    std::queue<SoundInstanceID> reusableIDs;
    SoundInstanceID nextInstanceID = 1;
};
