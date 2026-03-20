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
using UniformValue = std::variant<
    int,
    float,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::mat4
>;

class Material {
    friend RenderManager;

public:
    Material(std::shared_ptr<Shader> _shader) : shader(std::move(_shader)), isInstancingEnabled(false){}
    virtual ~Material() = default;
    virtual bool IsCompute() const { return false; }

    void SetTexture(const std::string& uniformName, std::shared_ptr<Texture> texture)
    {
        textures[uniformName] = texture;
    }

    void SetUniform(const std::string& name, UniformValue value)
    {
        uniforms[name] = value;
    }

    [[nodiscard]] bool IsInstancingSupported() const;

    void EnableInstancing(bool enable, std::shared_ptr<Mesh> mesh);

protected:
    void Bind() const;

    void UnBind() const;

    virtual void SendData();

    void SendUniforms();

    std::shared_ptr<Shader> shader;
    std::unordered_map<std::string, UniformValue> uniforms;
    bool isInstancingEnabled;

private:
    void SendTextures();

    [[nodiscard]] bool HasTexture() const { return !textures.empty(); }

    [[nodiscard]] bool HasTexture(std::shared_ptr<Texture> texture) const;

    [[nodiscard]] bool HasShader(std::shared_ptr<Shader> shader_) const;

    [[nodiscard]] std::shared_ptr<Shader> GetShader() const { return shader; }


    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
};

class ComputeMaterial : public Material
{
    struct ImageBind
    {
        std::shared_ptr<Texture> tex;
        ImageAccess access;
        ImageFormat format;
        int level;
    };
    std::shared_ptr <Texture> destinationTexture;
    std::unordered_map<std::string, ImageBind> images;
public:
    ComputeMaterial(std::shared_ptr<Shader> _shader) : Material(_shader) {}
    [[nodiscard]] bool IsCompute() const override { return true; }
    void SetImage(const std::string& uniformName, std::shared_ptr<Texture> texture, ImageAccess access, ImageFormat format, int level)
    {
        ImageBind img = { texture,access,format,level};
        images[uniformName] = img;
        if (uniformName == "u_Dst")
            destinationTexture = texture;
    }
    [[nodiscard]] std::shared_ptr<Texture> GetDstTexture() const { return destinationTexture; }
    void EnableInstancing(bool enable, std::shared_ptr<Mesh> mesh) = delete;
    void SetTexture(const std::string& uniformName, std::shared_ptr<Texture> texture) = delete;
    void SendData() override;
};