#pragma once
#include <vector>
#include "Material.h"

class ObjectManager;

using GLuint = unsigned int;
using GLsizei = int;

/**
 * @brief Defines the primitive topology used when issuing draw calls for a Mesh.
 *
 * @details
 * PrimitiveType selects how the vertex/index data is interpreted by the GPU draw call.
 * RenderManager/Mesh map this enum to the corresponding OpenGL primitive (e.g., Triangles -> GL_TRIANGLES).
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
 * @brief Basic 2D vertex layout used by this engine's Mesh.
 *
 * @details
 * - position: vec3 where XY are used for 2D placement and Z may be used for depth or left as 0.
 * - uv: texture coordinates.
 */
struct Vertex
{
    glm::vec3 position;
    glm::vec2 uv;
};

/**
 * @brief GPU mesh wrapper that owns VAO/VBO/EBO and supports optional instanced rendering buffers.
 *
 * @details
 * Mesh encapsulates OpenGL buffer objects and draw routines. It supports:
 * - Non-indexed and indexed rendering (useIndex + ebo + indexCount).
 * - A primitive topology (PrimitiveType) used for draw calls.
 * - A local-space bounds half-size (localHalfSize) computed from vertex positions for culling/placement.
 * - Instanced rendering via a dedicated instance VAO and multiple instance VBOs:
 *   - transforms (mat4)
 *   - colors (vec4)
 *   - uvOffsets (vec2)
 *   - uvScales (vec2)
 *
 * Lifecycle and access:
 * - Mesh is constructed from CPU vertex/index arrays, uploads them to GPU, and computes local bounds.
 * - RenderManager and Material are friends to allow them to bind and draw meshes efficiently.
 */
class Mesh {
    friend Material;
    friend RenderManager;

public:
    /**
     * @brief Creates a mesh and uploads vertex/index data to the GPU.
     *
     * @details
     * The constructor:
     * - Stores primitiveType.
     * - Creates and configures VAO/VBO (and EBO if indices are provided).
     * - Computes local bounds from the provided vertices (ComputeLocalBounds).
     * - Prepares instance buffers/attributes so RenderManager can draw instanced batches.
     *
     * If indices is empty, the mesh is treated as non-indexed (useIndex = false).
     *
     * @param vertices      Vertex array to upload.
     * @param indices       Optional index array. If non-empty, indexed drawing is enabled.
     * @param primitiveType Primitive topology used for draw calls.
     */
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices = {}, PrimitiveType primitiveType = PrimitiveType::Triangles);

    /**
     * @brief Destroys GPU buffers owned by this mesh.
     *
     * @details
     * Deletes VAO/VBO/EBO and instance VAO/VBOs if they were created.
     * The mesh must not be in use by the GPU when destroyed (standard OpenGL lifetime rules).
     */
    ~Mesh();

    /**
     * @brief Returns half of the local-space axis-aligned bounds size.
     *
     * @details
     * This value is computed from vertex positions by ComputeLocalBounds().
     * It is typically used as an approximate local-space size for visibility culling and debug bounds.
     *
     * @return Local bounds half-size in mesh local space.
     */
    [[nodiscard]] glm::vec2 GetLocalBoundsHalfSize() const { return localHalfSize; }

private:
    /**
     * @brief Binds the appropriate VAO for drawing.
     *
     * @details
     * If instanced is true, binds instanceVAO (which includes base vertex attributes + instance attributes).
     * Otherwise binds the non-instanced VAO (vao).
     *
     * @param instanced Whether the draw call will be instanced.
     */
    void BindVAO(bool instanced) const;

    /**
     * @brief Configures vertex attribute state for instance data.
     *
     * @details
     * Creates instance VAO/VBOs and sets up attribute pointers/divisors for:
     * - mat4 transform (usually split across 4 vec4 attributes)
     * - vec4 color
     * - vec2 uv offset
     * - vec2 uv scale
     *
     * The exact attribute locations must match the shader convention used by this engine
     * (e.g., attributes named "i_Model", etc.).
     */
    void SetupInstanceAttributes();

    /**
     * @brief Issues a non-instanced draw call using this mesh's topology.
     *
     * @details
     * Uses either glDrawElements (if useIndex is true) or glDrawArrays (if useIndex is false),
     * mapping primitiveType to the appropriate OpenGL primitive.
     */
    void Draw() const;

    /**
     * @brief Issues an instanced draw call using this mesh's topology.
     *
     * @details
     * Uses either glDrawElementsInstanced (if useIndex is true) or glDrawArraysInstanced (if useIndex is false).
     * Instance attribute buffers must have been updated via UpdateInstanceBuffer() prior to calling.
     *
     * @param instanceCount Number of instances to draw.
     */
    void DrawInstanced(GLsizei instanceCount) const;

    /**
     * @brief Uploads vertex/index data and configures base vertex attributes.
     *
     * @details
     * Creates and binds vao/vbo and uploads the vertex array.
     * If indices is non-empty, creates/binds ebo and uploads indices, enabling indexed rendering and setting indexCount.
     *
     * Also configures vertex attribute pointers for the Vertex layout (position + uv).
     *
     * @param vertices Vertex array to upload.
     * @param indices  Index array to upload (may be empty).
     */
    void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    /**
     * @brief Computes an approximate local-space AABB half-size from vertex positions.
     *
     * @details
     * Behavior:
     * - If vertices is empty: localHalfSize becomes (0.5, 0.5) as a conservative default.
     * - If vertices has one element: localHalfSize becomes (0.0001, 0.0001) to avoid a zero-size bound.
     * - Otherwise: computes min/max over XY from all vertices and sets localHalfSize = (max-min)*0.5.
     *
     * This function assumes mesh geometry is primarily 2D (XY plane), even though Vertex::position is vec3.
     *
     * @param vertices Vertex array used to compute bounds.
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
     * @brief Updates GPU instance buffers used for instanced drawing.
     *
     * @details
     * Uploads per-instance arrays into instanceVBO buffers so DrawInstanced() can be executed.
     * All arrays are expected to have the same length and match instanceCount used in DrawInstanced().
     *
     * Typical contents:
     * - transforms: model matrices for each instance.
     * - colors: per-instance color multiplier.
     * - uvOffsets / uvScales: per-instance sprite sheet UV transform.
     *
     * @param transforms Per-instance model matrices.
     * @param colors     Per-instance colors.
     * @param uvOffsets  Per-instance UV offsets.
     * @param uvScales   Per-instance UV scales.
     */
    void UpdateInstanceBuffer(const std::vector<glm::mat4>& transforms, const std::vector<glm::vec4>& colors, const std::vector<glm::vec2>& uvOffsets, const std::vector<glm::vec2>& uvScales) const;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei indexCount;

    GLuint instanceVAO;
    GLuint instanceVBO[4];

    bool useIndex;

    PrimitiveType primitiveType;
    glm::vec2 localHalfSize;
};
