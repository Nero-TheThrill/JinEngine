#include "Engine.h"
#include "gl.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//used anonymous namespace to hide these functions from other files

static GLint ConvertMinFilter(TextureMinFilter filter)
{
    switch (filter)
    {
    case TextureMinFilter::Nearest:              return GL_NEAREST;
    case TextureMinFilter::Linear:               return GL_LINEAR;
    case TextureMinFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
    case TextureMinFilter::LinearMipmapNearest:  return GL_LINEAR_MIPMAP_NEAREST;
    case TextureMinFilter::NearestMipmapLinear:  return GL_NEAREST_MIPMAP_LINEAR;
    case TextureMinFilter::LinearMipmapLinear:   return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}

static GLint ConvertMagFilter(TextureMagFilter filter)
{
    switch (filter)
    {
    case TextureMagFilter::Nearest:              return GL_NEAREST;
    case TextureMagFilter::Linear:               return GL_LINEAR;
    }
    return GL_LINEAR;
}

static GLint ConvertWrap(TextureWrap wrap)
{
    switch (wrap)
    {
    case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
    case TextureWrap::Repeat:         return GL_REPEAT;
    case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
    case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
    }
    return GL_CLAMP_TO_EDGE;
}
inline GLenum ConvertAccess(ImageAccess access)
{
    switch (access)
    {
    case ImageAccess::ReadOnly:  return GL_READ_ONLY;
    case ImageAccess::WriteOnly: return GL_WRITE_ONLY;
    case ImageAccess::ReadWrite: return GL_READ_WRITE;
    }
    return GL_READ_ONLY;
}

inline GLenum ConvertFormat(ImageFormat fmt)
{
    switch (fmt)
    {
    case ImageFormat::R8:       return GL_R8;
    case ImageFormat::RG8:      return GL_RG8;
    case ImageFormat::RGBA8:    return GL_RGBA8;

    case ImageFormat::R16F:     return GL_R16F;
    case ImageFormat::RG16F:    return GL_RG16F;
    case ImageFormat::RGBA16F:  return GL_RGBA16F;

    case ImageFormat::R32F:     return GL_R32F;
    case ImageFormat::RG32F:    return GL_RG32F;
    case ImageFormat::RGBA32F:  return GL_RGBA32F;

    case ImageFormat::R8UI:     return GL_R8UI;
    case ImageFormat::RG8UI:    return GL_RG8UI;
    case ImageFormat::RGBA8UI:  return GL_RGBA8UI;

    case ImageFormat::R32UI:    return GL_R32UI;
    case ImageFormat::RG32UI:   return GL_RG32UI;
    case ImageFormat::RGBA32UI: return GL_RGBA32UI;
    }
    return GL_RGBA8;
}

Texture::Texture(const std::string& path, const TextureSettings& settings) :id(0), width(0), height(0), channels(0)
{
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data)
    {
        JIN_ERR("Failed to load texture: " << path);
        return;
    }
    GenerateTexture(data, settings);
    stbi_image_free(data);
}

Texture::Texture(const unsigned char* data, int width_, int height_, int channels_, const TextureSettings& settings)
{
    width = width_;
    height = height_;
    channels = channels_;

    GenerateTexture(data, settings);
}

Texture::~Texture()
{
    if (id != 0)
    {
        glDeleteTextures(1, &id);
    }
}

void Texture::BindToUnit(unsigned int unit) const
{
    glBindTextureUnit(unit, id);
}
void Texture::BindAsImage(unsigned int  unit, ImageAccess access, ImageFormat  format, int level) const
{
    glBindImageTexture(unit, id, level, GL_FALSE, 0, ConvertAccess(access), ConvertFormat(format));
}
void Texture::UnBind(unsigned int unit) const
{
    glBindTextureUnit(unit, 0);
}

void Texture::GenerateTexture(const unsigned char* data, const TextureSettings& settings)
{
    GLenum internalFormat = GL_RGBA8;
    GLenum pixelFormat = GL_RGBA;

    if (channels == 1)
    {
        internalFormat = GL_R8;
        pixelFormat = GL_RED;
    }
    else if (channels == 3)
    {
        internalFormat = GL_RGB8;
        pixelFormat = GL_RGB;
    }
    else if (channels == 4)
    {
        internalFormat = GL_RGBA8;
        pixelFormat = GL_RGBA;
    }

    GLint prev = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glCreateTextures(GL_TEXTURE_2D, 1, &id);
    glTextureStorage2D(id, settings.generateMipmap ? 1 + floor(log2(std::max(width, height))) : 1, internalFormat, width, height);
    if (data)
        glTextureSubImage2D(id, 0, 0, 0, width, height, pixelFormat, GL_UNSIGNED_BYTE, data);
    else
    {
        if (pixelFormat == GL_RGBA)
        {
            const GLuint zero[4] = { 0,0,0,0 };
            glClearTexImage(id, 0, GL_RGBA, GL_UNSIGNED_INT, zero);
        }
        else if (pixelFormat == GL_RGB)
        {
            const GLuint zero[3] = { 0,0,0 };
            glClearTexImage(id, 0, GL_RGB, GL_UNSIGNED_INT, zero);
        }
    }
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, ConvertMinFilter(settings.minFilter));
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, ConvertMagFilter(settings.magFilter));
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, ConvertWrap(settings.wrapS));
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, ConvertWrap(settings.wrapT));

    if (settings.generateMipmap)
    {
        glGenerateTextureMipmap(id);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, prev);
}

void Texture::ForceUpdateTexture(unsigned int id_, int width_, int height_, int channels_)
{
    id = id_;
    width = width_;
    height = height_;
    channels = channels_;
}

void Texture::ForceUpdateTexture(const unsigned char* data, int width_, int height_, int channels_,
	const TextureSettings& settings)
{
    if (id != 0)
    {
        glDeleteTextures(1, &id);
    }
    width = width_;
    height = height_;
    channels = channels_;
    GenerateTexture(data, settings);

}
