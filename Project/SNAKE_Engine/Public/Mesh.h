#pragma once
#include <vector>
#include "Material.h"

class ObjectManager;

using GLuint = unsigned int;
using GLsizei = int;

/**
 * @brief Primitive topology used by the mesh draw calls.
 *
 * @details
 * This is mapped to the corresponding OpenGL primitive modes
 * (e.g., Triangles -> GL_TRIANGLES, Lines -> GL_LINES, etc.).
 */
enum class PrimitiveType
{
    Triangles,
    Lines,
    Points,
    TriangleFan,
    TriangleStrip,
    LineStrip
};

/**
 * @brief Vertex format used by Mesh.
 *
 * @details
 * Contains a 3D position and a 2D UV coordinate.
 */
struct Vertex
{
    glm::vec3 position; ///< Object-space position.
    glm::vec2 uv;       ///< Texture coordinates.
};

/**
 * @brief GPU-backed draw-able geometry (VAO/VBO/EBO) with optional instancing.
 *
 * @details
 * Mesh uploads vertex/index data to GPU buffers and exposes non-instanced and
 * instanced draw paths. It also computes a conservative local half-size used
 * for culling or bounds estimates.
 *
 * The instancing path expects per-instance: model matrix, color, UV offset and UV scale.
 */
class Mesh {
    friend Material;
    friend RenderManager;

public:
    /**
     * @brief Creates a mesh from vertex and optional index data.
     *
     * @param vertices The vertex array (position + UV).
     * @param indices Optional index array; if empty, the mesh is drawn non-indexed.
     * @param primitiveType Primitive topology (default = Triangles).
     * @note
     * Uploads buffers, configures VAO, and computes local bounds.
     */
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices = {}, PrimitiveType primitiveType = PrimitiveType::Triangles);

    /**
     * @brief Destroys GPU resources (VAO/VBO/EBO and instance buffers).
     */
    ~Mesh();

    /**
     * @brief Returns half of the local-space AABB size (X/Y).
     *
     * @details
     * Computed from vertex positions on construction.
     *
     * @return Half-size of local bounds.
     */
    [[nodiscard]] glm::vec2 GetLocalBoundsHalfSize() const { return localHalfSize; }

private:
    /**
     * @brief Binds the appropriate VAO for drawing.
     *
     * @param instanced True if instance attributes are needed.
     */
    void BindVAO(bool instanced) const;

    /**
     * @brief Defines per-instance vertex attributes (mat4, color, uv offset/scale).
     *
     * @details
     * Called during initialization to set divisors/attribute pointers
     * into @ref instanceVBO streams.
     */
    void SetupInstanceAttributes();

    /**
     * @brief Issues a non-instanced draw call.
     *
     * @details
     * Uses indexed draw if @ref useIndex is true; otherwise, array draw.
     */
    void Draw() const;

    /**
     * @brief Issues an instanced draw call.
     *
     * @param instanceCount Number of instances to draw.
     * @details
     * Requires @ref UpdateInstanceBuffer to have uploaded the per-instance data.
     */
    void DrawInstanced(GLsizei instanceCount) const;

    /**
     * @brief Uploads vertex/index data and configures VAO/VBO/EBO.
     *
     * @param vertices Vertex array.
     * @param indices Index array (optional).
     * @note
     * Also computes @ref localHalfSize via @ref ComputeLocalBounds.
     */
    void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    /**
     * @brief Computes local-space half-size from vertex positions.
     *
     * @details
     * For empty input, defaults to {0.5, 0.5}; for a single vertex, a tiny size.
     * Otherwise, builds an AABB over positions and takes half of its extent.
     *
     * @param vertices Vertex array used for bounds.
     */
    void ComputeLocalBounds(const std::vector<Vertex>& vertices)
    {
        if (vertices.empty())
        {
            localHalfSize = glm::vec2(0.5f);
            return;
        }
        else if (vertices.size() == 1)
        {
            localHalfSize = glm::vec2(0.0001f);
            return;
        }

        glm::vec2 minPos = vertices[0].position;
        glm::vec2 maxPos = vertices[1].position;

        for (const auto& v : vertices)
        {
            glm::vec2 pos = v.position;
            minPos = glm::min(minPos, pos);
            maxPos = glm::max(maxPos, pos);
        }

        glm::vec2 size = maxPos - minPos;
        localHalfSize = size * 0.5f;
    }

    /**
     * @brief Updates per-instance buffers (model, color, UV offset/scale).
     *
     * @param transforms Array of model matrices (one per instance).
     * @param colors Array of per-instance colors.
     * @param uvOffsets Array of per-instance UV offsets.
     * @param uvScales Array of per-instance UV scales.
     * @note
     * All arrays must have the same length (instanceCount).
     */
    void UpdateInstanceBuffer(const std::vector<glm::mat4>& transforms, const std::vector<glm::vec4>& colors, const std::vector<glm::vec2>& uvOffsets, const std::vector<glm::vec2>& uvScales) const;

    GLuint vao;                 ///< Vertex array object for non-instanced draws.
    GLuint vbo;                 ///< Vertex buffer object (positions + UVs).
    GLuint ebo;                 ///< Optional index buffer object.
    GLsizei indexCount;         ///< Number of indices (0 for non-indexed).

    GLuint instanceVAO;         ///< Vertex array configured for instanced draws.
    GLuint instanceVBO[4];      ///< Streams: [0]=mat4, [1]=color, [2]=uv offset, [3]=uv scale.

    bool useIndex;              ///< True if EBO is used.

    PrimitiveType primitiveType;///< Primitive topology used for draw calls.
    glm::vec2 localHalfSize;    ///< Precomputed half-size of local AABB (XY).
};
