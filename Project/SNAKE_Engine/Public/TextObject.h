#pragma once
#include "EngineContext.h"
#include "Mesh.h"
#include "Object.h"
#include "Transform.h"

class Camera2D;

/**
 * @brief Holds text rendering parameters for a text instance.
 *
 * @details
 * A lightweight bundle that pairs a font resource with the UTF-8 text
 * string to render. This is consumed by TextObject to build/update a
 * mesh of quads per-glyph using the engine's dynamic glyph atlas.
 */
struct TextInstance
{
    Font* font = nullptr;      ///< Font resource used to render glyphs. Must be valid while used by TextObject.
    std::string text = "";     ///< UTF-8 encoded text string to be rendered.
};

/**
 * @brief Scene object specialized for rendering 2D text.
 *
 * @details
 * TextObject produces a mesh from a UTF-8 string using the provided Font's
 * dynamic glyph atlas. It honors horizontal/vertical alignment, integrates
 * with the engine's camera rules (including HUD-style "ignore camera"), and
 * updates its mesh lazily when the text or font atlas changes.
 *
 * @note
 * Material/Mesh setters and animator APIs are intentionally deleted to prevent
 * misuse. TextObject internally owns its mesh and uses the Font-provided material.
 */
class TextObject : public Object
{
public:
    /**
     * @brief Constructs a TextObject with the given font and text.
     *
     * @details
     * Stores the font and UTF-8 text, alignment preferences, and immediately
     * builds the internal mesh via @ref UpdateMesh. The material comes from
     * the Font (internal text shader + atlas texture).
     *
     * @param font Font resource used to bake glyphs and supply material.
     * @param text UTF-8 text to render.
     * @param alignH Horizontal alignment (Left/Center/Right). Defaults to Left.
     * @param alignV Vertical alignment (Top/Middle/Bottom). Defaults to Top.
     * @code
     * auto* t = state->GetObjectManager().AddObject(
     *     std::make_unique<TextObject>(rm->GetFontByTag("[Font]ui"), "Hello", TextAlignH::Center, TextAlignV::Middle),
     *     "[Object]UIHello");
     * t->SetIgnoreCamera(true, state->GetActiveCamera()); // HUD text
     * @endcode
     */
    TextObject(Font* font, const std::string& text, TextAlignH alignH = TextAlignH::Left, TextAlignV alignV = TextAlignV::Top);

    /**
     * @brief Virtual destructor.
     */
    ~TextObject() override = default;

