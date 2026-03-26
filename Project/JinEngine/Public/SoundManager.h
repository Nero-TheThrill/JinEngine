#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <cstdint>
#include <functional>

struct ma_engine;
struct ma_sound;

class JinEngine;
using SoundInstanceID = uint64_t;

/**
 * @brief Manages sound resource registration, runtime playback instances, and global sound control for the engine.
 *
 * @details
 * SoundManager is the engine-level audio system built on top of miniaudio.
 * It stores lightweight sound metadata through LoadSound(), and creates an actual playback
 * instance only when Play() is called.
 *
 * Each Play() call creates a new runtime sound instance, assigns it a unique SoundInstanceID,
 * stores it in an internal instance map, and also groups it by tag so that sounds can be
 * controlled either individually or in batches.
 *
 * The manager supports:
 * - loading sound metadata by tag,
 * - starting multiple simultaneous instances from the same sound tag,
 * - per-instance, per-tag, or global volume control,
 * - per-instance, per-tag, or global pause/resume/stop control,
 * - automatic cleanup of finished non-looping sounds,
 * - optional completion callbacks for one-shot playback.
 *
 * Pause and resume are implemented by saving the current PCM cursor position,
 * stopping playback, and later seeking back to the saved frame before restarting.
 *
 * Finished non-looping sounds are not removed immediately at the exact moment playback ends.
 * They are collected and released during Update(), which internally calls Cleanup().
 * Because of that, the engine must call Update() regularly every frame for proper sound lifecycle management.
 *
 * @note
 * LoadSound() only registers file path and loop information. Actual miniaudio sound objects
 * are created per playback instance inside Play().
 *
 * @note
 * This class is owned and driven by the engine. JinEngine is declared as a friend so it can
 * access initialization, update, and shutdown functions.
 */
class SoundManager
{
    friend JinEngine;

public:
    /**
     * @brief Constructs an empty SoundManager.
     *
     * @details
     * The constructor initializes internal state only.
     * It does not initialize the underlying miniaudio engine automatically.
     * Actual audio backend initialization is performed by Init().
     *
     * The reusable ID queue starts empty, and the next generated sound instance ID starts from 1.
     */
    SoundManager();

    /**
     * @brief Destroys the SoundManager and releases all audio resources.
     *
     * @details
     * The destructor calls Free(), which:
     * - stops and uninitializes all active sound instances,
     * - deletes all runtime sound instance objects,
     * - clears the instance map and tag-grouped channel lists,
     * - clears registered sound metadata,
     * - resets reusable ID storage,
     * - uninitializes and deletes the miniaudio engine.
     *
     * @note
     * After destruction, all previously returned SoundInstanceID values become invalid.
     */
    ~SoundManager();

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    /**
     * @brief Registers a sound resource under a tag.
     *
     * @details
     * This function stores only the file path and loop flag in the internal sound registry.
     * It does not create or preload a miniaudio playback object at registration time.
     *
     * If the same tag already exists, its metadata is overwritten with the new file path and loop flag.
     *
     * @param tag User-defined lookup tag for the sound resource.
     * @param filepath Path to the sound file to be used when playback instances are created.
     * @param loop Whether instances created from this tag should loop automatically.
     *
     * @note
     * This function only prepares metadata for future playback.
     * Real sound instances are created later by Play().
     */
    void LoadSound(const std::string& tag, const std::string& filepath, bool loop = false);

    /**
     * @brief Plays a registered sound using its tag.
     *
     * @details
     * This overload creates a new runtime sound instance from the registered file path,
     * applies the requested volume, optionally seeks to the given start time, starts playback,
     * assigns a SoundInstanceID, and tracks the instance internally.
     *
     * Multiple calls with the same tag create multiple simultaneous playback instances.
     *
     * @param tag Tag of the sound registered with LoadSound().
     * @param volume Initial playback volume.
     * @param startTimeSec Start position in seconds. If greater than zero, playback seeks to that position before starting.
     *
     * @return A valid SoundInstanceID if playback starts successfully.
     * @return 0 if the audio engine is not initialized, the tag is missing, the file cannot be loaded, or playback start fails.
     *
     * @note
     * This overload does not register a completion callback.
     */
    [[maybe_unused]] SoundInstanceID Play(const std::string& tag,
        float volume = 1.0f,
        float startTimeSec = 0.0f);

