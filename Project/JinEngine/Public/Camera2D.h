#pragma once
#include "glm.hpp"

/**
 * @brief Represents a simple 2D camera used for view transformation and visibility tests.
 *
 * @details
 * Camera2D stores the camera position, zoom level, and screen size used by the engine's
 * 2D rendering pipeline.
 *
 * Its responsibilities include:
 * - storing the current camera position,
 * - storing the current zoom factor,
 * - providing a view matrix for rendering,
 * - checking whether an object is inside the current visible region.
 *
 * The view matrix is built by translating the world by the negative camera position
 * and then applying zoom scaling. Visibility tests are performed using a simple
 * bounding-radius check against the camera's viewport extents. :contentReference[oaicite:1]{index=1}
 *
 * @note
 * This camera is designed for 2D world rendering and does not include rotation.
 */
class Camera2D
{
public:
    /**
     * @brief Creates a 2D camera with the given screen size.
     *
     * @details
     * Initializes the camera with the provided screen width and height.
     * The initial camera position is (0, 0), and the initial zoom is 1.0f.
     *
     * @param screenWidth Initial viewport width in pixels.
     * @param screenHeight Initial viewport height in pixels.
     */
    Camera2D(int screenWidth = 800, int screenHeight = 600);

    /**
     * @brief Updates the camera's screen size.
     *
     * @details
     * Stores the new width and height used by this camera.
     * This is typically called when the window or render target size changes.
     *
     * @param width New screen width in pixels.
     * @param height New screen height in pixels.
     */
    void SetScreenSize(int width, int height);

    /**
     * @brief Returns the current screen width.
     *
     * @return Current viewport width in pixels.
     */
    [[nodiscard]] int GetScreenWidth() const { return screenWidth; }

    /**
     * @brief Returns the current screen height.
     *
     * @return Current viewport height in pixels.
     */
    [[nodiscard]] int GetScreenHeight() const { return screenHeight; }

    /**
     * @brief Sets the camera position.
     *
     * @details
     * Replaces the current camera position with the given world-space position.
     *
     * @param pos New camera position.
     */
    void SetPosition(const glm::vec2& pos);

    /**
     * @brief Adds an offset to the current camera position.
     *
     * @details
     * Moves the camera relative to its current position by the given offset vector.
     *
     * @param pos Offset to add to the current camera position.
     */
    void AddPosition(const glm::vec2& pos);

    /**
     * @brief Returns the current camera position.
     *
     * @return Reference to the current world-space camera position.
     */
    [[nodiscard]] const glm::vec2& GetPosition() const;

    /**
     * @brief Sets the camera zoom level.
     *
     * @details
     * Stores the zoom value after clamping it to the supported range of
     * 0.1f to 100.0f. This prevents invalid or extreme zoom values from being used
     * by rendering and visibility calculations. :contentReference[oaicite:2]{index=2}
     *
     * @param z Requested zoom factor.
     */
    void SetZoom(float z);

    /**
     * @brief Returns the current zoom level.
     *
     * @return Current zoom factor.
     */
    [[nodiscard]] float GetZoom() const;

    /**
     * @brief Builds and returns the camera view matrix.
     *
     * @details
     * The returned matrix is constructed by first translating the world by the
     * negative camera position, then scaling by the current zoom value.
     *
     * This means objects are rendered relative to the camera position, and the
     * visible world size changes according to the zoom factor. :contentReference[oaicite:3]{index=3}
     *
     * @return 2D view matrix used for rendering.
     */
    [[nodiscard]] glm::mat4 GetViewMatrix() const;

    /**
     * @brief Tests whether a circular object is inside the visible region.
     *
     * @details
     * Performs a simple culling check using the object's position and bounding radius.
     * The function computes the camera-centered visible bounds from the provided
     * viewport size and current zoom, then checks whether the object overlaps
     * that visible region.
     *
     * This is typically used as a lightweight visibility test before render submission. :contentReference[oaicite:4]{index=4}
     *
     * @param pos World-space center position of the object.
     * @param radius Bounding radius of the object.
     * @param viewportSize Viewport size used for the visibility calculation.
     * @return True if the object overlaps the visible region, otherwise false.
     */
    [[nodiscard]] bool IsInView(const glm::vec2& pos, float radius, glm::vec2 viewportSize) const;

private:
    glm::vec2 position = glm::vec2(0.0f);
    float zoom = 1.0f;
    int screenWidth = 800;
    int screenHeight = 600;
};