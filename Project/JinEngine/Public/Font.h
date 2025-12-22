#pragma once

#include <memory>
#include <unordered_map>
#include <string>

#include "glm.hpp"
#include "ft2build.h"
#include FT_FREETYPE_H

#include "Texture.h"
#include "Material.h"

class Camera2D;

struct EngineContext;

/**
 * @brief Horizontal alignment options for text mesh generation.
 *
 * @details
 * Used by Font::GenerateTextMesh to offset the starting X cursor
 * per line based on the computed line width.
 */
enum class TextAlignH
{
    Left,
    Center,
    Right
};

/**
 * @brief Vertical alignment options for multi-line text mesh generation.
 *
 * @details
 * Used by Font::GenerateTextMesh to offset the starting Y cursor
 * based on total line count and line spacing.
 */
enum class TextAlignV
{
    Top,
    Middle,
    Bottom
};

/**
 * @brief Cached glyph metrics and atlas UV coordinates for a single Unicode codepoint.
 *
 * @details
 * This struct stores the data needed to layout and render a glyph quad:
 * - size: bitmap width/height in pixels
 * - bearing: left/top bearing in pixels from baseline
 * - advance: FreeType advance value in 26.6 fixed-point format (advance >> 6 = pixels)
 * - uvTopLeft / uvBottomRight: normalized UV region in the font atlas texture
 *
 * Glyph instances are stored in Font::glyphs and are populated during baking
 * via Font::TryBakeGlyph.
 */
struct Glyph
{
    glm::ivec2 size;
    glm::ivec2 bearing;
    uint32_t advance;
    glm::vec2 uvTopLeft;
    glm::vec2 uvBottomRight;
};

/**
 * @brief FreeType-backed font that generates text meshes using a dynamically baked atlas.
 *
 * @details
 * Font loads a TTF face using FreeType and maintains a single-channel (GL_RED)
 * atlas texture that is populated on demand.
 *
 * Key behaviors based on the implementation:
 * - The input string is treated as UTF-8 and decoded into Unicode codepoints.
 * - Font::TryBakeGlyph lazily renders a glyph via FT_Load_Char(..., FT_LOAD_RENDER)
 *   and uploads it into the atlas using glTextureSubImage2D.
 * - If the atlas becomes full, Font::ExpandAtlas doubles the atlas size and re-bakes
 *   all previously baked glyphs into the new texture, then increments atlasVersion.
 * - Font::GenerateTextMesh builds a quad per glyph (4 vertices, 6 indices) and supports
 *   horizontal/vertical alignment for multi-line text.
 *
 * The Font also owns a Material that references the atlas texture via the uniform
 * name "u_FontTexture" and sets a default "u_Color" uniform.
 */
class Font
{
public:
    /**
     * @brief Creates a font from a TTF file and builds the initial empty atlas/material.
     *
     * @details
     * This constructor:
     * 1) Initializes FreeType and loads the face from ttfPath.
     * 2) Sets the pixel size to fontSize.
     * 3) Creates an initial atlas texture (starting at 128x128, single channel).
     * 4) Creates a Material using the internal text shader tag
     *    "[EngineShader]internal_text" and binds the atlas to "u_FontTexture".
     *
     * Note: The parameter name is engineContext in the header, but the implementation
     * uses it as a RenderManager reference for shader lookup and material setup.
     *
     * @param engineContext RenderManager used to obtain the internal text shader and configure the material.
     * @param ttfPath Path to the .ttf font file.
     * @param fontSize Pixel height used for FT_Set_Pixel_Sizes.
     */
    Font(RenderManager& engineContext, const std::string& ttfPath, uint32_t fontSize);

    /**
     * @brief Destroys the font and releases FreeType resources.
     *
     * @details
     * Calls FT_Done_Face and FT_Done_FreeType.
     * (GPU resources such as Texture/Material are released via unique_ptr ownership.)
     */
    ~Font();

    /**
     * @brief Returns the Material used for text rendering.
     *
     * @details
     * The returned material references the atlas texture via "u_FontTexture".
     *
     * @return Pointer to the owned Material.
     */
    [[nodiscard]] Material* GetMaterial() const { return material.get(); }

    /**
     * @brief Calculates the pixel size of a UTF-8 string using cached glyph advances.
     *
     * @details
     * The width is computed by summing (advance >> 6) per codepoint for each line,
     * and taking the maximum line width.
     *
     * The height is computed as (fontSize * lineCount), where lineCount is the number
     * of newline-separated lines in the string.
     *
     * This function uses cached Glyph data via GetGlyph and does not bake new glyphs.
     * If a codepoint is missing from the cache, the fallback behavior of GetGlyph applies.
     *
     * @param text UTF-8 text, may contain '\n' for multiple lines.
     * @return {maxLineWidth, totalHeight} in pixels.
     */
    [[nodiscard]] glm::vec2 GetTextSize(const std::string& text) const;

