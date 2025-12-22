#pragma once
#include <string>

using FilePath = std::string;

/**
 * @brief Specifies texture minification filtering modes.
 *
 * @details
 * These values are mapped to OpenGL texture minification filter states and define
 * how a texture is sampled when it is displayed smaller than its original resolution.
 */
enum class TextureMinFilter
{
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

/**
 * @brief Specifies texture magnification filtering modes.
 *
 * @details
 * These values are mapped to OpenGL texture magnification filter states and define
 * how a texture is sampled when it is displayed larger than its original resolution.
 */
enum class TextureMagFilter
{
    Nearest,
    Linear,
};

/**
 * @brief Specifies texture coordinate wrapping behavior.
 *
 * @details
 * These values correspond to OpenGL texture wrap modes and define how texture
 * coordinates outside the [0, 1] range are handled.
 */
enum class TextureWrap
{
    ClampToEdge,
    Repeat,
    MirroredRepeat,
    ClampToBorder
};

/**
 * @brief Describes configuration parameters used when creating or updating a texture.
 *
 * @details
 * TextureSettings encapsulates common OpenGL texture state such as filtering,
 * wrapping, and mipmap generation. It is passed to texture creation and update
 * routines to ensure consistent configuration.
 */
struct TextureSettings
{
    TextureMinFilter minFilter = TextureMinFilter::Linear;
    TextureMagFilter magFilter = TextureMagFilter::Linear;
    TextureWrap wrapS = TextureWrap::ClampToEdge;
    TextureWrap wrapT = TextureWrap::ClampToEdge;
    bool generateMipmap = true;
};

/**
 * @brief Specifies access permissions when binding a texture as an image.
 *
 * @details
 * These values map directly to OpenGL image load/store access qualifiers and
 * control whether shaders can read from, write to, or both read and write to
 * the bound image.
 */
enum class ImageAccess
{
    ReadOnly,
    WriteOnly,
    ReadWrite
};

/**
 * @brief Specifies internal image formats for image load/store operations.
 *
 * @details
 * The formats define the internal storage layout and precision when a texture
 * is bound as an image. They correspond to OpenGL internal image formats.
 */
enum class ImageFormat
{
    R8, RG8, RGBA8,
    R16F, RG16F, RGBA16F,
    R32F, RG32F, RGBA32F,
    R8UI, RG8UI, RGBA8UI,
    R32UI, RG32UI, RGBA32UI
};

/**
 * @brief Encapsulates an OpenGL texture object and its associated metadata.
 *
 * @details
 * Texture manages the lifetime of an OpenGL texture ID along with its width,
 * height, and channel count. It provides interfaces for binding the texture
 * for sampling or image load/store operations.
 *
 * Texture creation can be driven from a file path, raw pixel data, or an
 * externally supplied OpenGL texture ID. Low-level update and generation
 * functions are restricted to friend classes such as Material and RenderManager.
 */
class Texture
{
    friend class Material;
    friend class RenderManager;
public:
    /**
     * @brief Wraps an existing OpenGL texture ID without generating a new texture.
     *
     * @param id_       Existing OpenGL texture ID.
     * @param width_    Texture width in pixels.
     * @param height_   Texture height in pixels.
     * @param channels_ Number of color channels.
     */
    Texture(unsigned int id_, int width_, int height_, int channels_) :id(id_), width(width_), height(height_), channels(channels_) {}

    /**
     * @brief Creates a texture from an image file.
     *
     * @details
     * The image is loaded from disk and uploaded to the GPU using the provided
     * texture settings.
     *
     * @param path     File path to the image resource.
     * @param settings Texture creation settings.
     */
    Texture(const FilePath& path, const TextureSettings& settings = {});

    /**
     * @brief Creates a texture from raw pixel data in memory.
     *
     * @details
     * The pixel data is uploaded to the GPU and configured according to the
     * provided texture settings.
     *
     * @param data      Pointer to raw pixel data.
     * @param width_    Texture width in pixels.
     * @param height_   Texture height in pixels.
     * @param channels_ Number of color channels.
     * @param settings  Texture creation settings.
     */
    Texture(const unsigned char* data, int width_, int height_, int channels_, const TextureSettings& settings = {});

    /**
     * @brief Destroys the texture and releases the OpenGL resource.
     */
    ~Texture();

    /**
     * @brief Returns the texture width in pixels.
     */
    [[nodiscard]] int GetWidth() const { return width; }

    /**
     * @brief Returns the texture height in pixels.
     */
    [[nodiscard]] int GetHeight() const { return height; }

    /**
     * @brief Returns the underlying OpenGL texture ID.
     */
    [[nodiscard]] unsigned int GetID() const { return id; }

    /**
     * @brief Binds the texture to a texture unit for sampling.
     *
     * @param unit Texture unit index.
     */
    void BindToUnit(unsigned int unit) const;

    /**
     * @brief Binds the texture as an image for shader image load/store operations.
     *
     * @param unit   Image unit index.
     * @param access Access permissions for the image.
     * @param format Internal image format.
     * @param level  Mipmap level to bind.
     */
    void BindAsImage(unsigned int unit, ImageAccess access, ImageFormat format, int level = 0) const;

    /**
     * @brief Unbinds the texture from a texture unit.
     *
     * @param unit Texture unit index.
     */
    void UnBind(unsigned int unit) const;

private:
    /**
     * @brief Generates and configures an OpenGL texture from raw data.
     *
     * @param data     Pointer to pixel data.
     * @param settings Texture configuration settings.
     */
    void GenerateTexture(const unsigned char* data, const TextureSettings& settings);

    /**
     * @brief Forcibly updates the internal texture metadata to match an existing ID.
     *
     * @param id_       OpenGL texture ID.
     * @param width_    Texture width.
     * @param height_   Texture height.
     * @param channels_ Number of channels.
     */
    void ForceUpdateTexture(unsigned int id_, int width_, int height_, int channels_);

    /**
     * @brief Forcibly replaces the texture contents and configuration.
     *
     * @param data      Pointer to new pixel data.
     * @param width_    Texture width.
     * @param height_   Texture height.
     * @param channels_ Number of channels.
     * @param settings  Texture configuration settings.
     */
    void ForceUpdateTexture(const unsigned char* data, int width_, int height_, int channels_, const TextureSettings& settings = {});

    unsigned int id;
    int width, height, channels;
};