    /**
     * @brief Initializes object-side resources. No-op for TextObject.
     *
     * @details
     * TextObject builds its mesh in the constructor/UpdateMesh, so Init is empty.
     */
    void Init([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Late initialization hook. No-op for TextObject.
     */
    void LateInit([[maybe_unused]] const EngineContext& engineContext) override;
    
    /**
     * @brief Per-frame update. No-op for TextObject.
     *
     * @details
     * TextObject does not animate by itself. Use SetText/SetAlign* to trigger mesh rebuilds.
     */
    void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) override;
    
    /**
     * @brief Issue draw-time hooks. No-op; actual draw is driven by RenderManager.
     *
     * @details
     * RenderManager batches and draws using the internal mesh/material of TextObject.
     */
    void Draw([[maybe_unused]] const EngineContext& engineContext) override;
    
    /**
     * @brief Frees resources before removal. No-op (unique_ptr handles mesh).
     */
    void Free([[maybe_unused]] const EngineContext& engineContext) override;
    
    /**
     * @brief Late free hook. No-op for TextObject.
     */
    void LateFree([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Approximates a bounding radius in world space for culling.
     *
     * @details
     * Computes the text's pixel size via Font::GetTextSize(text), scales by
     * Transform2D scale, and returns the length as a conservative radius.
     *
     * @return Bounding radius of this text in world space units.
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Replaces the current UTF-8 text and rebuilds the mesh if changed.
     *
     * @param text New UTF-8 string. No-op if identical to the current value.
     * @note
     * Triggers @ref UpdateMesh on change. Also affects alignment and radius.
     */
    void SetText(const std::string& text);

    /**
     * @brief Replaces both font and text in a single operation.
     *
     * @details
     * Useful when changing the font (e.g., size or style) together with text.
     * Rebuilds the internal mesh if either value changes.
     *
     * @param textInstance_ New text instance (font + text).
     */
    void SetTextInstance(const TextInstance& textInstance_);

    /**
     * @brief Sets horizontal alignment and rebuilds the mesh if changed.
     *
     * @param alignH_ New horizontal alignment mode.
     */
    void SetAlignH(TextAlignH alignH_);

    /**
     * @brief Sets vertical alignment and rebuilds the mesh if changed.
     *
     * @param alignV_ New vertical alignment mode.
     */
    void SetAlignV(TextAlignV alignV_);

    /**
     * @brief Returns a modifiable pointer to the current TextInstance.
     *
     * @return Pointer to the internal TextInstance.
     * @warning
     * Mutating the returned data does not auto-rebuild the mesh. Call @ref SetText
     * or @ref SetTextInstance (or @ref UpdateMesh) after modifications to reflect changes.
     */
    TextInstance* GetTextInstance();

    /**
     * @brief TextObject does not support sprite animation.
     *
     * @return Always false.
     */
    [[nodiscard]] bool HasAnimation() const override { return false; }

    /**
     * @brief TextObject does not provide a SpriteAnimator.
     *
     * @return Always nullptr.
     */
    [[nodiscard]] SpriteAnimator* GetSpriteAnimator() const override { return nullptr; }

    /**
     * @brief Returns the world-space anchor position, honoring alignment and HUD mode.
     *
     * @details
     * Computes an offset based on alignment and text size, then:
     * - If ShouldIgnoreCamera() and referenceCamera are set: converts from screen space
     *   (HUD) to world space by applying camera position/zoom.
     * - Otherwise, returns Transform2D position (already in world space).
     *
     * @return World-space position used for culling and debug.
     */
    [[nodiscard]] glm::vec2 GetWorldPosition() const override;

    /**
     * @brief Returns the world-space scaled size of the text quad area.
     *
     * @details
     * Uses Font::GetTextSize(text) and Transform2D scale. In HUD mode
     * (ignore camera), also divides by reference camera zoom.
     *
     * @return World-space 2D extent derived from text size and scale.
     */
    [[nodiscard]] glm::vec2 GetWorldScale() const override;

    /**
     * @brief Rebuilds the mesh on-demand when the font atlas changes.
     *
     * @details
     * Compares an internal atlas version tracker to Font::GetTextAtlasVersion().
     * If different, regenerates the mesh via Font::GenerateTextMesh().
     * This keeps glyph UVs valid when the dynamic atlas expands.
     */
    void CheckFontAtlasAndMeshUpdate();

    /**
     * @brief Deleted: TextObject manages its own material through Font.
     *
     * @details
     * Prevents external material override; text rendering requires a specific shader/atlas.
     */
    void SetMaterial(const EngineContext& engineContext, const std::string& tag) = delete;

    /**
     * @brief Deleted: TextObject manages its own material through Font.
     */
    void SetMaterial(Material* material_) = delete;

    /**
     * @brief Deleted: Material is internal; use TextInstance::font to affect visuals.
     */
    [[nodiscard]] Material* GetMaterial() const = delete;

    /**
     * @brief Deleted: TextObject owns and rebuilds its mesh automatically.
     */
    void SetMesh(const EngineContext& engineContext, const std::string& tag) = delete;

    /**
     * @brief Deleted: TextObject owns and rebuilds its mesh automatically.
     */
    void SetMesh(Mesh* mesh_) = delete;

    /**
     * @brief Deleted: TextObject is not sprite-animated.
     */
    [[nodiscard]] Mesh* GetMesh() const = delete;

    /**
     * @brief Deleted: TextObject does not support attaching sprite animators.
     */
    void AttachAnimator(std::unique_ptr<SpriteAnimator> anim) = delete;

    /**
     * @brief Deleted: TextObject does not support attaching sprite animators.
     */
    void AttachAnimator(SpriteSheet* sheet, float frameTime, bool loop = true) = delete;

    /**
     * @brief Deleted: TextObject does not support sprite animators.
     */
    void DetachAnimator() = delete;
protected:
    /**
     * @brief Rebuilds the internal mesh from current text/alignment.
     *
     * @details
     * Calls Font::GenerateTextMesh(text, alignH, alignV) and stores the
     * result in @ref textMesh, updating the raw @ref mesh pointer inherited from Object.
     * Invoked by setters and the constructor to keep geometry in sync.
     */
    void UpdateMesh();

    TextAlignH alignH;                 ///< Horizontal alignment mode.
    TextAlignV alignV;                 ///< Vertical alignment mode.

    TextInstance textInstance;         ///< Current font+text bundle used to build the mesh.
    std::unique_ptr<Mesh> textMesh;    ///< Owned mesh generated from glyph quads.

    int textAtlasVersionTracker = 0;   ///< Tracks Font atlas version to trigger on-demand rebuilds.
};
