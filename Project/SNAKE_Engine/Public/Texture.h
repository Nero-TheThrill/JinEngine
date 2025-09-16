#pragma once
#include <string>

using FilePath = std::string;

/**
 * @brief Specifies texture minification filter modes.
 *
 * @details
 * Defines how a texture is sampled when it is displayed smaller than its original resolution.
 */
enum class TextureMinFilter
{
    Nearest,                ///< Nearest texel sampling.
    Linear,                 ///< Linear interpolation between texels.
    NearestMipmapNearest,   ///< Nearest mipmap level, nearest texel.
    LinearMipmapNearest,    ///< Nearest mipmap level, linear interpolation.
    NearestMipmapLinear,    ///< Linear mipmap selection, nearest texel.
    LinearMipmapLinear      ///< Linear mipmap selection and linear texel interpolation (trilinear).
};

/**
 * @brief Specifies texture magnification filter modes.
 *
 * @details
 * Defines how a texture is sampled when it is displayed larger than its original resolution.
 */
enum class TextureMagFilter
{
    Nearest,  ///< Nearest texel sampling.
    Linear,   ///< Linear interpolation between texels.
};

/**
 * @brief Specifies texture wrapping modes.
 *
 * @details
 * Determines how texture coordinates outside the [0,1] range are handled.
 */
enum class TextureWrap
{
    ClampToEdge,    ///< Clamps coordinates to the edge texel.
    Repeat,         ///< Repeats the texture in tiled fashion.
    MirroredRepeat, ///< Repeats the texture with mirroring.
    ClampToBorder   ///< Coordinates outside use the border color.
};

/**
 * @brief Configuration structure for texture creation.
 *
 * @details
 * Stores filtering, wrapping, and mipmap generation settings used when creating a texture.
 */
struct TextureSettings
{
    TextureMinFilter minFilter = TextureMinFilter::Linear; ///< Minification filter.
    TextureMagFilter magFilter = TextureMagFilter::Linear; ///< Magnification filter.
    TextureWrap wrapS = TextureWrap::ClampToEdge;          ///< Wrapping mode for S (U) axis.
    TextureWrap wrapT = TextureWrap::ClampToEdge;          ///< Wrapping mode for T (V) axis.
    bool generateMipmap = true;                            ///< Whether to generate mipmaps.
};

/**
 * @brief Specifies how a texture image is accessed in shaders.
 */
enum class ImageAccess
{
    ReadOnly,   ///< Shader can only read from the image.
    WriteOnly,  ///< Shader can only write to the image.
    ReadWrite   ///< Shader can read and write to the image.
};

/**
 * @brief Specifies internal image format for texture storage.
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
 * @brief Encapsulates an OpenGL texture object.
 *
 * @details
 * Provides functionality to load, bind, and manage textures in GPU memory.
 * Supports loading from file, raw pixel data, or existing OpenGL texture IDs.
 * Also provides image binding for compute shaders.
 */
class Texture
{
    friend class Material;
    friend class RenderManager;
public:
    /**
     * @brief Constructs a texture wrapper from an existing OpenGL texture ID.
     *
     * @param id_ Existing OpenGL texture ID.
     * @param width_ Texture width in pixels.
     * @param height_ Texture height in pixels.
     * @param channels_ Number of color channels.
     */
    Texture(unsigned int id_, int width_, int height_, int channels_) :id(id_), width(width_), height(height_), channels(channels_) {}

    /**
     * @brief Loads a texture from a file path.
     *
     * @details
     * Uses stb_image to load pixel data from disk, then creates
     * an OpenGL texture with the provided settings.
     *
     * @param path File path to the texture image.
     * @param settings Optional texture creation settings (filters, wrap, mipmaps).
     */
    Texture(const FilePath& path, const TextureSettings& settings = {});

    /**
     * @brief Creates a texture from raw pixel data.
     *
     * @param data Pointer to raw pixel buffer.
     * @param width_ Width of the texture in pixels.
     * @param height_ Height of the texture in pixels.
     * @param channels_ Number of channels (1=R, 3=RGB, 4=RGBA).
     * @param settings Texture creation settings.
     */
    Texture(const unsigned char* data, int width_, int height_, int channels_, const TextureSettings& settings = {});

    /**
     * @brief Destroys the texture and frees GPU resources.
     */
    ~Texture();

    /**
     * @brief Gets the width of the texture.
     *
     * @return Width in pixels.
     */
    [[nodiscard]] int GetWidth() const { return width; }

    /**
     * @brief Gets the height of the texture.
     *
     * @return Height in pixels.
     */
    [[nodiscard]] int GetHeight() const { return height; }

    /**
     * @brief Gets the OpenGL texture ID.
     *
     * @return Texture object ID.
     */
    [[nodiscard]] unsigned int GetID() const { return id; }

    /**
     * @brief Binds the texture to a texture unit for sampling.
     *
     * @param unit Texture unit index.
     */
    void BindToUnit(unsigned int unit) const;

    /**
     * @brief Binds the texture as an image for compute shader access.
     *
     * @param unit Image unit index.
     * @param access Specifies read/write access type.
     * @param format Image format.
     * @param level Mipmap level (default = 0).
     */
    void BindAsImage(unsigned int unit, ImageAccess access, ImageFormat format, int level = 0) const;

    /**
     * @brief Unbinds the texture from the given texture unit.
     *
     * @param unit Texture unit index.
     */
    void UnBind(unsigned int unit) const;

private:
    /**
     * @brief Generates and uploads the texture data to GPU.
     *
     * @details
     * Allocates storage for the texture, uploads pixel data if provided,
     * applies filtering/wrapping settings, and generates mipmaps if enabled.
     *
     * @param data Raw pixel data (can be nullptr for empty textures).
     * @param settings Texture creation settings.
     */
    void GenerateTexture(const unsigned char* data, const TextureSettings& settings);

    unsigned int id; ///< OpenGL texture ID
    int width, height, channels; ///< Dimensions and channel count
};
