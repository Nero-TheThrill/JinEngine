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
 * @brief Variant type used to store material uniform values in a type-safe way.
 *
 * @details
 * UniformValue represents the set of uniform data types currently supported by Material.
 * The current implementation supports:
 * - int
 * - float
 * - glm::vec2
 * - glm::vec3
 * - glm::vec4
 * - glm::mat4
 *
 * Material::SendUniforms() visits this variant and forwards each stored value to
 * Shader::SendUniform() using the matching overload.
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
 * @brief Rendering material that stores a shader, uniform values, and texture bindings.
 *
 * @details
 * Material represents the CPU-side binding state required before drawing a mesh.
 * It owns:
 * - a shared Shader
 * - a set of named uniform values
 * - a set of named texture bindings
 * - an instancing enabled flag
 *
 * During rendering, RenderManager binds the material, updates view/projection/model and other
 * runtime uniforms, calls SendData(), and then issues the draw call.
 *
 * The current implementation works as follows:
 * - Bind() activates the shader.
 * - SendData() sends bound textures first, then sends stored uniform values.
 * - UnBind() unbinds all textures that were bound by this material and then disables the shader. :contentReference[oaicite:3]{index=3}
 *
 * Material also supports instancing when:
 * - instancing has been explicitly enabled on the material
 * - the shader reports support for instancing
 *
 * The shader support check is ultimately based on whether the linked shader contains
 * an attribute named "i_Model".
 *
 * @note
 * This class stores rendering state only. It does not perform draw calls directly.
 *
 * @note
 * The texture and uniform maps are keyed by uniform name strings, so the caller must
 * ensure the names match the shader source.
 */
class Material {
    friend RenderManager;

public:
    /**
     * @brief Constructs a material bound to a shader.
     *
     * @param _shader Shared shader used by this material.
     *
     * @details
     * Instancing is disabled by default.
     */
    Material(std::shared_ptr<Shader> _shader) : shader(std::move(_shader)), isInstancingEnabled(false) {}

    /**
     * @brief Virtual destructor.
     */
    virtual ~Material() = default;

    /**
     * @brief Returns whether this material is intended for compute dispatch.
     *
     * @details
     * The base Material always returns false.
     * ComputeMaterial overrides this to return true.
     *
     * @return False for regular rendering materials.
     */
    virtual bool IsCompute() const { return false; }

    /**
     * @brief Binds a texture to a named sampler uniform entry.
     *
     * @details
     * The binding is stored in the internal texture map and later applied by SendTextures().
     * During SendTextures(), textures are assigned consecutive texture units in iteration order,
     * bound through Texture::BindToUnit(), and the matching sampler uniform is updated. :contentReference[oaicite:5]{index=5}
     *
     * If the same uniform name is assigned again, the previous binding is replaced.
     *
     * @param uniformName Name of the sampler uniform in the shader.
     * @param texture Texture resource to bind.
     */
    void SetTexture(const std::string& uniformName, std::shared_ptr<Texture> texture)
    {
        textures[uniformName] = texture;
    }

    /**
     * @brief Stores or updates a named uniform value.
     *
     * @details
     * The value is cached in the material and later sent to the shader by SendUniforms().
     * If the same uniform name already exists, the previous value is replaced.
     *
     * @param name Uniform name as declared in the shader.
     * @param value Value to store.
     */
    void SetUniform(const std::string& name, UniformValue value)
    {
        uniforms[name] = value;
    }

    /**
     * @brief Returns whether this material can currently be used for instanced rendering.
     *
     * @details
     * The current implementation returns true only when:
     * - instancing has been enabled on this material
     * - a shader exists
     * - the shader reports instancing support
     *
     * Shader support is determined after linking by checking whether the attribute
     * "i_Model" exists.
     *
     * @return True if instanced rendering is supported for this material in its current state.
     */
    [[nodiscard]] bool IsInstancingSupported() const;

