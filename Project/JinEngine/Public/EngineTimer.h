#pragma once
#include "EngineContext.h"

/**
 * @brief Frame timing utility used for delta time calculation and FPS measurement.
 *
 * @details
 * EngineTimer is responsible for tracking frame time progression and computing
 * per-frame delta time (dt) as well as frames per second (FPS).
 *
 * It is typically used inside the main engine loop:
 * - Start() initializes the timer.
 * - Tick() is called every frame to compute delta time.
 * - ShouldUpdateFPS() accumulates time and frame count to periodically update FPS.
 *
 * The timer relies on the window system's time source (e.g., glfwGetTime)
 * accessed through EngineContext.
 *
 * @note
 * This struct does not own any system and depends on EngineContext for time queries.
 */
struct EngineTimer
{
    /**
     * @brief Initializes the timer.
     *
     * @details
     * Captures the current time as the starting reference point.
     * This should be called once before entering the main loop.
     *
     * @note
     * Resets internal state used for delta time calculation.
     */
    void Start();

    /**
     * @brief Advances the timer by one frame and computes delta time.
     *
     * @details
     * Retrieves the current time from the engine (via EngineContext),
     * computes the difference from the previous frame (lastTime),
     * and updates lastTime with the current value.
     *
     * This function is expected to be called once per frame.
     *
     * @param engineContext Provides access to the engine's time source.
     * @return Delta time in seconds since the last frame.
     *
     * @note
     * A stable delta time is essential for frame-independent movement and simulation.
     */
    [[nodiscard]] float Tick(const EngineContext& engineContext);

    /**
     * @brief Determines whether FPS should be updated and computes it.
     *
     * @details
     * Accumulates elapsed time (fpsTimer) and frame count (frameCount).
     * When the accumulated time exceeds a threshold (commonly ~1 second),
     * FPS is calculated as:
     *
     *     FPS = frameCount / fpsTimer
     *
     * After computation, both counters are reset.
     *
     * @param outFPS Output parameter that receives the calculated FPS value.
     * @return True if FPS was updated this frame, false otherwise.
     *
     * @note
     * This function is typically used to update UI or debug overlays at a lower frequency.
     */
    [[nodiscard]] bool ShouldUpdateFPS(float& outFPS);

    /**
     * @brief Time value from the previous frame.
     *
     * @details
     * Used to compute delta time by subtracting from the current time.
     */
    float lastTime = 0.0f;

    /**
     * @brief Accumulated time used for FPS calculation.
     *
     * @details
     * Increases every frame by delta time until it reaches a threshold (e.g., 1 second).
     */
    float fpsTimer = 0.0f;

    /**
     * @brief Number of frames counted during the FPS accumulation period.
     *
     * @details
     * Incremented every frame and used together with fpsTimer to compute FPS.
     */
    int frameCount = 0;
};