#pragma once
#include "glm.hpp"

/**
 * @brief 2D camera providing view transformation and visibility tests.
 *
 * @details
 * Camera2D represents a simple orthographic-style camera for 2D rendering.
 * It maintains position, zoom, and screen size information and provides:
 * - View matrix generation for rendering
 * - Screen size management
 * - World-space visibility testing for basic view culling
 *
 * The camera operates in world space using a position and a uniform zoom factor.
 * Screen size is used to determine the visible region in world coordinates.
 */
class Camera2D
{
public:
    /**
     * @brief Constructs a Camera2D with an initial screen size.
     *
     * @details
     * Initializes the camera at the origin with a default zoom of 1.0.
     *
     * @param screenWidth Initial screen width in pixels.
     * @param screenHeight Initial screen height in pixels.
     */
    Camera2D(int screenWidth = 800, int screenHeight = 600);

    /**
     * @brief Sets the screen size used by the camera.
     *
     * @details
     * This function should be called when the window is resized.
     *
     * @param width New screen width in pixels.
     * @param height New screen height in pixels.
     */
    void SetScreenSize(int width, int height);

    /**
     * @brief Returns the current screen width.
     */
    [[nodiscard]] int GetScreenWidth() const { return screenWidth; }

    /**
     * @brief Returns the current screen height.
     */
    [[nodiscard]] int GetScreenHeight() const { return screenHeight; }

    /**
     * @brief Sets the world-space position of the camera.
     *
     * @param pos New camera position in world space.
     */
    void SetPosition(const glm::vec2& pos);

    /**
     * @brief Translates the camera by the given offset.
     *
     * @param pos Offset to add to the current camera position.
     */
    void AddPosition(const glm::vec2& pos);

    /**
     * @brief Returns the current world-space position of the camera.
     */
    [[nodiscard]] const glm::vec2& GetPosition() const;

    /**
     * @brief Sets the camera zoom factor.
     *
     * @details
     * Values greater than 1.0 zoom in, while values less than 1.0 zoom out.
     *
     * @param z New zoom factor.
     */
    void SetZoom(float z);

    /**
     * @brief Returns the current camera zoom factor.
     */
    [[nodiscard]] float GetZoom() const;

    /**
     * @brief Computes and returns the view matrix for this camera.
     *
     * @details
     * The view matrix is constructed from the camera position and zoom,
     * and is intended to be combined with a projection matrix
     * in the rendering pipeline.
     *
     * @return View transformation matrix.
     */
    [[nodiscard]] glm::mat4 GetViewMatrix() const;

    /**
     * @brief Tests whether a circular object is inside the camera view.
     *
     * @details
     * This function performs a simple visibility test by checking whether
     * a circle defined by a center position and radius intersects the
     * camera's visible world-space rectangle.
     *
     * It is commonly used for view frustum culling in 2D scenes.
     *
     * @param pos Center position of the object in world space.
     * @param radius Bounding radius of the object.
     * @param viewportSize Size of the viewport in world units.
     * @return true if the object is considered visible; false otherwise.
     */
    [[nodiscard]] bool IsInView(const glm::vec2& pos, float radius, glm::vec2 viewportSize) const;

private:
    glm::vec2 position = glm::vec2(0.0f);
    float zoom = 1.0f;
    int screenWidth = 800;
    int screenHeight = 600;
};
