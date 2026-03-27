#include "Engine.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <unordered_set>

#include "gl.h"

static std::vector<char32_t> UTF8ToCodepoints(const std::string& text)
{
    std::vector<char32_t> result;
    size_t i = 0;

    while (i < text.size())
    {
        char32_t cp = 0;
        unsigned char byte = text[i];

        if ((byte & 0b10000000) == 0)
        {
            cp = byte;
            ++i;
        }
        else if ((byte & 0b11100000) == 0b11000000)
        {
            cp = (byte & 0b00011111) << 6;
            cp |= (text[i + 1] & 0b00111111);
            i += 2;
        }
        else if ((byte & 0b11110000) == 0b11100000)
        {
            cp = (byte & 0b00001111) << 12;
            cp |= (text[i + 1] & 0b00111111) << 6;
            cp |= (text[i + 2] & 0b00111111);
            i += 3;
        }
        else if ((byte & 0b11111000) == 0b11110000)
        {
            cp = (byte & 0b00000111) << 18;
            cp |= (text[i + 1] & 0b00111111) << 12;
            cp |= (text[i + 2] & 0b00111111) << 6;
            cp |= (text[i + 3] & 0b00111111);
            i += 4;
        }
        else
        {
            ++i;
            continue;
        }

        result.push_back(cp);
    }

    return result;
}


Font::Font(RenderManager& renderManager, const std::string& ttfPath, uint32_t fontSize_)
{
    LoadFont(ttfPath, fontSize_);
    BakeAtlas(renderManager);
    fontSize = fontSize_;
}

Font::~Font()
{
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void Font::LoadFont(const std::string& path, uint32_t fontSize)
{
    if (FT_Init_FreeType(&ft))
        throw std::runtime_error("Failed to init FreeType");

    if (FT_New_Face(ft, path.c_str(), 0, &face))
        throw std::runtime_error("Failed to load font: " + path);

    FT_Set_Pixel_Sizes(face, 0, fontSize);
}

void Font::BakeAtlas(RenderManager& renderManager)
{
    int texWidth = 128;
    int texHeight = 128;
    std::vector<unsigned char> pixels(texWidth * texHeight, 0);
    GLint prevAlignment = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    atlasTexture = std::make_shared<Texture>(pixels.data(), texWidth, texHeight, 1);

    std::shared_ptr<Shader> textShader = renderManager.GetShaderByTag("[EngineShader]internal_text");
    material = std::make_shared<Material>(textShader);
    material->SetTexture("u_FontTexture", atlasTexture);
    material->SetUniform("u_Color", glm::vec4(1.0f));

    nextX = 0;
    nextY = 0;
    maxRowHeight = 0;
    glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);
}

bool Font::TryBakeGlyph(char32_t c)
{
    if (glyphs.find(c) != glyphs.end())
        return true;

    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
    {
        JIN_ERR("FT_Load_Char failed for: U+" << std::hex << (int)c);
        return false;
    }

    FT_GlyphSlot g = face->glyph;
    int w = g->bitmap.width;
    int h = g->bitmap.rows;
    bool hasBitmap = (w > 0 && h > 0);
    bool hasAdvance = (g->advance.x > 0);

    if (!hasBitmap && !hasAdvance)
    {
        JIN_WRN("Glyph has no bitmap or advance: U+" << std::hex << (int)c);
        return false;
    }

    const int padding = 1;
    const int safeW = std::max(1, w);
    const int safeH = std::max(1, h);
    const int cellW = safeW + padding * 2;
    const int cellH = safeH + padding * 2;

    if (nextX + cellW > atlasTexture->GetWidth())
    {
        nextX = 0;
        nextY += maxRowHeight;
        maxRowHeight = 0;
    }

    if (nextY + cellH > atlasTexture->GetHeight())
    {
        ExpandAtlas();
        return TryBakeGlyph(c);
    }

    int drawX = nextX + padding;
    int drawY = nextY + padding;

    if (hasBitmap && g->bitmap.buffer)
    {
        GLint prevAlignment = 0;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureSubImage2D(
            atlasTexture->GetID(), 0,
            drawX, drawY, w, h,
            GL_RED, GL_UNSIGNED_BYTE,
            g->bitmap.buffer
        );
        glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);
    }
    else
    {
        unsigned char dummy = 0;
        glTextureSubImage2D(
            atlasTexture->GetID(), 0,
            drawX, drawY, 1, 1,
            GL_RED, GL_UNSIGNED_BYTE,
            &dummy
        );
    }

    Glyph glyph;
    glyph.size = { w, h };
    glyph.bearing = { g->bitmap_left, g->bitmap_top };
    glyph.advance = g->advance.x;

    glyph.uvTopLeft = {
        static_cast<float>(drawX) / atlasTexture->GetWidth(),
        static_cast<float>(drawY) / atlasTexture->GetHeight()
    };
    glyph.uvBottomRight = {
        static_cast<float>(drawX + safeW) / atlasTexture->GetWidth(),
        static_cast<float>(drawY + safeH) / atlasTexture->GetHeight()
    };

    glyphs[c] = glyph;

    nextX += cellW;
    maxRowHeight = std::max(maxRowHeight, cellH);

    return true;
}


