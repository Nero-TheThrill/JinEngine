#pragma once
#include "glm.hpp"

/**
 * @brief A simple 2D camera for managing position, zoom, and screen size.
 *
 * @details
 * Camera2D provides functionality for transforming world coordinates into
 * screen-space via view matrices. It supports zoom, screen resizing,
 * and viewport culling checks. Typically used by RenderManager and
 * CameraManager to control scene rendering.
 */
class Camera2D
{
public:
    /**
     * @brief Construct a new Camera2D.
     *
     * @param screenWidth Initial screen width in pixels. Defaults to 800.
     * @param screenHeight Initial screen height in pixels. Defaults to 600.
     */
    Camera2D(int screenWidth = 800, int screenHeight = 600);

    /**
     * @brief Set the screen size for the camera.
     *
     * @param width New screen width in pixels.
     * @param height New screen height in pixels.
     */
    void SetScreenSize(int width, int height);

    /**
     * @brief Get the current screen width.
     *
     * @return Screen width in pixels.
     */
    [[nodiscard]] int GetScreenWidth() const { return screenWidth; }

    /**
     * @brief Get the current screen height.
     *
     * @return Screen height in pixels.
     */
    [[nodiscard]] int GetScreenHeight() const { return screenHeight; }

    /**
     * @brief Set the world position of the camera center.
     *
     * @param pos 2D vector representing the new camera position.
     */
    void SetPosition(const glm::vec2& pos);

    /**
     * @brief Move the camera by a given offset.
     *
     * @param pos 2D offset to add to the current position.
     */
    void AddPosition(const glm::vec2& pos);

    /**
     * @brief Get the current camera position in world space.
     *
     * @return 2D vector representing the camera position.
     */
    [[nodiscard]] const glm::vec2& GetPosition() const;

    /**
     * @brief Set the zoom level of the camera.
     *
     * @param z Zoom factor. Values >1 zoom in, values <1 zoom out.
     */
    void SetZoom(float z);

    /**
     * @brief Get the current zoom level.
     *
     * @return Zoom factor.
     */
    [[nodiscard]] float GetZoom() const;

    /**
     * @brief Compute and return the view matrix.
     *
     * @return 4x4 matrix representing the camera's view transformation.
     */
    [[nodiscard]] glm::mat4 GetViewMatrix() const;

    /**
     * @brief Determine if a world-space point is visible in the viewport.
     *
     * @param pos World-space position of the object.
     * @param radius Bounding radius of the object in world units.
     * @param viewportSize Size of the viewport in pixels (width, height).
     * @return True if the object is at least partially within the view, false otherwise.
     */
    [[nodiscard]] bool IsInView(const glm::vec2& pos, float radius, glm::vec2 viewportSize) const;

private:
    glm::vec2 position = glm::vec2(0.0f);  ///< Camera center position in world space.
    float zoom = 1.0f;                     ///< Zoom factor (scale applied to view).
    int screenWidth = 800;                 ///< Current screen width in pixels.
    int screenHeight = 600;                ///< Current screen height in pixels.
};
