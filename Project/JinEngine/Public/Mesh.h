#pragma once
#include <vector>
#include "Material.h"

class ObjectManager;

using GLuint = unsigned int;
using GLsizei = int;

/**
 * @brief Specifies the OpenGL primitive topology used when drawing a mesh.
 *
 * @details
 * This enum is converted to the corresponding OpenGL draw mode inside the mesh
 * implementation. The selected value determines how the vertex/index data is
 * interpreted when Mesh::Draw() or Mesh::DrawInstanced() is executed.
 *
 * The mapping is handled internally by the renderer-side implementation and is
 * used by both indexed and non-indexed mesh draws.
 *
 * @note
 * The actual OpenGL enum conversion is performed in Mesh.cpp.
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
 * @brief Represents a single mesh vertex for the engine's default 2D/quad-style pipeline.
 *
 * @details
 * Each vertex currently stores:
 * - a 3D position used by the mesh buffer and shader input
 * - a 2D UV coordinate used for texture sampling
 *
 * Although this engine is primarily using the structure for 2D rendering at the
 * moment, the position is stored as glm::vec3, which makes the mesh layout
 * compatible with depth-aware rendering and future expansion.
 *
 * @note
 * The vertex layout is bound in Mesh::SetupMesh() and Mesh::SetupInstanceAttributes()
 * using attribute location 0 for position and attribute location 1 for UV. :contentReference[oaicite:3]{index=3}
 */
struct Vertex
{
    glm::vec3 position;
    glm::vec2 uv;
};

/**
 * @brief Owns GPU-side mesh buffers and provides draw operations for normal and instanced rendering.
 *
 * @details
 * Mesh is the engine's low-level geometry container. It creates and owns the
 * OpenGL objects required to render a piece of geometry:
 * - a standard VAO for regular drawing
 * - a VBO for vertex data
 * - an optional EBO for indexed rendering
 * - a dedicated instancing VAO and per-instance buffers for batched rendering
 *
 * During construction, the mesh uploads the provided vertex/index data to GPU
 * buffers and computes local bounds from the source vertices. Those bounds are
 * later used by Object::GetBoundingRadius() to estimate visibility and culling
 * size.
 *
 * Instanced rendering support is not automatically enabled just because this
 * class owns instance buffers. The mesh must first be prepared through
 * Material::EnableInstancing(), which internally calls SetupInstanceAttributes()
 * after verifying that the bound shader supports instancing attributes. :contentReference[oaicite:5]{index=5}
 *
 * At runtime:
 * - regular rendering uses Draw()
 * - batched rendering uploads per-instance data through UpdateInstanceBuffer()
 *   and then calls DrawInstanced()
 *
 * The RenderManager uses these paths when flushing draw commands. :contentReference[oaicite:6]{index=6}
 *
 * @note
 * This class is intentionally tightly integrated with Material and RenderManager.
 * Internal GPU setup and draw functions are kept private and are exposed only
 * through friendship.
 */
class Mesh {
    friend Material;
    friend RenderManager;

public:
    /**
     * @brief Creates a mesh from vertex data, optional index data, and a primitive type.
     *
     * @details
     * This constructor initializes all OpenGL object handles to safe default values,
     * stores the selected primitive topology, uploads the provided geometry to GPU
     * buffers by calling SetupMesh(), and computes the local bounding half-size by
     * calling ComputeLocalBounds().
     *
     * If the index array is empty, the mesh is treated as a non-indexed mesh and
     * will later be rendered with glDrawArrays-style calls. Otherwise, it will use
     * indexed drawing with glDrawElements-style calls. :contentReference[oaicite:7]{index=7}
     *
     * The computed local bound is later used indirectly for approximate culling and
     * bounding radius calculation through Object::GetBoundingRadius(). :contentReference[oaicite:8]{index=8}
     *
     * @param vertices Source vertex array to upload into the mesh vertex buffer.
     * @param indices Optional index array. If empty, the mesh will use non-indexed rendering.
     * @param primitiveType Primitive topology used when this mesh is drawn.
     *
     * @note
     * The constructor does not automatically configure instancing attributes.
     * Instancing setup happens later only if a material explicitly enables it.
     */
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices = {}, PrimitiveType primitiveType = PrimitiveType::Triangles);

    /**
     * @brief Releases all GPU resources owned by this mesh.
     *
     * @details
     * The destructor deletes:
     * - the element buffer
     * - the vertex buffer
     * - the regular VAO
     * - all per-instance buffers
     * - the instancing VAO
     *
     * After deletion, the internal OpenGL handles are reset to zero to avoid
     * accidental reuse of stale IDs in debug scenarios. :contentReference[oaicite:9]{index=9}
     *
     * @note
     * This assumes the OpenGL context is still valid at destruction time.
     */
    ~Mesh();

    /**
     * @brief Returns the mesh's local half-size computed from the original vertex positions.
     *
     * @details
     * This value is calculated in ComputeLocalBounds() from the minimum and maximum
     * vertex positions in local space. It represents half of the mesh's width and
     * height in local coordinates.
     *
     * The engine later uses this value to estimate an object's bounding radius by
     * multiplying it with the object's scale and taking the vector length. This is
     * used for view culling and related spatial checks.
     *
     * Special cases handled during bound computation:
     * - empty vertex list  -> defaults to 0.5f, 0.5f
     * - single vertex mesh -> uses a very small half-size to avoid a zero-area bound
     *
     * @return Local-space half-size of the mesh as a 2D vector.
     */
    [[nodiscard]] glm::vec2 GetLocalBoundsHalfSize() const { return localHalfSize; }