    /**
     * @brief Enables or disables instanced rendering support for this material.
     *
     * @details
     * When enabling instancing, the current implementation validates:
     * - mesh is not nullptr
     * - shader is not nullptr
     * - shader supports instancing
     *
     * If all checks pass and enabling is requested, the material marks instancing as enabled
     * and asks the mesh to prepare instance attributes by calling Mesh::SetupInstanceAttributes().
     *
     * If the shader does not support the required "i_Model" attribute, the request is ignored
     * and a warning is logged.
     *
     * @param enable True to enable instancing, false to disable it.
     * @param mesh Mesh whose instance vertex layout should be prepared.
     *
     * @note
     * Disabling instancing only updates the material flag. It does not remove instance buffers
     * already created on the mesh.
     */
    void EnableInstancing(bool enable, std::shared_ptr<Mesh> mesh);

protected:
    /**
     * @brief Activates the shader associated with this material.
     *
     * @details
     * The current implementation forwards directly to Shader::Use().
     */
    void Bind() const;

    /**
     * @brief Unbinds textures previously bound by this material and then disables the shader.
     *
     * @details
     * The current implementation iterates over all stored texture bindings,
     * unbinds each texture unit in the same sequential order used by SendTextures(),
     * and then calls Shader::Unuse(). :contentReference[oaicite:10]{index=10}
     */
    void UnBind() const;

    /**
     * @brief Sends all material data required before drawing.
     *
     * @details
     * The base implementation sends textures first and then sends stored uniform values.
     * This is the standard CPU-to-GPU submission path used by regular draw materials. :contentReference[oaicite:11]{index=11}
     */
    virtual void SendData();

    /**
     * @brief Sends all cached uniform values to the shader.
     *
     * @details
     * The current implementation iterates over the internal uniform map and visits each
     * UniformValue to dispatch the correct Shader::SendUniform() overload.
     */
    void SendUniforms();

    /**
     * @brief Shader used by this material.
     */
    std::shared_ptr<Shader> shader;

    /**
     * @brief Cached uniform values indexed by shader uniform name.
     */
    std::unordered_map<std::string, UniformValue> uniforms;

    /**
     * @brief Whether instancing has been explicitly enabled for this material.
     */
    bool isInstancingEnabled;

private:
    /**
     * @brief Sends all stored texture bindings to the GPU.
     *
     * @details
     * The current implementation assigns texture units starting at zero,
     * binds each texture through Texture::BindToUnit(unit),
     * and updates the corresponding sampler uniform with that unit index. :contentReference[oaicite:13]{index=13}
     */
    void SendTextures();

    /**
     * @brief Returns whether this material currently has at least one bound texture entry.
     *
     * @return True if the internal texture map is not empty.
     */
    [[nodiscard]] bool HasTexture() const { return !textures.empty(); }

    /**
     * @brief Returns whether the specified texture is already referenced by this material.
     *
     * @param texture Texture to search for.
     * @return True if the texture pointer is present in any stored binding.
     */
    [[nodiscard]] bool HasTexture(std::shared_ptr<Texture> texture) const;

    /**
     * @brief Returns whether this material uses the specified shader.
     *
     * @param shader_ Shader to compare against.
     * @return True if the stored shader pointer matches.
     */
    [[nodiscard]] bool HasShader(std::shared_ptr<Shader> shader_) const;

    /**
     * @brief Returns the shader associated with this material.
     *
     * @return Shared pointer to the shader.
     */
    [[nodiscard]] std::shared_ptr<Shader> GetShader() const { return shader; }

    /**
     * @brief Texture bindings indexed by sampler uniform name.
     */
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
};