const Glyph& Font::GetGlyph(char32_t c) const
{
    auto it = glyphs.find(c);
    if (it != glyphs.end())
        return it->second;

    static const char32_t fallbackCodepoint = U'?';

    auto fallbackIt = glyphs.find(fallbackCodepoint);
    if (fallbackIt != glyphs.end())
        return fallbackIt->second;

    static Glyph empty{};
    return empty;
}

glm::vec2 Font::GetTextSize(const std::string& text) const
{
    float maxWidth = 0.0f;
    float currentLineWidth = 0.0f;
    int lineCount = 1;

    std::vector<char32_t> codepoints = UTF8ToCodepoints(text);
    for (char32_t c : codepoints)
    {
        if (c == U'\n')
        {
            maxWidth = std::max(maxWidth, currentLineWidth);
            currentLineWidth = 0.0f;
            ++lineCount;
            continue;
        }

        const Glyph& glyph = GetGlyph(c);
        currentLineWidth += (glyph.advance >> 6);
    }

    maxWidth = std::max(maxWidth, currentLineWidth);
    float totalHeight = static_cast<float>(fontSize * lineCount);

    return { maxWidth, totalHeight };
}

std::shared_ptr<Mesh> Font::GenerateTextMesh(const std::string& text, TextAlignH alignH, TextAlignV alignV)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t indexOffset = 0;

    std::stringstream ss(text);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(ss, line, '\n'))
        lines.push_back(line);

    float lineSpacing = static_cast<float>(fontSize);
    size_t lineCount = lines.size();

    std::vector<float> lineWidths;
    float maxLineWidth = 0.0f;
    for (const std::string& lineText : lines)
    {
        std::vector<char32_t> u32Line = UTF8ToCodepoints(lineText);
        //pre-bake for calculating width
        for (char32_t c : u32Line)
            if (!TryBakeGlyph(c))
            {
                JIN_WRN("Failed to bake glyph");
            }

        float lineWidth = 0.0f;
        for (char32_t c : u32Line)
            lineWidth += static_cast<float>(GetGlyph(c).advance >> 6);

        maxLineWidth = std::max(maxLineWidth, lineWidth);
        lineWidths.push_back(lineWidth);
    }

    float totalHeight = static_cast<float>(lineCount) * lineSpacing;
    float yStart = 0;
    if (alignV == TextAlignV::Middle)
        yStart += totalHeight * 0.5f - lineSpacing;
    else if (alignV == TextAlignV::Bottom)
        yStart += totalHeight - lineSpacing;
    if (alignV == TextAlignV::Top)
        yStart -= fontSize;

    float yCursor = yStart;

    for (size_t i = 0; i < lineCount; ++i)
    {
        const std::string& lineText = lines[i];
        float lineWidth = lineWidths[i];

        float xCursor = 0;
        if (alignH == TextAlignH::Center)
            xCursor -= lineWidth * 0.5f;
        else if (alignH == TextAlignH::Right)
            xCursor -= lineWidth;

        std::vector<char32_t> u32 = UTF8ToCodepoints(lineText);
        for (char32_t c : u32)
        {
            if (!TryBakeGlyph(c))
            {
                JIN_WRN("Failed to bake glyph");
            }
            const Glyph& glyph = GetGlyph(c);
            float xpos = xCursor + (float)glyph.bearing.x;
            float ypos = yCursor - (float)(glyph.size.y - glyph.bearing.y);
            float w = (float)glyph.size.x;
            float h = (float)glyph.size.y;

            float u0 = glyph.uvTopLeft.x;
            float v0 = glyph.uvTopLeft.y;
            float u1 = glyph.uvBottomRight.x;
            float v1 = glyph.uvBottomRight.y;

            vertices.insert(vertices.end(), {
                { {xpos,     ypos + h, 0.0f }, { u0, v0 } },
                { {xpos,     ypos,     0.0f}, {u0, v1} },
                { {xpos + w, ypos,     0.0f}, {u1, v1} },
                { { xpos + w, ypos + h, 0.0f}, {u1, v0} } });

            indices.insert(indices.end(), {
                indexOffset + 0, indexOffset + 1, indexOffset + 2,
                indexOffset + 0, indexOffset + 2, indexOffset + 3
                });

            xCursor += static_cast<float>(glyph.advance >> 6);
            indexOffset += 4;
        }

        yCursor -= lineSpacing;
    }

    return std::make_shared<Mesh>(vertices, indices);
}


void Font::ExpandAtlas()
{
    int oldWidth = atlasTexture->GetWidth();
    int oldHeight = atlasTexture->GetHeight();

    int newWidth = oldWidth * 2;
    int newHeight = oldHeight * 2;

    std::vector<unsigned char> newPixels(newWidth * newHeight, 0);
    std::shared_ptr<Texture> newAtlas = std::make_shared<Texture>(newPixels.data(), newWidth, newHeight, 1);
    material->SetTexture("u_FontTexture", newAtlas);
    atlasTexture = std::move(newAtlas);

    nextX = 0;
    nextY = 0;
    maxRowHeight = 0;

    std::unordered_map<char32_t, Glyph> oldGlyphs = glyphs;
    glyphs.clear();

    for (const auto& [c, _] : oldGlyphs)
    {
        if (!TryBakeGlyph(c))
        {
            JIN_WRN("Failed to bake glyph");
        }
    }
    atlasVersion++;
}
