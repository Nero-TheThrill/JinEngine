#pragma once
#include "glm.hpp"

/**
 * @brief Represents a 2D transform containing position, rotation, scale, and depth information.
 *
 * @details
 * Transform2D stores translation, rotation, scaling, and depth values for a 2D object and lazily
 * computes a transformation matrix when requested. Any modification to position, rotation, scale,
 * or depth marks the internal state as dirty via isChanged.
 *
 * The transformation matrix is only recomputed when GetMatrix() is called and the transform has
 * changed since the last computation. This avoids unnecessary matrix recalculations each frame.
 *
 * Depth is stored separately from position to allow explicit control over rendering order or
 * depth-based sorting without affecting the 2D transform matrix composition.
 */
class Transform2D
{
public:
    /**
     * @brief Constructs a default 2D transform.
     *
     * @details
     * Initializes position to (0, 0), rotation to 0, scale to (1, 1), depth to 0,
     * and the transformation matrix to identity. The transform is marked as changed
     * so the matrix will be computed on first access.
     */
    Transform2D()
        : position(0.f), rotation(0.f), scale(1.f),
        matrix(1.f), isChanged(true), depth(0.f)
    {
    }

    /**
     * @brief Sets the absolute position of the transform.
     *
     * @param pos New position in 2D space.
     */
    void SetPosition(const glm::vec2& pos)
    {
        position = pos;
        isChanged = true;
    }

    /**
     * @brief Adds an offset to the current position.
     *
     * @param pos Delta value to add to the current position.
     */
    void AddPosition(const glm::vec2& pos)
    {
        position += pos;
        isChanged = true;
    }

    /**
     * @brief Sets the absolute rotation of the transform.
     *
     * @details
     * Rotation is stored as a single float value, typically interpreted as radians.
     *
     * @param rot New rotation value.
     */
    void SetRotation(float rot)
    {
        rotation = rot;
        isChanged = true;
    }

    /**
     * @brief Adds a delta to the current rotation.
     *
     * @param rot Rotation delta to add.
     */
    void AddRotation(float rot)
    {
        rotation += rot;
        isChanged = true;
    }

    /**
     * @brief Sets the absolute scale of the transform.
     *
     * @param scl New scale vector.
     */
    void SetScale(const glm::vec2& scl)
    {
        scale = scl;
        isChanged = true;
    }

    /**
     * @brief Adds a delta to the current scale.
     *
     * @param scl Scale delta to add.
     */
    void AddScale(const glm::vec2& scl)
    {
        scale += scl;
        isChanged = true;
    }

    /**
     * @brief Sets the depth value of the transform.
     *
     * @details
     * Depth does not directly affect the transformation matrix but can be used
     * externally for render ordering or depth-based logic.
     *
     * @param dpth New depth value.
     */
    void SetDepth(float dpth)
    {
        depth = dpth;
        isChanged = true;
    }

    /**
     * @brief Returns the current position.
     */
    [[nodiscard]] const glm::vec2& GetPosition() const { return position; }

    /**
     * @brief Returns the current rotation value.
     */
    [[nodiscard]] float GetRotation() const { return rotation; }

    /**
     * @brief Returns the current scale.
     */
    [[nodiscard]] const glm::vec2& GetScale() const { return scale; }

    /**
     * @brief Returns the current depth value.
     */
    [[nodiscard]] float GetDepth() const { return depth; }

    /**
     * @brief Returns the transformation matrix for this transform.
     *
     * @details
     * If the transform has changed since the last call, the matrix is recomputed
     * using the current position, rotation, and scale. The updated matrix is cached
     * and reused until another modification occurs.
     *
     * @return Reference to the internally stored transformation matrix.
     */
    [[nodiscard]] glm::mat4& GetMatrix();

private:
    glm::vec2 position;
    float depth;
    float rotation;
    glm::vec2 scale;
    glm::mat4 matrix;
    bool isChanged;
};