/**
 * @brief Specialized material type used for compute shader dispatch.
 *
 * @details
 * ComputeMaterial extends Material to support image bindings used by compute shaders.
 * Instead of relying on ordinary sampler texture bindings for output images,
 * it stores named image units with access qualifiers, image formats, and mip levels.
 *
 * The current implementation:
 * - overrides IsCompute() to return true
 * - deletes SetTexture() because regular sampler bindings are not the intended path here
 * - deletes EnableInstancing() because compute materials are never used for instanced mesh drawing
 * - overrides SendData() to bind images through Texture::BindAsImage() and then send uniforms
 *
 * RenderManager::DispatchCompute() expects a ComputeMaterial with a valid destination texture
 * and uses GetDstTexture() to derive the dispatch group count. :contentReference[oaicite:15]{index=15}
 */
class ComputeMaterial : public Material
{
    /**
     * @brief Describes one image binding used by a compute shader.
     *
     * @details
     * Each entry stores:
     * - the texture resource
     * - the image access mode
     * - the image format
     * - the mip level to bind
     */
    struct ImageBind
    {
        /**
         * @brief Texture bound as an image.
         */
        std::shared_ptr<Texture> tex;

        /**
         * @brief Access mode used when binding the image.
         */
        ImageAccess access;

        /**
         * @brief Image format declared for the binding.
         */
        ImageFormat format;

        /**
         * @brief Mip level used for image access.
         */
        int level;
    };

    /**
     * @brief Cached pointer to the destination texture used for compute output.
     *
     * @details
     * This is set automatically when SetImage() is called with the uniform name "u_Dst".
     */
    std::shared_ptr <Texture> destinationTexture;

    /**
     * @brief Image bindings indexed by image uniform name.
     */
    std::unordered_map<std::string, ImageBind> images;

public:
    /**
     * @brief Constructs a compute material bound to a shader.
     *
     * @param _shader Shared compute shader used by this material.
     */
    ComputeMaterial(std::shared_ptr<Shader> _shader) : Material(_shader) {}

    /**
     * @brief Returns whether this material is a compute material.
     *
     * @return Always true for ComputeMaterial.
     */
    [[nodiscard]] bool IsCompute() const override { return true; }

    /**
     * @brief Binds an image resource to a named compute shader image uniform entry.
     *
     * @details
     * The binding is stored internally and later applied by ComputeMaterial::SendData()
     * through Texture::BindAsImage().
     *
     * If the uniform name is "u_Dst", the texture is additionally stored as the destination
     * texture used by RenderManager::DispatchCompute() to determine dispatch dimensions.
     *
     * Reassigning the same uniform name replaces the previous binding.
     *
     * @param uniformName Name of the image uniform in the compute shader.
     * @param texture Texture resource to bind.
     * @param access Image access mode.
     * @param format Image format.
     * @param level Mip level to bind.
     */
    void SetImage(const std::string& uniformName, std::shared_ptr<Texture> texture, ImageAccess access, ImageFormat format, int level)
    {
        ImageBind img = { texture,access,format,level };
        images[uniformName] = img;
        if (uniformName == "u_Dst")
            destinationTexture = texture;
    }

    /**
     * @brief Returns the texture currently designated as the compute destination.
     *
     * @details
     * RenderManager::DispatchCompute() uses this texture to determine the width and height
     * for workgroup dispatch. :contentReference[oaicite:18]{index=18}
     *
     * @return Shared pointer to the destination texture.
     */
    [[nodiscard]] std::shared_ptr<Texture> GetDstTexture() const { return destinationTexture; }

    /**
     * @brief Deleted because compute materials are not used for instanced mesh drawing.
     */
    void EnableInstancing(bool enable, std::shared_ptr<Mesh> mesh) = delete;

    /**
     * @brief Deleted because compute materials use image bindings rather than ordinary texture sampler bindings.
     */
    void SetTexture(const std::string& uniformName, std::shared_ptr<Texture> texture) = delete;

    /**
     * @brief Sends image bindings and uniform values required for compute dispatch.
     *
     * @details
     * The current implementation binds each stored image to a consecutive image unit
     * through Texture::BindAsImage(), updates the corresponding uniform with that unit index,
     * and then sends the cached uniform values. :contentReference[oaicite:19]{index=19}
     */
    void SendData() override;
};