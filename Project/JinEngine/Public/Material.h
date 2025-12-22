#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include "glm.hpp"
#include "Texture.h"

class RenderManager;
class ObjectManager;
class Shader;
class Texture;
class Mesh;

using GLuint = unsigned int;

/**
 * @brief Variant type used to store strongly-typed uniform values for a Material.
 *
 * @details
 * Material stores per-uniform data in a map of (uniform name -> UniformValue).
 * When binding, Material serializes these values and forwards them to Shader::SendUniform(...)
 * using the correct overload based on the active variant type.
 *
 * Supported types are limited to the most common scalar/vector/matrix uniform types used by the engine.
 */
using UniformValue = std::variant<
    int,
    float,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::mat4
>;

/**
 * @brief Render-time binding object that combines a Shader with textures and uniform parameters.
 *
 * @details
 * Material acts as a lightweight state bundle that RenderManager uses to drive drawing:
 * - shader: the GPU program used for the draw.
 * - textures: a mapping of uniform sampler names to Texture* pointers.
 * - uniforms: a mapping of uniform names to typed values (UniformValue).
 *
 * Ownership model:
 * - Material does not own Shader/Texture/Mesh. It only stores raw pointers that must remain valid.
 * - RenderManager typically owns the registered Shaders/Textures/Materials and ensures lifetime safety.
 *
 * Binding workflow:
 * - Bind() activates the Shader (Use) and sends all textures/uniforms required for the draw.
 * - UnBind() deactivates the Shader (Unuse) and unbinds texture units if needed.
 *
 * Instancing:
 * - IsInstancingSupported() checks whether the underlying Shader is compatible with the engine's instancing path.
 * - EnableInstancing(enable, mesh) toggles the instancing flag and may configure the Mesh-side instance attributes.
 *
 * Compute:
 * - IsCompute() identifies whether a Material is a compute dispatch material. Regular materials return false.
 * - ComputeMaterial overrides IsCompute() and SendData() and uses image bindings instead of sampler textures.
 */
class Material {
    friend RenderManager;

public:
    /**
     * @brief Constructs a material using a specific shader.
     *
     * @details
     * Instancing is disabled by default. Textures and uniforms can be assigned after construction.
     *
     * @param _shader Shader program to be used for this material.
     */
    Material(Shader* _shader) : shader(_shader), isInstancingEnabled(false) {}

    /**
     * @brief Virtual destructor for polymorphic cleanup.
     */
    virtual ~Material() = default;

    /**
     * @brief Indicates whether this material is intended for compute dispatch.
     *
     * @details
     * Regular materials return false. ComputeMaterial overrides this to return true.
     */
    virtual bool IsCompute() const { return false; }

    /**
     * @brief Binds a texture to a sampler uniform name.
     *
     * @details
     * Stores the mapping in the internal texture table. The texture is not bound immediately;
     * it is bound later during SendTextures() when RenderManager binds the material for a draw.
     *
     * If the same uniform name already exists, it is overwritten.
     *
     * @param uniformName Sampler uniform name expected by the shader (e.g., "u_Texture").
     * @param texture     Texture pointer to bind to the sampler.
     */
    void SetTexture(const std::string& uniformName, Texture* texture)
    {
        textures[uniformName] = texture;
    }

    /**
     * @brief Sets a typed uniform value to be sent during binding.
     *
     * @details
     * Stores (name -> value) in uniforms. Values are sent to the shader during Bind()/SendUniforms().
     * If the same uniform name already exists, it is overwritten.
     *
     * @param name  Uniform name expected by the shader.
     * @param value Typed uniform value (int/float/vec2/vec3/vec4/mat4).
     */
    void SetUniform(const std::string& name, UniformValue value)
    {
        uniforms[name] = value;
    }

    /**
     * @brief Returns whether instanced rendering is supported for this material.
     *
     * @details
     * This is typically determined by the underlying Shader's capabilities (e.g., presence of
     * required instancing attributes/uniform conventions). If unsupported, RenderManager should
     * fall back to per-object draws even if objects are batchable.
     *
     * @return True if instancing is supported; false otherwise.
     */
    [[nodiscard]] bool IsInstancingSupported() const;

    /**
     * @brief Enables or disables instancing for this material and prepares the mesh for instanced draws.
     *
     * @details
     * When enabling instancing, the engine may require Mesh-side instance attribute setup so that
     * per-instance data (model/color/uv) can be streamed and used by the shader.
     *
     * Disabling instancing only toggles the material flag; RenderManager will then use non-instanced draws.
     *
     * @param enable True to enable instancing; false to disable.
     * @param mesh   Mesh that will be used for instanced draws (used to set up instance attributes).
     */
    void EnableInstancing(bool enable, Mesh* mesh);

protected:
    /**
     * @brief Binds this material for rendering.
     *
     * @details
     * Expected responsibilities:
     * - Activates the shader program.
     * - Sends textures (SendTextures()).
     * - Sends uniform values (SendUniforms()).
     * - Calls SendData() for subclasses to push additional data.
     *
     * This is intended to be called by RenderManager immediately before drawing.
     */
    void Bind() const;

    /**
     * @brief Unbinds this material after rendering.
     *
     * @details
     * Expected responsibilities:
     * - Unbind textures if the engine requires explicit cleanup.
     * - Deactivates the shader program.
     *
     * This is intended to be called by RenderManager after drawing or when switching materials.
     */
    void UnBind() const;