    /**
     * @brief Plays a registered sound using its tag and registers a callback for one-shot completion.
     *
     * @details
     * This overload behaves the same as the simpler Play() overload, but also stores an optional
     * completion callback inside the runtime sound instance.
     *
     * When a non-looping sound reaches the end of playback, Cleanup() collects the callback and
     * executes it after internal cleanup has been prepared. The callback receives the finished
     * SoundInstanceID and the original sound tag.
     *
     * Looping sounds are not considered finished during normal playback, so their callback is not
     * triggered automatically unless the sound is allowed to reach a natural end as a non-looping instance.
     *
     * @param tag Tag of the sound registered with LoadSound().
     * @param volume Initial playback volume.
     * @param startTimeSec Start position in seconds. If greater than zero, playback seeks to that position before starting.
     * @param onFinished Callback invoked after a non-looping playback instance naturally reaches the end.
     *
     * @return A valid SoundInstanceID if playback starts successfully.
     * @return 0 if the audio engine is not initialized, the tag is missing, the file cannot be loaded, or playback start fails.
     *
     * @note
     * The callback is executed during Update() -> Cleanup(), not at the exact low-level audio completion moment.
     */
    [[maybe_unused]] SoundInstanceID Play(const std::string& tag,
        float volume,
        float startTimeSec,
        std::function<void(SoundInstanceID, const std::string&)> onFinished);

    /**
     * @brief Sets the playback volume of a specific active sound instance.
     *
     * @details
     * Looks up the runtime instance by SoundInstanceID and applies the new volume
     * directly to the underlying miniaudio sound object.
     *
     * If the ID does not exist or the instance is already gone, the function does nothing.
     *
     * @param id Runtime ID of the active playback instance.
     * @param volume New volume value to apply.
     */
    void SetVolumeByID(SoundInstanceID id, float volume);

    /**
     * @brief Sets the playback volume for all active instances created from the same sound tag.
     *
     * @details
     * This function iterates over all currently active runtime instances grouped under the given tag
     * and applies the new volume to each one.
     *
     * If no active instance exists for the tag, the function does nothing.
     *
     * @param tag Tag used when the sound was registered and played.
     * @param volume New volume value to apply to all active instances for that tag.
     *
     * @note
     * This affects only currently active instances. It does not change the default metadata stored by LoadSound().
     */
    void SetVolumeByTag(const std::string& tag, float volume);

    /**
     * @brief Sets the playback volume for all active sound instances.
     *
     * @details
     * This function iterates over every active tag group and applies the given volume
     * to every currently active runtime sound instance.
     *
     * @param volume New volume value to apply globally to all active sounds.
     */
    void SetVolumeAll(float volume);

    /**
     * @brief Defines the supported runtime control operations for sound playback instances.
     *
     * @details
     * Pause:
     * Saves the current PCM cursor, stops playback, and marks the instance as paused.
     *
     * Resume:
     * Seeks back to the saved PCM cursor and restarts playback only if the instance is currently paused.
     *
     * Stop:
     * Immediately stops playback, uninitializes the sound object, removes the instance from tracking containers,
     * deletes the runtime instance, and recycles its SoundInstanceID for future reuse.
     */
    enum class SoundControlType
    {
        Pause,
        Resume,
        Stop
    };

    /**
     * @brief Applies a playback control command to one active sound instance.
     *
     * @details
     * The behavior depends on the control type:
     *
     * - Pause:
     *   Saves the current playback cursor in PCM frames, stops the sound, and marks it as paused.
     *
     * - Resume:
     *   If the instance was previously paused, seeks back to the stored cursor and restarts playback.
     *
     * - Stop:
     *   Stops the sound immediately, uninitializes the miniaudio object, removes the instance from
     *   tag-based tracking, deletes the runtime object, erases the ID mapping, and recycles the ID.
     *
     * If the given ID does not exist, the function does nothing.
     *
     * @param control Runtime control command to apply.
     * @param id Runtime ID of the target playback instance.
     *
     * @note
     * Stop performs immediate destruction of the runtime sound instance.
     * After Stop, the ID is no longer valid.
     */
    void ControlByID(SoundControlType control, SoundInstanceID id);