    /**
     * @brief Generates a mesh containing quads for the given UTF-8 text.
     *
     * @details
     * This function:
     * - Splits the input by '\n' into lines.
     * - Pre-bakes glyphs for each line to compute accurate line widths.
     * - Applies horizontal alignment per line by shifting the X cursor:
     *   - Left: no shift
     *   - Center: -lineWidth * 0.5
     *   - Right: -lineWidth
     * - Applies vertical alignment by shifting the starting Y cursor based on total height.
     * - For each codepoint, appends a quad (4 vertices, 6 indices) using the glyph's
     *   bearing, size, and atlas UVs.
     *
     * The returned Mesh is allocated with new and must be managed/freed by the caller
     * according to the engine's ownership conventions.
     *
     * @param text UTF-8 text, may contain '\n' for multiple lines.
     * @param alignH Horizontal alignment mode.
     * @param alignV Vertical alignment mode.
     * @return Newly allocated Mesh pointer containing the generated geometry.
     */
    [[nodiscard]] Mesh* GenerateTextMesh(const std::string& text, TextAlignH alignH = TextAlignH::Left, TextAlignV alignV = TextAlignV::Top);

    /**
     * @brief Returns the current atlas version counter.
     *
     * @details
     * atlasVersion is incremented when the atlas is expanded and re-baked
     * (Font::ExpandAtlas). This can be used by render code to detect when
     * cached GPU bindings/state should be refreshed.
     *
     * @return Integer version that increases when the atlas is rebuilt.
     */
    int GetTextAtlasVersion() { return atlasVersion; }

private:
    /**
     * @brief Loads and configures the FreeType face from a .ttf file.
     *
     * @details
     * Initializes the FreeType library (FT_Init_FreeType),
     * loads the font face (FT_New_Face),
     * and sets the pixel size (FT_Set_Pixel_Sizes).
     *
     * @param path Path to the .ttf font file.
     * @param fontSize Pixel height used for FT_Set_Pixel_Sizes.
     */
    void LoadFont(const std::string& path, uint32_t fontSize);

    /**
     * @brief Creates the initial atlas texture and text material.
     *
     * @details
     * - Allocates a 128x128 single-channel atlas (initially filled with zeros).
     * - Temporarily forces GL_UNPACK_ALIGNMENT to 1 for tight byte uploads.
     * - Creates a Material using shader tag "[EngineShader]internal_text".
     * - Binds the atlas texture to "u_FontTexture" and sets "u_Color" to (1,1,1,1).
     * - Resets packing cursor state (nextX, nextY, maxRowHeight).
     *
     * @param renderManager RenderManager used to look up the internal text shader.
     */
    void BakeAtlas(RenderManager& renderManager);

    /**
     * @brief Returns the cached Glyph for a Unicode codepoint.
     *
     * @details
     * If the glyph is missing, this function attempts to return the cached fallback
     * glyph for U'?' if it exists. If neither exists, it returns a static empty Glyph.
     *
     * This function does not perform baking; use TryBakeGlyph to populate glyphs.
     *
     * @param c Unicode codepoint.
     * @return Reference to the cached Glyph (or fallback/empty glyph).
     */
    [[nodiscard]] const Glyph& GetGlyph(char32_t c) const;

    /**
     * @brief Ensures the glyph for a Unicode codepoint exists in the atlas cache.
     *
     * @details
     * If the glyph already exists in Font::glyphs, returns true immediately.
     *
     * Otherwise:
     * - Renders the glyph via FreeType (FT_Load_Char with FT_LOAD_RENDER).
     * - Computes a padded cell placement in the atlas.
     * - Uploads bitmap data into the atlas using glTextureSubImage2D.
     * - Stores glyph metrics and UVs in Font::glyphs.
     *
     * If the atlas does not have enough space, ExpandAtlas is called, and this function
     * retries the baking operation.
     *
     * @param c Unicode codepoint to bake.
     * @return true if the glyph is available after the call; false on failure.
     */
    [[nodiscard]] bool TryBakeGlyph(char32_t c);

    /**
     * @brief Expands the atlas texture and re-bakes all cached glyphs.
     *
     * @details
     * Doubles the atlas width and height, creates a new Texture,
     * rebinds the material's "u_FontTexture" to the new atlas,
     * clears the glyph cache, then re-bakes every previously known codepoint.
     *
     * Increments atlasVersion after the rebuild completes.
     */
    void ExpandAtlas();

    FT_Library ft;
    FT_Face face;

    uint32_t fontSize;

    std::unordered_map<char32_t, Glyph> glyphs;
    std::unique_ptr<Texture> atlasTexture;
    std::unique_ptr<Material> material;

    int nextX = 0;
    int nextY = 0;
    int maxRowHeight = 0;

    int atlasVersion = 0;
};