private:
    /**
     * @brief Binds the appropriate VAO for either regular or instanced rendering.
     *
     * @details
     * When instanced is false, this binds the standard VAO configured by SetupMesh().
     * When instanced is true, this binds the dedicated instancing VAO configured by
     * SetupInstanceAttributes().
     *
     * This function is used internally by Draw() and DrawInstanced() before issuing
     * the final OpenGL draw call. :contentReference[oaicite:11]{index=11}
     *
     * @param instanced True to bind the instancing VAO, false to bind the standard VAO.
     */
    void BindVAO(bool instanced) const;

    /**
     * @brief Creates and configures per-instance vertex attributes for instanced rendering.
     *
     * @details
     * This function prepares the mesh for batched instanced draws by creating:
     * - a dedicated instancing VAO
     * - four instance buffers used for per-instance data
     *
     * The instanced layout contains:
     * - location 2~5 : mat4 instance transform
     * - location 6   : vec4 instance color
     * - location 7   : vec2 instance UV offset
     * - location 8   : vec2 instance UV scale
     *
     * Divisors are configured so that these attributes advance once per instance
     * rather than once per vertex. If the mesh uses an index buffer, the same EBO
     * is also attached to the instancing VAO. :contentReference[oaicite:12]{index=12}
     *
     * This function is not part of the normal public API. It is typically called
     * indirectly through Material::EnableInstancing() after the material confirms
     * that its shader exposes the required instancing attributes. :contentReference[oaicite:13]{index=13}
     *
     * @note
     * Calling this repeatedly is safe with respect to object creation checks, since
     * the implementation only creates the VAO/VBO objects when they do not already exist.
     */
    void SetupInstanceAttributes();

    /**
     * @brief Draws the mesh once using its currently stored geometry buffers.
     *
     * @details
     * This function binds the regular VAO, converts the stored PrimitiveType into
     * an OpenGL draw mode, and issues a single non-instanced draw call.
     *
     * The actual draw path depends on whether index data was provided at construction:
     * - indexed mesh    -> glDrawElements
     * - non-indexed     -> glDrawArrays
     *
     * This function is used by the render manager for objects that are not currently
     * rendered through the instancing path.
     */
    void Draw() const;

    /**
     * @brief Draws the mesh multiple times in a single instanced draw call.
     *
     * @details
     * This function binds the instancing VAO, converts the stored PrimitiveType into
     * an OpenGL draw mode, and issues an instanced draw call using the current
     * instance buffers previously updated through UpdateInstanceBuffer().
     *
     * The actual draw path depends on whether the mesh uses indices:
     * - indexed mesh    -> glDrawElementsInstanced
     * - non-indexed     -> glDrawArraysInstanced
     *
     * This path is used by RenderManager when a batch of objects shares a compatible
     * mesh/material combination and the material supports instancing.
     *
     * @param instanceCount Number of instances to render in the batch.
     */
    void DrawInstanced(GLsizei instanceCount) const;

    /**
     * @brief Uploads vertex and optional index data and configures the base VAO layout.
     *
     * @details
     * This function is called during construction and performs the core GPU setup for
     * the mesh:
     * - determines whether indexed drawing will be used
     * - stores the effective draw count in indexCount
     * - creates the regular VAO
     * - creates and uploads the vertex buffer
     * - binds vertex attributes for position and UV
     * - optionally creates and attaches an element buffer
     *
     * Vertex attributes are configured as:
     * - location 0 : vec3 position
     * - location 1 : vec2 uv
     *
     * When indices are provided, indexCount stores the number of indices.
     * Otherwise, it stores the number of vertices, which becomes the draw count
     * for glDrawArrays-based rendering. :contentReference[oaicite:16]{index=16}
     *
     * @param vertices Vertex array to upload into the mesh VBO.
     * @param indices Optional index array to upload into the mesh EBO.
     */
    void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    /**
     * @brief Computes a simple local-space bounding half-size from the input vertices.
     *
     * @details
     * This helper scans the vertex positions, finds the local minimum and maximum
     * XY extents, computes the size difference, and stores half of that size in
     * localHalfSize.
     *
     * The value is later used by Object::GetBoundingRadius() as part of the
     * engine's approximate visibility and spatial checks.
     *
     * Special handling:
     * - if the mesh has no vertices, the bound defaults to (0.5, 0.5)
     * - if the mesh has exactly one vertex, a tiny non-zero bound is used
     *
     * @param vertices Source vertex array used to estimate the local bound.
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
     * @brief Uploads per-instance batch data into the mesh's instance buffers.
     *
     * @details
     * This function refreshes all instance buffers used by the instanced rendering path:
     * - transforms
     * - colors
     * - UV offsets
     * - UV scales
     *
     * The render manager calls this immediately before DrawInstanced() so that the
     * GPU-side instance data matches the current batch contents for the frame.
     *
     * All buffers are uploaded using GL_DYNAMIC_DRAW because the data changes per frame
     * or per batch.
     *
     * @param transforms Per-instance model matrices.
     * @param colors Per-instance tint colors.
     * @param uvOffsets Per-instance UV offsets, commonly used for sprite-sheet animation.
     * @param uvScales Per-instance UV scales, commonly used for sprite-sheet animation.
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