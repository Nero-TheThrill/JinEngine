#pragma once
#include "EngineContext.h"

/**
 * @brief Utility structure for frame timing and FPS calculation.
 *
 * @details
 * EngineTimer provides basic timing functionality used by the engine loop.
 * It measures delta time between frames and optionally computes frames-per-second (FPS)
 * over a fixed accumulation interval.
 *
 * The timer relies on time information provided through EngineContext
 * (typically sourced from a high-resolution platform timer).
 *
 * This structure is intended to be owned and driven by the core engine loop.
 */
struct EngineTimer
{
    /**
     * @brief Initializes the timer state.
     *
     * @details
     * Resets internal time tracking values and prepares the timer
     * for the first Tick call.
     */
    void Start();

    /**
     * @brief Advances the timer and computes delta time for the current frame.
     *
     * @details
     * This function retrieves the current time from EngineContext,
     * computes the elapsed time since the previous frame,
     * updates internal accumulators, and returns delta time in seconds.
     *
     * The returned delta time is typically used to drive
     * frame-based updates throughout the engine.
     *
     * @param engineContext Engine context providing the current time source.
     * @return Delta time (seconds) since the last Tick call.
     */
    [[nodiscard]] float Tick(const EngineContext& engineContext);

    /**
     * @brief Determines whether an updated FPS value should be reported.
     *
     * @details
     * This function accumulates frame count and elapsed time internally.
     * When a predefined time interval has elapsed (commonly 1 second),
     * it computes the average FPS, writes it to outFPS,
     * resets the internal FPS counters, and returns true.
     *
     * If the interval has not yet elapsed, the function returns false
     * and outFPS remains unmodified.
     *
     * @param outFPS Output parameter that receives the calculated FPS value.
     * @return true if a new FPS value was computed; false otherwise.
     */
    [[nodiscard]] bool ShouldUpdateFPS(float& outFPS);

    /**
     * @brief Time value recorded during the previous frame.
     *
     * @details
     * Used to compute delta time in Tick.
     */
    float lastTime = 0.0f;

    /**
     * @brief Accumulated elapsed time used for FPS calculation.
     */
    float fpsTimer = 0.0f;

    /**
     * @brief Number of frames accumulated since the last FPS update.
     */
    int frameCount = 0;
};
