#pragma once
#include "glm.hpp"

/**
 * @brief Represents a 2D transform with position, rotation, scale, and depth.
 *
 * @details
 * Transform2D encapsulates spatial data for objects in 2D space, including
 * position, rotation (in radians), scale, and rendering depth. It maintains
 * a transformation matrix that is lazily updated only when values change,
 * minimizing redundant matrix computations.
 */
class Transform2D
{
public:
    /**
     * @brief Constructs a new Transform2D with default values.
     *
     * @details
     * Initializes position = (0,0), rotation = 0, scale = (1,1),
     * depth = 0, and matrix = identity. The `isChanged` flag is set to true
     * so the first call to `GetMatrix()` recomputes the transform.
     */
    Transform2D()
        : position(0.f), rotation(0.f), scale(1.f),
        matrix(1.f), isChanged(true), depth(0.f)
    {
    }

    /**
     * @brief Sets the world position of the transform.
     *
     * @param pos New 2D position vector.
     */
    void SetPosition(const glm::vec2& pos)
    {
        position = pos;
        isChanged = true;
    }

    /**
     * @brief Adds an offset to the current position.
     *
     * @param pos 2D vector offset to add to the current position.
     */
    void AddPosition(const glm::vec2& pos)
    {
        position += pos;
        isChanged = true;
    }

    /**
     * @brief Sets the rotation of the transform in radians.
     *
     * @param rot Rotation angle in radians.
     */
    void SetRotation(float rot)
    {
        rotation = rot;
        isChanged = true;
    }

    /**
     * @brief Adds a rotation offset in radians to the current rotation.
     *
     * @param rot Angle in radians to add.
     */
    void AddRotation(float rot)
    {
        rotation += rot;
        isChanged = true;
    }

    /**
     * @brief Sets the scale of the transform.
     *
     * @param scl New 2D scale vector.
     */
    void SetScale(const glm::vec2& scl)
    {
        scale = scl;
        isChanged = true;
    }

    /**
     * @brief Adds an incremental scale factor.
     *
     * @param scl Scale vector to add to the current scale.
     */
    void AddScale(const glm::vec2& scl)
    {
        scale += scl;
        isChanged = true;
    }

    /**
     * @brief Sets the rendering depth value.
     *
     * @details
     * Depth is used for rendering order. Smaller values will be rendered
     * behind bigger ones.
     *
     * @param dpth New depth value.
     */
    void SetDepth(float dpth)
    {
        depth = dpth;
        isChanged = true;
    }

    /**
     * @brief Gets the current position.
     *
     * @return const reference to the 2D position vector.
     */
    [[nodiscard]] const glm::vec2& GetPosition() const { return position; }

    /**
     * @brief Gets the current rotation in radians.
     *
     * @return Rotation angle in radians.
     */
    [[nodiscard]] float GetRotation() const { return rotation; }

    /**
     * @brief Gets the current scale vector.
     *
     * @return const reference to the 2D scale vector.
     */
    [[nodiscard]] const glm::vec2& GetScale() const { return scale; }

    /**
     * @brief Gets the current depth value.
     *
     * @return Depth value.
     */
    [[nodiscard]] float GetDepth() const { return depth; }

    /**
     * @brief Retrieves the transformation matrix.
     *
     * @details
     * If any property (position, rotation, scale, depth) has changed,
     * this function recalculates the matrix as T * R * S, where:
     * - T = translation matrix
     * - R = rotation around Z axis
     * - S = scale matrix
     *
     * Otherwise, it returns the cached matrix.
     *
     * @return Reference to the 4x4 transformation matrix.
     * @code
     * glm::mat4 model = transform.GetMatrix();
     * @endcode
     */
    [[nodiscard]] glm::mat4& GetMatrix();

private:
    glm::vec2 position;   ///< 2D position
    float depth;          ///< Rendering depth
    float rotation;       ///< Rotation in radians
    glm::vec2 scale;      ///< 2D scale factor
    glm::mat4 matrix;     ///< Cached transformation matrix
    bool isChanged;       ///< Flag indicating if matrix needs recomputation
};