    /**
     * @brief Applies a playback control command to all active sound instances associated with a tag.
     *
     * @details
     * The behavior depends on the control type:
     *
     * - Pause:
     *   Saves each instance's current playback cursor, stops playback, and marks the instance as paused.
     *
     * - Resume:
     *   Restarts only the instances currently marked as paused by seeking back to their saved cursor positions.
     *
     * - Stop:
     *   Immediately stops and uninitializes every instance under the tag, removes their IDs from instance tracking,
     *   recycles those IDs, deletes the runtime objects, and clears the tag's active channel list.
     *
     * If the tag has no active instances, the function does nothing.
     *
     * @param control Runtime control command to apply.
     * @param tag Tag whose currently active instances should be controlled.
     *
     * @note
     * Stop removes only currently active instances for that tag.
     * The registered sound metadata itself remains in the sounds registry.
     */
    void ControlByTag(SoundControlType control, const std::string& tag);

    /**
     * @brief Applies a playback control command to all active sound instances in the manager.
     *
     * @details
     * This function first copies all currently known active tags into a temporary list,
     * then calls ControlByTag() for each tag.
     *
     * Copying the tag list first avoids invalidation issues while ControlByTag(Stop, ...)
     * modifies internal tracking containers.
     *
     * @param control Runtime control command to apply globally.
     */
    void ControlAll(SoundControlType control);

private:
    /**
     * @brief Initializes the underlying miniaudio engine.
     *
     * @details
     * If the engine pointer is null, this function allocates a new ma_engine object
     * and then initializes it with ma_engine_init().
     *
     * On initialization failure, the engine object is deleted, the pointer is reset to null,
     * and the function returns false.
     *
     * @return true if the audio engine was initialized successfully.
     * @return false if initialization failed.
     *
     * @note
     * Playback functions return 0 immediately if Init() has not completed successfully.
     */
    bool Init();

    /**
     * @brief Updates the sound system for the current frame.
     *
     * @details
     * This function currently forwards to Cleanup().
     * It should be called regularly by the engine so that finished non-looping sounds are released,
     * stale tracking entries are removed, reusable IDs are recycled, and pending completion callbacks are executed.
     *
     * @note
     * If Update() is not called regularly, finished one-shot sounds may remain in tracking containers longer than intended.
     */
    void Update();

    /**
     * @brief Cleans up finished non-looping sound instances and executes pending completion callbacks.
     *
     * @details
     * This function scans all tracked runtime instances and identifies sound instances that:
     * - are not currently playing,
     * - are not looping,
     * - and have reached the end of the sound stream.
     *
     * For each such instance, Cleanup():
     * - queues its completion callback if one exists,
     * - uninitializes the miniaudio sound object,
     * - removes the instance pointer from its tag-based active channel list,
     * - deletes the runtime object,
     * - erases the ID from instanceMap,
     * - pushes the released ID into the reusable ID queue.
     *
     * After that, it also performs an additional pass over active channel vectors to remove stale pointers
     * that correspond to naturally finished non-looping instances.
     *
     * Finally, it executes queued callbacks after internal cleanup preparation is complete.
     *
     * @note
     * Looping sounds are intentionally skipped here and remain active until explicitly stopped.
     */
    void Cleanup();

    /**
     * @brief Releases every internal audio resource and resets the manager to its initial empty state.
     *
     * @details
     * This function:
     * - uninitializes and deletes all currently tracked sound instances,
     * - clears active channel groups,
     * - clears the registered sound metadata map,
     * - empties the reusable ID queue,
     * - resets the next instance ID counter back to 1,
     * - uninitializes and destroys the miniaudio engine.
     *
     * @note
     * This is called by the destructor and is intended for full system shutdown / full reset.
     */
    void Free();

    /**
     * @brief Generates a runtime ID for a new playback instance.
     *
     * @details
     * If there is an ID available in the reusable ID queue, that ID is returned first.
     * Otherwise, the manager returns the current nextInstanceID and then increments it.
     *
     * @return A SoundInstanceID to be assigned to a newly created playback instance.
     *
     * @note
     * IDs released by Stop() or Cleanup() may later be reused by newly played sound instances.
     */
    SoundInstanceID GenerateID();

private:
    ma_engine* engine = nullptr;

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