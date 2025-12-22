#pragma once
#include "EngineContext.h"
#include "Mesh.h"
#include "Object.h"
#include "Transform.h"

class Camera2D;

/**
 * @brief Lightweight data container representing a text rendering request.
 *
 * @details
 * TextInstance holds a reference to a Font and the UTF-8 text string to be rendered.
 * It does not own the Font resource and is intended to be used as a mutable payload
 * that can trigger mesh regeneration when its contents change.
 */
struct TextInstance
{
    Font* font = nullptr;
    std::string text = "";
};

/**
 * @brief Scene object responsible for rendering text within the standard Object pipeline.
 *
 * @details
 * TextObject is a specialized Object implementation for in-world text rendering.
 * Unlike HUD or immediate-mode text systems, TextObject participates in the normal
 * Object lifecycle (Init / Update / Draw / Free) and render ordering.
 *
 * Internally, TextObject owns a dynamically generated Mesh representing the glyph
 * geometry of the current text. The mesh is regenerated whenever the text content
 * changes or when the associated Font's atlas version changes.
 *
 * TextObject deliberately disables access to generic material, mesh, and animation
 * interfaces inherited from Object to prevent misuse. Text rendering is tightly
 * coupled to Font-driven mesh generation and a dedicated text material/shader setup.
 */
class TextObject : public Object
{
public:
    /**
     * @brief Constructs a TextObject with initial text content and alignment.
     *
     * @details
     * The provided Font pointer is stored but not owned by this object.
     * The actual text mesh is generated during Init/LateInit once the rendering
     * context and font atlas are available.
     *
     * @param font   Font used for glyph lookup and atlas access.
     * @param text   Initial UTF-8 text string.
     * @param alignH Horizontal text alignment.
     * @param alignV Vertical text alignment.
     */
    TextObject(Font* font, const std::string& text, TextAlignH alignH = TextAlignH::Left, TextAlignV alignV = TextAlignV::Top);

    /**
     * @brief Virtual destructor.
     */
    ~TextObject() override = default;

    /**
     * @brief Initializes text-related resources.
     *
     * @details
     * Called during the standard Object initialization phase.
     * This function typically prepares internal state required for text mesh
     * generation but may defer actual mesh creation until LateInit().
     *
     * @param engineContext Engine-wide context providing access to managers.
     */
    void Init([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Performs late initialization after all objects are initialized.
     *
     * @details
     * This stage ensures that Font atlases and rendering resources are fully
     * available before generating the initial text mesh.
     *
     * @param engineContext Engine-wide context.
     */
    void LateInit([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Updates the TextObject each frame.
     *
     * @details
     * This function checks whether the text content or font atlas has changed
     * and triggers mesh regeneration if required.
     *
     * @param dt             Delta time in seconds.
     * @param engineContext Engine-wide context.
     */
    void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Submits the text mesh for rendering.
     *
     * @details
     * The text mesh is submitted through the standard Object rendering path,
     * using a text-specific material and shader configuration.
     *
     * @param engineContext Engine-wide context.
     */
    void Draw([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Releases resources owned by this TextObject.
     *
     * @details
     * This stage releases GPU-side resources associated with the text mesh.
     *
     * @param engineContext Engine-wide context.
     */
    void Free([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Performs late cleanup after all objects have been freed.
     *
     * @param engineContext Engine-wide context.
     */
    void LateFree([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Returns the bounding radius used for spatial queries or culling.
     *
     * @details
     * The radius is derived from the current text mesh extents.
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Sets a new text string and marks the mesh for regeneration.
     *
     * @param text New UTF-8 text string.
     */
    void SetText(const std::string& text);

    /**
     * @brief Replaces the entire TextInstance payload.
     *
     * @details
     * This can change both the font and the text string simultaneously.
     *
     * @param textInstance_ New text instance data.
     */
    void SetTextInstance(const TextInstance& textInstance_);

    /**
     * @brief Sets horizontal text alignment.
     *
     * @param alignH_ New horizontal alignment mode.
     */
    void SetAlignH(TextAlignH alignH_);

    /**
     * @brief Sets vertical text alignment.
     *
     * @param alignV_ New vertical alignment mode.
     */
    void SetAlignV(TextAlignV alignV_);

    /**
     * @brief Returns a mutable pointer to the internal TextInstance.
     *
     * @details
     * Direct modification requires a subsequent mesh update to take effect.
     */
    TextInstance* GetTextInstance();

    /**
     * @brief Indicates whether this object has animation support.
     *
     * @return Always false for TextObject.
     */
    [[nodiscard]] bool HasAnimation() const override { return false; }

    /**
     * @brief Returns the sprite animator attached to this object.
     *
     * @return Always nullptr for TextObject.
     */
    [[nodiscard]] SpriteAnimator* GetSpriteAnimator() const override { return nullptr; }

    /**
     * @brief Returns the world-space position of the text.
     *
     * @details
     * This is derived from the underlying Transform without additional offsets.
     */
    [[nodiscard]] glm::vec2 GetWorldPosition() const override;

    /**
     * @brief Returns the world-space scale of the text.
     */
    [[nodiscard]] glm::vec2 GetWorldScale() const override;

    /**
     * @brief Validates font atlas state and updates the text mesh if required.
     *
     * @details
     * If the font's atlas version differs from the last recorded version,
     * the mesh is regenerated to reflect newly baked glyphs.
     */
    void CheckFontAtlasAndMeshUpdate();

    void SetMaterial(const EngineContext& engineContext, const std::string& tag) = delete;
    void SetMaterial(Material* material_) = delete;
    [[nodiscard]] Material* GetMaterial() const = delete;

    void SetMesh(const EngineContext& engineContext, const std::string& tag) = delete;
    void SetMesh(Mesh* mesh_) = delete;
    [[nodiscard]] Mesh* GetMesh() const = delete;

    void AttachAnimator(std::unique_ptr<SpriteAnimator> anim) = delete;
    void AttachAnimator(SpriteSheet* sheet, float frameTime, bool loop = true) = delete;
    void DetachAnimator() = delete;

protected:
    /**
     * @brief Regenerates the internal mesh based on current text and alignment.
     *
     * @details
     * This function rebuilds vertex and index buffers according to glyph metrics
     * provided by the Font and applies horizontal and vertical alignment rules.
     */
    void UpdateMesh();

    TextAlignH alignH;
    TextAlignV alignV;

    TextInstance textInstance;
    std::unique_ptr<Mesh> textMesh;

    int textAtlasVersionTracker = 0;
};
