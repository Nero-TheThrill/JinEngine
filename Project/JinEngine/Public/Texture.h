#pragma once
#include <string>

using FilePath = std::string;

enum class TextureMinFilter
{
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class TextureMagFilter
{
    Nearest,
    Linear,
};

enum class TextureWrap
{
    ClampToEdge,
    Repeat,
    MirroredRepeat,
    ClampToBorder
};

/**
 * @brief Describes sampler-state options used when creating a texture object.
 *
 * @details
 * This structure is consumed by Texture constructors and by the internal texture
 * regeneration path. It controls minification and magnification filters, wrap
 * modes for the S and T axes, and whether mipmaps are generated after uploading
 * the texture data.
 *
 * The implementation applies these values directly to the OpenGL texture object
 * after storage allocation and pixel upload. When generateMipmap is enabled,
 * the texture allocates enough mip levels for the full chain and generates them
 * immediately after the base level upload.
 *
 * @note
 * These settings are only applied when a GPU texture is created or recreated.
 * Updating a wrapper to reference an external OpenGL texture ID does not reapply
 * these settings automatically.
 */
struct TextureSettings
{
    TextureMinFilter minFilter = TextureMinFilter::Linear;
    TextureMagFilter magFilter = TextureMagFilter::Linear;
    TextureWrap wrapS = TextureWrap::ClampToEdge;
    TextureWrap wrapT = TextureWrap::ClampToEdge;
    bool generateMipmap = true;
};

enum class ImageAccess
{
    ReadOnly,
    WriteOnly,
    ReadWrite
};

enum class ImageFormat
{
    R8, RG8, RGBA8,
    R16F, RG16F, RGBA16F,
    R32F, RG32F, RGBA32F,
    R8UI, RG8UI, RGBA8UI,
    R32UI, RG32UI, RGBA32UI
};

class Texture
{
    friend class Material;
    friend class RenderManager;

public:
    /**
     * @brief Wraps an already existing OpenGL texture object.
     *
     * @details
     * This constructor does not allocate GPU storage or upload any pixel data.
     * It simply stores an existing OpenGL texture ID together with metadata such
     * as width, height, and channel count so the engine can treat the external
     * texture like a regular Texture resource.
     *
     * This is mainly used by internal engine systems that create textures through
     * other OpenGL paths, such as render targets or externally managed resources.
     *
     * @param id_ Existing OpenGL texture object ID.
     * @param width_ Width of the texture in pixels.
     * @param height_ Height of the texture in pixels.
     * @param channels_ Number of color channels represented by the texture data.
     *
     * @note
     * Since this object takes ownership of the texture ID at the wrapper level,
     * the destructor will delete the OpenGL texture if the stored ID is non-zero.
     */
    Texture(unsigned int id_, int width_, int height_, int channels_) :id(id_), width(width_), height(height_), channels(channels_) {}

    /**
     * @brief Loads an image file from disk and creates a GPU texture from it.
     *
     * @details
     * The implementation uses stb_image to load the file contents. Vertical flip
     * on load is enabled before reading, so image data is imported in a form that
     * matches the engine's texture coordinate convention.
     *
     * After loading succeeds, the constructor forwards the pixel buffer and the
     * requested TextureSettings to GenerateTexture(), which allocates immutable
     * texture storage, uploads the base image, applies filter and wrap state,
     * and optionally generates mipmaps.
     *
     * If loading fails, the texture remains with an invalid or empty state and an
     * error is logged.
     *
     * @param path Path to the image file to load.
     * @param settings Texture creation settings such as filtering, wrapping, and mipmap generation.
     *
     * @note
     * The loaded CPU-side image buffer is released immediately after the GPU
     * texture has been created.
     */
    Texture(const FilePath& path, const TextureSettings& settings = {});

    /**
     * @brief Creates a GPU texture directly from raw pixel data in memory.
     *
     * @details
     * This constructor is used when image bytes are already available in memory,
     * such as asynchronously loaded textures, generated images, font atlases, or
     * procedural content. The provided width, height, and channel count are stored
     * first, then GenerateTexture() creates the OpenGL texture and uploads the
     * supplied data.
     *
     * Supported channel counts in the implementation are 1, 3, and 4, which map
     * to red, RGB, and RGBA texture formats respectively.
     *
     * @param data Pointer to raw pixel data to upload. Passing nullptr creates an allocated texture with cleared contents.
     * @param width_ Width of the texture in pixels.
     * @param height_ Height of the texture in pixels.
     * @param channels_ Number of channels in the source pixel data.
     * @param settings Texture creation settings such as filtering, wrapping, and mipmap generation.
     */
    Texture(const unsigned char* data, int width_, int height_, int channels_, const TextureSettings& settings = {});

