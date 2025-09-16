#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <cstdint>

struct ma_engine;
struct ma_sound;

class SNAKE_Engine;
using SoundInstanceID = uint64_t;

/**
 * @brief Manages sound resources and playback through miniaudio.
 *
 * @details
 * SoundManager loads audio files, creates playback instances, and provides
 * control over volume, pausing, resuming, and stopping. Each sound instance
 * is identified by a unique SoundInstanceID, which can be reused for efficiency.
 *
 * Sounds are registered by a string tag and can be controlled individually
 * or in groups (by tag or globally).
 */
class SoundManager
{
    friend SNAKE_Engine;

public:
    /**
     * @brief Constructs and initializes the sound manager.
     *
     * @details
     * Internally initializes the miniaudio engine. If initialization fails,
     * no audio playback will occur.
     */
    SoundManager();

    /**
     * @brief Destroys the sound manager and frees all audio resources.
     *
     * @details
     * Stops all active sounds, cleans up allocated channels,
     * and uninitializes the miniaudio engine.
     */
    ~SoundManager();

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    /**
     * @brief Loads a sound resource from disk.
     *
     * @param tag Unique string identifier for the sound.
     * @param filepath Path to the audio file.
     * @param loop Whether the sound should loop during playback.
     * @note
     * Loading a sound with an existing tag will overwrite its entry.
     */
    void LoadSound(const std::string& tag, const std::string& filepath, bool loop = false);

    /**
     * @brief Plays a sound by tag and returns an instance ID.
     *
     * @param tag Tag of the previously loaded sound.
     * @param volume Initial volume (0.0–1.0).
     * @param startTimeSec Optional start offset in seconds.
     * @return SoundInstanceID identifying the created instance.
     * @note
     * Returns 0 if the sound tag is invalid or playback creation fails.
     */
    [[maybe_unused]] SoundInstanceID Play(const std::string& tag,
        float volume = 1.0f,
        float startTimeSec = 0.0f);

    /**
     * @brief Sets the volume of a sound instance by its ID.
     *
     * @param id The SoundInstanceID to modify.
     * @param volume Volume level (0.0–1.0).
     */
    void SetVolumeByID(SoundInstanceID id, float volume);

    /**
     * @brief Sets the volume for all active instances of a given tag.
     *
     * @param tag Tag of the sound resource.
     * @param volume Volume level (0.0–1.0).
     */
    void SetVolumeByTag(const std::string& tag, float volume);

    /**
     * @brief Sets the volume for all active sounds globally.
     *
     * @param volume Volume level (0.0–1.0).
     */
    void SetVolumeAll(float volume);

    /**
     * @brief Types of playback control operations.
     */
    enum class SoundControlType { Pause, Resume, Stop };

    /**
     * @brief Controls a specific sound instance by ID.
     *
     * @param control Control operation (Pause, Resume, Stop).
     * @param id The SoundInstanceID to target.
     */
    void ControlByID(SoundControlType control, SoundInstanceID id);

    /**
     * @brief Controls all active instances of a sound tag.
     *
     * @param control Control operation (Pause, Resume, Stop).
     * @param tag Tag of the sound resource.
     */
    void ControlByTag(SoundControlType control, const std::string& tag);

    /**
     * @brief Controls all currently playing sounds.
     *
     * @param control Control operation (Pause, Resume, Stop).
     */
    void ControlAll(SoundControlType control);

private:
    /**
     * @brief Initializes the internal miniaudio engine.
     *
     * @return True if initialization succeeded, false otherwise.
     */
    bool Init();

    /**
     * @brief Updates the sound manager state.
     *
     * @details
     * Cleans up finished sound instances and recycles their IDs.
     * Typically called once per frame by the engine.
     */
    void Update();

    /**
     * @brief Cleans up all sound instances and channels.
     *
     * @details
     * Stops playback and clears internal data structures.
     */
    void Cleanup();

    /**
     * @brief Frees all resources and uninitializes the engine.
     *
     * @details
     * Called on destruction or when the engine shuts down.
     */
    void Free();

    /**
     * @brief Generates or reuses a unique SoundInstanceID.
     *
     * @details
     * If there are reusable IDs in the queue, one is popped and reused.
     * Otherwise, increments the `nextInstanceID`.
     *
     * @return A unique SoundInstanceID.
     */
    SoundInstanceID GenerateID();

private:
    ma_engine* engine = nullptr; ///< Underlying miniaudio engine instance.

    struct SoundInfo
    {
        std::string filepath; ///< File path of the loaded sound.
        bool loop = false;    ///< Whether this sound loops.
    };
    std::unordered_map<std::string, SoundInfo> sounds; ///< Loaded sound registry.

    struct SoundInstanceMA; ///< Forward-declared miniaudio-backed sound instance.
    std::unordered_map<std::string, std::vector<SoundInstanceMA*>> activeChannels; ///< Active instances grouped by tag.
    std::unordered_map<SoundInstanceID, SoundInstanceMA*> instanceMap; ///< Maps IDs to active instances.

    std::queue<SoundInstanceID> reusableIDs; ///< Queue of reusable instance IDs.
    SoundInstanceID nextInstanceID = 1;      ///< Next ID to assign if no reusable IDs available.
};