    /**
     * @brief Sends material-specific data beyond the standard textures and uniforms.
     *
     * @details
     * Base Material implementation may be a no-op or may forward to SendUniforms/SendTextures.
     * ComputeMaterial overrides this to bind images and set up compute-specific state.
     */
    virtual void SendData();

    /**
     * @brief Sends all typed uniform values stored in the uniform table to the shader.
     *
     * @details
     * Iterates uniforms and uses the correct Shader::SendUniform overload based on the UniformValue variant type.
     * This is usually called during Bind().
     */
    void SendUniforms();

    Shader* shader;
    std::unordered_map<std::string, UniformValue> uniforms;
    bool isInstancingEnabled;

private:
    /**
     * @brief Binds all textures stored in the texture table to texture units and assigns sampler uniforms.
     *
     * @details
     * Iterates textures and:
     * - binds each Texture to a unit (Texture::BindToUnit)
     * - updates the corresponding sampler uniform to that unit
     *
     * The exact unit assignment strategy is implementation-defined.
     */
    void SendTextures();

    /**
     * @brief Returns whether this material has at least one texture binding.
     */
    [[nodiscard]] bool HasTexture() const { return !textures.empty(); }

    /**
     * @brief Checks whether the provided texture pointer is currently bound by this material.
     *
     * @param texture Texture pointer to test.
     * @return True if any binding references the texture.
     */
    [[nodiscard]] bool HasTexture(Texture* texture) const;

    /**
     * @brief Checks whether this material uses the given shader pointer.
     *
     * @param shader_ Shader pointer to test.
     * @return True if shader_ matches this material's shader.
     */
    [[nodiscard]] bool HasShader(Shader* shader_) const;

    /**
     * @brief Returns the shader used by this material.
     *
     * @details
     * This is a low-level helper for RenderManager to read the active shader during batching and binding.
     */
    [[nodiscard]] Shader* GetShader() const { return shader; }

    std::unordered_map<std::string, Texture*> textures;
};

/**
 * @brief Compute-only material that binds textures as images for compute shader dispatch.
 *
 * @details
 * ComputeMaterial specializes Material for compute workloads:
 * - IsCompute() returns true so RenderManager can route it through DispatchCompute().
 * - SetImage() binds textures as images (Texture::BindAsImage) instead of sampler textures.
 * - destinationTexture is tracked for dispatch dimension calculation in RenderManager (e.g., using output size).
 *
 * Restrictions:
 * - Instancing is not applicable for compute, so EnableInstancing is deleted.
 * - Sampler-based SetTexture is deleted to prevent accidental use; compute uses images instead.
 *
 * Convention:
 * - If SetImage() is called with uniformName == "u_Dst", destinationTexture is updated to that texture.
 *   RenderManager typically uses this to compute dispatch group counts.
 */
class ComputeMaterial : public Material
{
    /**
     * @brief Describes a single image binding for a compute shader.
     *
     * @details
     * This packs the texture pointer and the required OpenGL image binding parameters.
     */
    struct ImageBind
    {
        Texture* tex;
        ImageAccess access;
        ImageFormat format;
        int level;
    };
    Texture* destinationTexture;
    std::unordered_map<std::string, ImageBind> images;
public:
    /**
     * @brief Constructs a compute material using a specific compute shader.
     *
     * @param _shader Compute shader program to be used.
     */
    ComputeMaterial(Shader* _shader) : Material(_shader) {}

    /**
     * @brief Indicates that this material is a compute material.
     */
    [[nodiscard]] bool IsCompute() const override { return true; }

    /**
     * @brief Binds a texture as an image uniform for compute shaders.
     *
     * @details
     * Stores the binding in the internal image table. The actual OpenGL image binding is performed
     * during SendData() when the material is used for a dispatch.
     *
     * If uniformName is "u_Dst", the provided texture is recorded as destinationTexture.
     *
     * @param uniformName Image uniform name in the compute shader.
     * @param texture     Texture to bind as an image.
     * @param access      Image access mode (read/write).
     * @param format      Image format used for the binding.
     * @param level       Mipmap level to bind.
     */
    void SetImage(const std::string& uniformName, Texture* texture, ImageAccess access, ImageFormat format, int level)
    {
        ImageBind img = { texture,access,format,level };
        images[uniformName] = img;
        if (uniformName == "u_Dst")
            destinationTexture = texture;
    }

    /**
     * @brief Returns the destination texture recorded for dispatch sizing.
     *
     * @details
     * This is commonly set by calling SetImage("u_Dst", ...).
     *
     * @return Destination texture pointer (may be uninitialized if never set).
     */
    [[nodiscard]] Texture* GetDstTexture() const { return destinationTexture; }

    /**
     * @brief Instancing is not supported for compute materials.
     */
    void EnableInstancing(bool enable, Mesh* mesh) = delete;

    /**
     * @brief Sampler-based texture binding is not supported for compute materials.
     */
    void SetTexture(const std::string& uniformName, Texture* texture) = delete;

    /**
     * @brief Sends compute-specific bindings (images) to the GPU.
     *
     * @details
     * Overrides Material::SendData(). Expected responsibilities:
     * - Bind each ImageBind texture via Texture::BindAsImage(...) to the correct unit.
     * - Set any compute-related uniforms needed by the shader.
     *
     * RenderManager calls this when dispatching compute workloads with this material.
     */
    void SendData() override;
};
