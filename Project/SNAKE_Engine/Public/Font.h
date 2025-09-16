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
 * @brief Horizontal alignment options for text rendering.
 */
enum class TextAlignH
{
    Left,   ///< Align text to the left edge.
    Center, ///< Align text to the center.
    Right   ///< Align text to the right edge.
};

/**
 * @brief Vertical alignment options for text rendering.
 */
enum class TextAlignV
{
    Top,    ///< Align text to the top edge.
    Middle, ///< Align text to the vertical middle.
    Bottom  ///< Align text to the bottom edge.
};

/**
 * @brief Glyph information for a single character.
 *
 * @details
 * Stores FreeType-extracted glyph metrics and its location
 * in the texture atlas (UV coordinates).
 */
struct Glyph
{
    glm::ivec2 size;         ///< Size of the glyph in pixels.
    glm::ivec2 bearing;      ///< Offset from baseline to left/top.
    uint32_t advance;        ///< Advance to next glyph in 1/64 pixels.
    glm::vec2 uvTopLeft;     ///< UV coordinates of top-left corner.
    glm::vec2 uvBottomRight; ///< UV coordinates of bottom-right corner.
};

/**
 * @brief Font class for text rendering using FreeType.
 *
 * @details
 * Font handles glyph extraction from a TTF file, dynamic atlas baking,
 * and text mesh generation. Each Font maintains its own texture atlas
 * and associated Material used for rendering text.
 *
 * Atlas baking uses a "lazy baking" strategy: missing glyphs are added
 * to the atlas when requested. The atlas can expand if more space is needed.
 */
class Font
{
public:
    /**
     * @brief Constructs a Font and loads glyphs from a TTF file.
     * @param renderManager RenderManager for registering the atlas texture.
     * @param ttfPath Path to a TrueType font file.
     * @param fontSize Requested pixel size of the font.
     */
    Font(RenderManager& renderManager, const std::string& ttfPath, uint32_t fontSize);

    /**
     * @brief Destructor. Frees FreeType resources.
     */
    ~Font();

    /**
     * @brief Returns the material associated with this font.
     *
     * @details
     * The material internally binds the atlas texture and a text shader.
     */
    [[nodiscard]] Material* GetMaterial() const { return material.get(); }

    /**
     * @brief Computes the size of the given text string in pixels.
     *
     * @param text UTF-8 encoded string.
     * @return The 2D size of the rendered text in pixels.
     */
    [[nodiscard]] glm::vec2 GetTextSize(const std::string& text) const;

    /**
     * @brief Generates a mesh for the given text.
     *
     * @param text UTF-8 encoded string.
     * @param alignH Horizontal alignment (default: Left).
     * @param alignV Vertical alignment (default: Top).
     * @return Pointer to a Mesh representing the text geometry.
     */
    [[nodiscard]] Mesh* GenerateTextMesh(const std::string& text, TextAlignH alignH = TextAlignH::Left, TextAlignV alignV = TextAlignV::Top);

    /**
     * @brief Returns the atlas version number.
     *
     * @details
     * Increments when the atlas is rebuilt or expanded,
     * allowing text objects to detect when they need to update.
     */
    int GetTextAtlasVersion() { return atlasVersion; }

private:
    /**
     * @brief Loads a font face using FreeType.
     * @param path Path to TTF file.
     * @param fontSize Font size in pixels.
     */
    void LoadFont(const std::string& path, uint32_t fontSize);

    /**
     * @brief Builds the initial glyph atlas and uploads it to GPU.
     * @param renderManager RenderManager used for registering textures.
     */
    void BakeAtlas(RenderManager& renderManager);

    /**
     * @brief Retrieves the glyph for a given character.
     * @param c Unicode codepoint.
     * @return Reference to the glyph.
     */
    [[nodiscard]] const Glyph& GetGlyph(char32_t c) const;

    /**
     * @brief Attempts to bake a glyph into the atlas if not already present.
     * @param c Unicode codepoint.
     * @return True if successfully baked or already exists.
     */
    [[nodiscard]] bool TryBakeGlyph(char32_t c);

    /**
     * @brief Expands the atlas texture to accommodate more glyphs.
     */
    void ExpandAtlas();

    FT_Library ft;   ///< FreeType library handle.
    FT_Face face;    ///< FreeType font face.

    uint32_t fontSize; ///< Font size in pixels.

    std::unordered_map<char32_t, Glyph> glyphs; ///< Map of codepoints to glyphs.
    std::unique_ptr<Texture> atlasTexture;      ///< Texture atlas storing glyph bitmaps.
    std::unique_ptr<Material> material;         ///< Material using atlas + text shader.

    int nextX = 0;          ///< Current X offset for atlas packing.
    int nextY = 0;          ///< Current Y offset for atlas packing.
    int maxRowHeight = 0;   ///< Max height of the current row in atlas.
    int atlasVersion = 0;   ///< Version number of atlas, incremented on rebuild.
};
