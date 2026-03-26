#pragma once
#include "glm.hpp"

class Transform2D
{
public:
    /**
     * @brief Constructs a default 2D transform.
     *
     * @details
     * Initializes the transform with:
     * - position = (0, 0)
     * - rotation = 0
     * - scale = (1, 1)
     * - matrix = identity
     * - isChanged = true
     * - depth = 0
     *
     * The matrix is not rebuilt immediately here. Instead, it is marked dirty
     * and lazily recalculated the next time GetMatrix() is called.
     *
     * @note
     * The depth value is stored separately from the transform matrix and is used
     * for render ordering rather than geometric transformation.
     */
    Transform2D()
        : position(0.f), rotation(0.f), scale(1.f),
        matrix(1.f), isChanged(true), depth(0.f)
    {
    }

    /**
     * @brief Sets the world position of the transform.
     *
     * @details
     * Replaces the current 2D position with the given value and marks the
     * internal matrix as dirty so it will be rebuilt on the next GetMatrix() call.
     *
     * @param pos
     * New 2D position to assign.
     */
    void SetPosition(const glm::vec2& pos)
    {
        position = pos;
        isChanged = true;
    }

    /**
     * @brief Adds an offset to the current position.
     *
     * @details
     * Accumulates the given value onto the current position and marks the
     * transform matrix as dirty.
     *
     * This is useful for movement updates such as velocity-based translation.
     *
     * @param pos
     * Position delta to add.
     */
    void AddPosition(const glm::vec2& pos)
    {
        position += pos;
        isChanged = true;
    }

    /**
     * @brief Sets the rotation angle of the transform.
     *
     * @details
     * Replaces the current rotation value and marks the matrix as dirty.
     * The actual matrix rebuild happens lazily in GetMatrix().
     *
     * The rotation is applied around the Z axis in the generated 4x4 matrix.
     *
     * @param rot
     * New rotation angle.
     *
     * @note
     * This implementation passes the stored value directly into glm::rotate().
     * In the current codebase, that means the unit must match what the rest of
     * your engine expects when building transforms.
     */
    void SetRotation(float rot)
    {
        rotation = rot;
        isChanged = true;
    }

    /**
     * @brief Adds to the current rotation angle.
     *
     * @details
     * Accumulates the given rotation amount and marks the cached matrix as dirty.
     *
     * @param rot
     * Rotation delta to add.
     */
    void AddRotation(float rot)
    {
        rotation += rot;
        isChanged = true;
    }

    /**
     * @brief Sets the local 2D scale of the transform.
     *
     * @details
     * Replaces the current scale and marks the matrix as dirty.
     * The scale is later applied as the last multiplication step when rebuilding
     * the matrix.
     *
     * @param scl
     * New 2D scale.
     */
    void SetScale(const glm::vec2& scl)
    {
        scale = scl;
        isChanged = true;
    }

    /**
     * @brief Adds to the current scale.
     *
     * @details
     * Accumulates the given scale delta onto the current scale and marks the
     * matrix as dirty.
     *
     * @param scl
     * Scale delta to add.
     *
     * @note
     * This performs additive scaling, not multiplicative scaling.
     */
    void AddScale(const glm::vec2& scl)
    {
        scale += scl;
        isChanged = true;
    }

    /**
     * @brief Sets the render depth value.
     *
     * @details
     * Stores the depth used for render ordering.
     * This value does not mark the transform matrix as dirty because it is not
     * included in the matrix built by GetMatrix().
     *
     * In the current rendering flow, depth is read separately and quantized for
     * render-map sorting.
     *
     * @param dpth
     * New depth value.
     */
    void SetDepth(float dpth)
    {
        depth = dpth;
    }

    /**
     * @brief Sets an additional local translation offset.
     *
     * @details
     * Stores an extra translation that is applied inside the final transform
     * matrix after the main position translation and before rotation and scale.
     *
     * Based on the current GetMatrix() implementation, the matrix is built as:
     * position translation * offset translation * rotation * scale
     *
     * @param os
     * Local offset to store.
     *
     * @note
     * If this value changes, the matrix should also be considered dirty because
     * it affects the final matrix result.
     */
    void SetOffset(glm::vec2 os)
    {
        offset = os;
    }

    /**
     * @brief Returns the current position.
     *
     * @return
     * Constant reference to the stored 2D position.
     */
    [[nodiscard]] const glm::vec2& GetPosition() const { return position; }

    /**
     * @brief Returns the current rotation value.
     *
     * @return
     * Stored rotation angle.
     */
    [[nodiscard]] float GetRotation() const { return rotation; }

    /**
     * @brief Returns the current scale.
     *
     * @return
     * Constant reference to the stored 2D scale.
     */
    [[nodiscard]] const glm::vec2& GetScale() const { return scale; }

    /**
     * @brief Returns the current render depth.
     *
     * @return
     * Stored depth value used for draw ordering.
     */
    [[nodiscard]] float GetDepth() const { return depth; }

    /**
     * @brief Returns the current local offset.
     *
     * @return
     * Stored offset vector.
     */
    [[nodiscard]] glm::vec2 GetOffset() const { return offset; }

    /**
     * @brief Returns the cached transform matrix.
     *
     * @details
     * Rebuilds the matrix only when the transform has changed.
     * In the current implementation, the matrix is composed as:
     *
     * position translation * offset translation * rotation * scale
     *
     * After rebuilding, the dirty flag is cleared and the cached matrix is reused
     * until another transform-changing function marks it dirty again.
     *
     * @return
     * Reference to the internally cached 4x4 transform matrix.
     *
     * @note
     * The returned reference remains valid as long as this Transform2D instance exists.
     * The depth value is not included in this matrix.
     */
    [[nodiscard]] glm::mat4& GetMatrix();

private:
    glm::vec2 position;
    glm::vec2 offset;
    float depth;
    float rotation;
    glm::vec2 scale;
    glm::mat4 matrix;
    bool isChanged;
};