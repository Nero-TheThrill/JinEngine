#include <gl.h>

#include "Engine.h"

void Material::Bind() const
{
    shader->Use();
}

void Material::UnBind() const
{
    int unit = 0;
    for (const auto& [uniformName, tex] : textures)
    {
        if (!tex) continue;
        tex->UnBind(unit);
        unit++;
    }
    shader->Unuse();
}

void Material::SendData()
{
    SendTextures();
    SendUniforms();
}

bool Material::IsInstancingSupported() const
{
    return isInstancingEnabled && shader && shader->SupportsInstancing();
}
void Material::EnableInstancing(bool enable, std::shared_ptr<Mesh> mesh)
{
    if (!mesh)
    {
        JIN_WRN("Enable Instancing skipped: Mesh is nullptr");
        return;
    }
    if (!shader)
    {
        JIN_WRN("Enable Instancing skipped: Shader is nullptr");
        return;
    }
    if (!shader->SupportsInstancing())
    {
        JIN_WRN("Enable Instancing skipped: Tried enable instancing, but shader does not support 'i_Model'.");
        return;
    }

    isInstancingEnabled = enable;

    if (mesh && isInstancingEnabled)
    {
        mesh->SetupInstanceAttributes();
    }
}

void Material::SendUniforms()
{
    for (const auto& [name, value] : uniforms)
    {
        std::visit([&](auto&& val)
            {
                shader->SendUniform(name, val);
            }, value);
    }
}

void Material::SendTextures()
{
    int unit = 0;
    for (const auto& [uniformName, tex] : textures)
    {
        if (!tex) continue;
        tex->BindToUnit(unit);
        shader->SendUniform(uniformName, unit);
        unit++;
    }
}

bool Material::HasTexture(std::shared_ptr<Texture> texture) const
{
    for (const auto& pair : textures)
    {
        if (pair.second == texture)
        {
            return true;
        }
    }
    return false;
}

bool Material::HasShader(std::shared_ptr<Shader> shader_) const
{
    return shader == shader_;
}


void ComputeMaterial::SendData()
{
    int unit = 0;
    for (const auto& [uniformName, img] : images)
    {
        if (!img.tex) continue;
        img.tex->BindAsImage(unit, img.access, img.format, img.level);
        shader->SendUniform(uniformName, unit);
        unit++;
    }
    SendUniforms();
}
