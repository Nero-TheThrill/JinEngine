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

enum class TextAlignH
{
    Left,
    Center,
    Right
};

enum class TextAlignV
{
    Top,
    Middle,
    Bottom
};

struct Glyph
{
    glm::ivec2 size;
    glm::ivec2 bearing;
    uint32_t advance;
    glm::vec2 uvTopLeft;
    glm::vec2 uvBottomRight;
};

class Font
{
public:
    Font(RenderManager& engineContext, const std::string& ttfPath, uint32_t fontSize);
    ~Font();

    [[nodiscard]] std::shared_ptr<Material> GetMaterial() const { return material; }

    [[nodiscard]] glm::vec2 GetTextSize(const std::string& text) const;

    [[nodiscard]] std::shared_ptr<Mesh> GenerateTextMesh(const std::string& text, TextAlignH alignH = TextAlignH::Left, TextAlignV alignV = TextAlignV::Top);

    int GetTextAtlasVersion() { return atlasVersion; }

private:
    void LoadFont(const std::string& path, uint32_t fontSize);

    void BakeAtlas(RenderManager& renderManager);

    [[nodiscard]] const Glyph& GetGlyph(char32_t c) const;

    [[nodiscard]] bool TryBakeGlyph(char32_t c);

    void ExpandAtlas();

    FT_Library ft;
    FT_Face face;

    uint32_t fontSize;

    std::unordered_map<char32_t, Glyph> glyphs;
    std::shared_ptr<Texture> atlasTexture;
    std::shared_ptr<Material> material;

    int nextX = 0;
    int nextY = 0;
    int maxRowHeight = 0;

    int atlasVersion = 0;
};