    /**
     * @brief Releases the owned OpenGL texture object.
     *
     * @details
     * If the stored texture ID is non-zero, the implementation deletes the
     * underlying OpenGL texture object during destruction.
     *
     * @note
     * This assumes the Texture instance owns the currently stored texture ID.
     * Be careful when replacing the internal ID through force-update paths.
     */
    ~Texture();

    /**
     * @brief Returns the stored texture width.
     *
     * @return Width in pixels.
     */
    [[nodiscard]] int GetWidth() const { return width; }

    /**
     * @brief Returns the stored texture height.
     *
     * @return Height in pixels.
     */
    [[nodiscard]] int GetHeight() const { return height; }

    /**
     * @brief Returns the underlying OpenGL texture object ID.
     *
     * @return OpenGL texture ID.
     */
    [[nodiscard]] unsigned int GetID() const { return id; }

    /**
     * @brief Binds this texture to a texture sampling unit.
     *
     * @details
     * This is the standard binding path used by materials when sending sampler
     * uniforms to shaders. The method binds the texture object to the specified
     * texture unit using direct state access style OpenGL calls.
     *
     * @param unit Texture unit index to bind to.
     */
    void BindToUnit(unsigned int unit) const;

    /**
     * @brief Binds this texture as an image unit for shader image access.
     *
     * @details
     * This path is intended for compute shaders or other shader stages that use
     * image load/store operations rather than sampler-based reads. The access
     * mode and image format are converted to the corresponding OpenGL enums and
     * bound through glBindImageTexture().
     *
     * @param unit Image unit index to bind to.
     * @param access Requested read/write access mode for the shader.
     * @param format Declared image format exposed to the shader.
     * @param level Mipmap level to bind. Defaults to the base level.
     */
    void BindAsImage(unsigned int unit, ImageAccess access, ImageFormat format, int level = 0) const;

    /**
     * @brief Unbinds any texture currently bound to the given texture unit.
     *
     * @details
     * This clears the binding for the specified sampling unit by binding texture
     * ID 0 to that unit. It is used when a material is being unbound so that the
     * next draw call starts from a clean texture-unit state.
     *
     * @param unit Texture unit index to clear.
     */
    void UnBind(unsigned int unit) const;

private:
    /**
     * @brief Allocates GPU storage and uploads pixel data according to the given settings.
     *
     * @details
     * This internal helper chooses the OpenGL internal format and upload format
     * from the stored channel count, creates the texture object, allocates
     * immutable storage, uploads the base image if data is present, applies
     * sampler parameters, and optionally generates mipmaps.
     *
     * When data is nullptr, the implementation still creates the texture and
     * clears its contents so the texture can be used as an empty resource or
     * destination surface.
     *
     * @param data Pixel buffer to upload. May be nullptr.
     * @param settings Texture creation settings applied to the new OpenGL texture.
     *
     * @note
     * The implementation temporarily forces GL_UNPACK_ALIGNMENT to 1 so tightly
     * packed image data uploads correctly regardless of row alignment.
     */
    void GenerateTexture(const unsigned char* data, const TextureSettings& settings);

    /**
     * @brief Replaces this wrapper with an already existing OpenGL texture object.
     *
     * @details
     * This internal function updates the stored texture ID and metadata without
     * creating a new texture. It is used by engine systems that generate textures
     * elsewhere and then want the Texture resource table to reference that object.
     *
     * @param id_ Existing OpenGL texture object ID.
     * @param width_ Width of the texture in pixels.
     * @param height_ Height of the texture in pixels.
     * @param channels_ Number of channels represented by the texture.
     *
     * @note
     * This function does not delete the previously stored texture ID before
     * replacing it.
     */
    void ForceUpdateTexture(unsigned int id_, int width_, int height_, int channels_);

    /**
     * @brief Recreates the texture from raw pixel data and replaces the current GPU object.
     *
     * @details
     * If this Texture already owns a valid OpenGL texture object, the old object
     * is deleted first. The stored metadata is then updated and GenerateTexture()
     * is called to allocate and upload the new texture contents.
     *
     * This is used when the engine wants to refresh an existing registered texture
     * entry with new image data while preserving the same resource handle in the
     * resource map.
     *
     * @param data Pointer to the new pixel buffer.
     * @param width_ Width of the new texture in pixels.
     * @param height_ Height of the new texture in pixels.
     * @param channels_ Number of channels in the new pixel data.
     * @param settings Texture creation settings for the recreated texture.
     */
    void ForceUpdateTexture(const unsigned char* data, int width_, int height_, int channels_, const TextureSettings& settings = {});

    unsigned int id;
    int width, height, channels;
};