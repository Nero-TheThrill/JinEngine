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
 * @brief Variant type for uniform values supported by Material.
 *
 * @details
 * Allowed types: int, float, vec2, vec3, vec4, mat4.
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
 * @brief High-level wrapper that binds a shader with uniforms and textures.
 *
 * @details
 * Material manages a Shader, a set of typed uniforms, and a set of named
 * texture bindings. It also exposes optional GPU instancing toggles if the
 * underlying Shader/Mesh pair supports it.
 *
 * Typical usage:
 * @code
 * Material* mat = rm->GetMaterialByTag("[Mat]sprite");
 * mat->SetUniform("u_Tint", glm::vec4(1,1,1,1));
 * mat->SetTexture("u_MainTex", rm->GetTextureByTag("[Tex]player"));
 * @endcode
 */
class Material {
    friend RenderManager;

public:
    /**
     * @brief Constructs a material bound to a shader.
     *
     * @param _shader Non-owning pointer to a compiled & linked shader program.
     */
    Material(Shader* _shader) : shader(_shader), isInstancingEnabled(false) {}

    /**
     * @brief Virtual destructor.
     */
    virtual ~Material() = default;

    /**
     * @brief Indicates whether this material represents a compute pipeline.
     *
     * @return false for base Material; overridden by ComputeMaterial.
     */
    virtual bool IsCompute() const { return false; }

    /**
     * @brief Binds a texture to a named sampler uniform.
     *
     * @param uniformName Name of sampler uniform in the shader (e.g., "u_MainTex").
     * @param texture Texture to bind (non-owning).
     */
    void SetTexture(const std::string& uniformName, Texture* texture)
    {
        textures[uniformName] = texture;
    }

    /**
     * @brief Sets (or overrides) a uniform value.
     *
     * @details
     * The value's type must match the uniform declared in the shader.
     *
     * @param name Uniform variable name.
     * @param value Variant value (int / float / vec* / mat4).
     */
     void SetUniform(const std::string & name, UniformValue value)
     {
         uniforms[name] = value;
     }

     /**
      * @brief Whether the underlying shader/mesh combination supports instancing.
      *
      * @details
      * Checks the shader's instancing attributes and mesh layout.
      *
      * @return true if instancing can be used; false otherwise.
      */
     [[nodiscard]] bool IsInstancingSupported() const;

     /**
      * @brief Enables or disables GPU instancing for this material.
      *
      * @details
      * A mesh pointer is provided so the material can configure instance
      * attributes. If the shader or mesh doesn't support instancing,
      * this call has no effect.
      *
      * @param enable Toggle instancing on/off.
      * @param mesh Mesh whose instance attributes will be used.
      */
     void EnableInstancing(bool enable, Mesh* mesh);

protected:
    /**
     * @brief Activates the shader program and common draw state.
     *
     * @details
     * Called by RenderManager before issuing draw calls.
     */
    void Bind() const;

    /**
     * @brief Deactivates the shader program and resets state if needed.
     */
    void UnBind() const;

    /**
     * @brief Sends material state to the GPU (uniforms, textures).
     *
     * @details
     * The base implementation uploads all registered uniforms and textures.
     * Derived classes may extend this to push additional state.
     */
    virtual void SendData();

    /**
     * @brief Uploads all stored uniform values.
     *
     * @details
     * Dispatches by variant type and calls the appropriate Shader::SendUniform().
     */
    void SendUniforms();

    Shader* shader;                                        ///< Non-owning shader pointer.
    std::unordered_map<std::string, UniformValue> uniforms;///< Name->uniform value table.
    bool isInstancingEnabled;                              ///< Instancing flag (runtime).

private:
    /**
     * @brief Uploads all bound textures to their sampler uniforms.
     *
     * @details
     * Assigns them to consecutive texture units and updates sampler uniforms.
     */
    void SendTextures();

    /**
     * @brief True if at least one texture is bound.
     */
    [[nodiscard]] bool HasTexture() const { return !textures.empty(); }

    /**
     * @brief Checks whether a specific texture is bound on any sampler.
     *
     * @param texture Texture pointer to search for.
     * @return true if the texture is already bound by this material.
     */
    [[nodiscard]] bool HasTexture(Texture* texture) const;

    /**
     * @brief Checks whether this material uses a specific shader.
     *
     * @param shader_ Shader to compare.
     * @return true if this->shader == shader_.
     */
    [[nodiscard]] bool HasShader(Shader* shader_) const;

    /**
     * @brief Returns the underlying shader pointer.
     */
    [[nodiscard]] Shader* GetShader() const { return shader; }

    std::unordered_map<std::string, Texture*> textures;    ///< Name->texture bindings (non-owning).
};

/**
 * @brief Material subtype for compute shaders (image load/store).
 *
 * @details
 * ComputeMaterial adds image bindings (uimage* / image*) on top of base uniforms.
 * It cannot use instancing or regular sampler textures via SetTexture().
 * A destination image(e.g., "u_Dst") can be tracked for post - dispatch use.
 */
class ComputeMaterial : public Material
 {
     /**
      * @brief Internal structure for image bindings.
      */
     struct ImageBind
     {
         Texture* tex;      ///< Image texture (non-owning).
         ImageAccess access;///< Access mode (read/write).
         ImageFormat format;///< Internal format.
         int level;         ///< Mipmap level.
     };

     Texture* destinationTexture; ///< Optional: tracked destination ("u_Dst").
     std::unordered_map<std::string, ImageBind> images; ///< Name->image binding table.

 public:
     /**
      * @brief Constructs a compute material with a compute-capable shader.
      *
      * @param _shader Non-owning pointer to a compiled compute shader program.
      */
     ComputeMaterial(Shader* _shader) : Material(_shader) {}

     /**
      * @brief Identifies this material as compute.
      *
      * @return Always true.
      */
     [[nodiscard]] bool IsCompute() const override { return true; }

     /**
      * @brief Binds a texture as an image for load/store operations.
      *
      * @param uniformName Name of the image uniform in the shader (e.g., "u_Src", "u_Dst").
      * @param texture Texture to bind as image (non-owning).
      * @param access Image access (read-only / write-only / read-write).
      * @param format Image format (e.g., RGBA8, R32F).
      * @param level Mipmap level to bind.
      *
      * @note
      * If @p uniformName equals "u_Dst", the texture is tracked as the destination.
      */
     void SetImage(const std::string& uniformName, Texture* texture, ImageAccess access, ImageFormat format, int level)
     {
         ImageBind img = { texture,access,format,level };
         images[uniformName] = img;
         if (uniformName == "u_Dst")
             destinationTexture = texture;
     }

     /**
      * @brief Returns the tracked destination texture ("u_Dst"), if set.
      *
      * @return Destination texture pointer or nullptr.
      */
     [[nodiscard]] Texture* GetDstTexture() const { return destinationTexture; }

     /**
      * @brief Instancing is not applicable to compute materials.
      */
     void EnableInstancing(bool enable, Mesh* mesh) = delete;

     /**
      * @brief Regular sampler textures are not supported for compute materials.
      */
     void SetTexture(const std::string& uniformName, Texture* texture) = delete;

     /**
      * @brief Sends uniforms and image bindings needed for a compute dispatch.
      *
      * @details
      * Overridden to upload image units (via Texture::BindAsImage) in addition
      * to base uniform uploads. Called by RenderManager before DispatchCompute().
      */
     void SendData() override;
 };
