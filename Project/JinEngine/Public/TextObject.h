#pragma once
#include "EngineContext.h"
#include "Mesh.h"
#include "Object.h"
#include "Transform.h"

class Camera2D;

/**
 * @brief Stores the font reference and UTF-8 text content used by a TextObject.
 *
 * @details
 * TextInstance is a lightweight data container that groups the minimum information
 * required to generate and refresh a text mesh:
 * - the font resource used for glyph lookup and atlas access,
 * - the actual text string to render.
 *
 * The font is stored as a weak_ptr so that TextObject does not force ownership of
 * the font resource. Because of that, TextObject must lock the weak_ptr before using it,
 * and safely handles the case where the font has expired.
 */
struct TextInstance
{
    std::weak_ptr<Font> font;
    std::string text = "";
};

/**
 * @brief Represents a renderable text object whose mesh is generated from a Font at runtime.
 *
 * @details
 * TextObject is a specialized Object implementation for rendering text in the world or in
 * screen-aligned space. Unlike a regular sprite/object pipeline, this class does not allow
 * arbitrary mesh or material replacement through the usual Object interface.
 *
 * Instead, TextObject builds and owns its render mesh from:
 * - the current Font,
 * - the current text string,
 * - the current horizontal and vertical alignment settings.
 *
 * The constructor immediately generates an initial text mesh through UpdateMesh().
 * Later, the mesh is regenerated whenever the text content, alignment, or font changes.
 *
 * In addition, TextObject supports automatic mesh refresh when the backing font atlas changes.
 * This is handled through CheckFontAtlasAndMeshUpdate(), which compares a tracked atlas version
 * against the current font atlas version and rebuilds the mesh only when necessary.
 *
 * ObjectManager explicitly calls CheckFontAtlasAndMeshUpdate() during update and when pending
 * objects are first initialized, so TextObject can react to lazy glyph baking or atlas growth
 * without requiring manual refresh from user code. :contentReference[oaicite:5]{index=5}
 *
 * The world position and world scale returned by this class are not simple transform passthroughs.
 * They incorporate:
 * - the current text size reported by the font,
 * - alignment offsets,
 * - optional camera-ignore behavior using the reference camera.
 *
 * @note
 * Init(), LateInit(), Update(), Draw(), Free(), and LateFree() are currently empty in the present implementation.
 *
 * @note
 * Material, mesh, and sprite animation reassignment APIs are intentionally deleted so that
 * TextObject always stays consistent with the font-driven text rendering pipeline.
 */
class TextObject : public Object
{
public:
    /**
     * @brief Constructs a TextObject from a font, text string, and alignment settings.
     *
     * @details
     * This constructor:
     * - initializes the base Object as ObjectType::TEXT,
     * - stores the horizontal and vertical alignment values,
     * - stores the provided font and text into textInstance,
     * - retrieves the font's material and assigns it to the inherited material slot,
     * - immediately generates the initial text mesh by calling UpdateMesh().
     *
     * @param font Shared font resource used to generate glyph geometry and provide the text material.
     * @param text Initial UTF-8 text string to render.
     * @param alignH Initial horizontal alignment used for mesh generation and position offset calculation.
     * @param alignV Initial vertical alignment used for mesh generation and position offset calculation.
     *
     * @note
     * The mesh is built immediately during construction, so a valid font should be provided.
     */
    TextObject(std::shared_ptr<Font> font, const std::string& text, TextAlignH alignH = TextAlignH::Left, TextAlignV alignV = TextAlignV::Top);

    /**
     * @brief Destroys the TextObject.
     *
     * @details
     * The default destructor is sufficient because owned resources are managed through smart pointers.
     * The generated text mesh is released automatically when the object is destroyed.
     */
    ~TextObject() override = default;

    /**
     * @brief Initializes the object during the first object initialization phase.
     *
     * @details
     * The current implementation performs no additional work here.
     * TextObject prepares its essential rendering state during construction instead.
     *
     * @param engineContext Shared engine systems available during initialization.
     */
    void Init([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Runs the second initialization phase after Init().
     *
     * @details
     * The current implementation performs no additional work here.
     *
     * @param engineContext Shared engine systems available during late initialization.
     */
    void LateInit([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Updates the TextObject for the current frame.
     *
     * @details
     * The current implementation performs no per-frame logic here.
     * Font atlas synchronization is handled externally by ObjectManager through
     * CheckFontAtlasAndMeshUpdate().
     *
     * @param dt Delta time of the current frame.
     * @param engineContext Shared engine systems available during update.
     */
    void Update([[maybe_unused]] float dt, [[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Submits per-draw state for the text object.
     *
     * @details
     * The current implementation does not perform any additional draw-step logic here.
     * Rendering behavior is expected to be handled by the engine's render submission pipeline
     * using the generated text mesh and inherited object state.
     *
     * @param engineContext Shared engine systems available during drawing.
     */
    void Draw([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Frees resources during the first destruction phase.
     *
     * @details
     * The current implementation performs no additional work here.
     *
     * @param engineContext Shared engine systems available during free.
     */
    void Free([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Runs the second destruction phase after Free().
     *
     * @details
     * The current implementation performs no additional work here.
     *
     * @param engineContext Shared engine systems available during late free.
     */
    void LateFree([[maybe_unused]] const EngineContext& engineContext) override;

    /**
     * @brief Returns an approximate bounding radius for the current rendered text.
     *
     * @details
     * This function first checks whether the current mesh and font are still valid.
     * If either resource is unavailable, it returns 0.
     *
     * Otherwise, it:
     * - queries the current text size from the font,
     * - multiplies that size by the current Transform2D scale,
     * - returns the vector length of the scaled size as a simple bounding radius approximation.
     *
     * This is not a tightly fitted collision radius. It is a convenient size estimate based on
     * rendered text dimensions.
     *
     * @return Approximate bounding radius of the current text.
     * @return 0.0f if the mesh or font is unavailable.
     */
    [[nodiscard]] float GetBoundingRadius() const override;

    /**
     * @brief Replaces the current text string and rebuilds the mesh if the content changed.
     *
     * @details
     * If the new text is identical to the current text, the function returns early.
     * Otherwise, it updates textInstance.text and calls UpdateMesh() so that the geometry
     * matches the new string.
     *
     * @param text New UTF-8 text string to render.
     *
     * @note
     * This triggers immediate mesh regeneration when the value changes.
     */
    void SetText(const std::string& text);

    /**
     * @brief Replaces the full text instance and rebuilds the mesh when needed.
     *
     * @details
     * This function compares:
     * - the current text string against the incoming text string,
     * - the currently locked font pointer against the incoming locked font pointer.
     *
     * If both are effectively unchanged, the function returns early.
     * Otherwise, it copies the new TextInstance and regenerates the mesh by calling UpdateMesh().
     *
     * @param textInstance_ New text instance containing a font reference and text content.
     */
    void SetTextInstance(const TextInstance& textInstance_);

    /**
     * @brief Changes the horizontal alignment and rebuilds the mesh if the value changed.
     *
     * @details
     * If the new alignment equals the current one, the function returns immediately.
     * Otherwise, it stores the new alignment and regenerates the mesh.
     *
     * @param alignH_ New horizontal alignment.
     */
    void SetAlignH(TextAlignH alignH_);

    /**
     * @brief Changes the vertical alignment and rebuilds the mesh if the value changed.
     *
     * @details
     * If the new alignment equals the current one, the function returns immediately.
     * Otherwise, it stores the new alignment and regenerates the mesh.
     *
     * @param alignV_ New vertical alignment.
     */
    void SetAlignV(TextAlignV alignV_);

    /**
     * @brief Returns direct access to the internal text instance.
     *
     * @details
     * This function exposes the internal TextInstance storage used by this object.
     * It allows engine-side systems to inspect or modify the stored font/text data directly.
     *
     * @return Pointer to the internal TextInstance.
     *
     * @note
     * Because this returns mutable access, callers should be careful to rebuild the mesh
     * afterward if they manually change the contents.
     */
    TextInstance* GetTextInstance();

    /**
     * @brief Indicates whether this object supports sprite animation.
     *
     * @return Always false for TextObject.
     *
     * @note
     * TextObject is a font-driven render object and does not use SpriteAnimator.
     */
    [[nodiscard]] bool HasAnimation() const override { return false; }

    /**
     * @brief Returns the sprite animator attached to this object.
     *
     * @return Always nullptr for TextObject.
     *
     * @note
     * TextObject does not support sprite animation attachment.
     */
    [[nodiscard]] SpriteAnimator* GetSpriteAnimator() const override { return nullptr; }

    /**
     * @brief Returns the effective world position used for text rendering.
     *
     * @details
     * This function starts from the Transform2D position and then applies an alignment offset
     * based on the current text size and alignment settings.
     *
     * Alignment behavior:
     * - Center/Middle applies no half-size offset.
     * - Left and Right shift horizontally by half the text width.
     * - Top and Bottom shift vertically by half the text height.
     *
     * After the alignment offset is computed:
     * - if the object ignores the camera and a reference camera exists, the function converts the
     *   screen-space style position back into world-aligned coordinates using the reference camera's
     *   position and zoom,
     * - otherwise it returns the aligned position directly.
     *
     * @return Effective world position for rendering or visibility calculations.
     */
    [[nodiscard]] glm::vec2 GetWorldPosition() const override;

    /**
     * @brief Returns the effective world scale for the current text.
     *
     * @details
     * This function first locks the current font.
     * If the font is unavailable, it returns a zero vector.
     *
     * Otherwise, it obtains the current text size from the font and multiplies it by the object's
     * Transform2D scale. If camera-ignore rendering is enabled and a reference camera exists,
     * the result is additionally divided by the camera zoom.
     *
     * This means the returned scale reflects the actual text dimensions, not just the raw transform scale.
     *
     * @return Effective world scale for the rendered text.
     * @return glm::vec2(0.0f) if the font is unavailable.
     */
    [[nodiscard]] glm::vec2 GetWorldScale() const override;

    /**
     * @brief Regenerates the text mesh only when the font atlas version has changed.
     *
     * @details
     * This function is designed for dynamic/lazy font atlas systems.
     * It safely locks the current font and:
     *
     * - if the font has expired:
     *   - clears the inherited mesh reference,
     *   - clears the owned text mesh,
     *   - resets the tracked atlas version to 0,
     *   - returns immediately.
     *
     * - if the tracked atlas version matches the font's current atlas version:
     *   - returns without rebuilding the mesh.
     *
     * - otherwise:
     *   - updates textAtlasVersionTracker,
     *   - regenerates the text mesh from the font,
     *   - stores the new mesh in both the inherited mesh handle and the owned textMesh.
     *
     * This function is called automatically by ObjectManager during update and during pending-object
     * initialization so that text objects stay synchronized with font atlas growth. :contentReference[oaicite:6]{index=6}
     */
    void CheckFontAtlasAndMeshUpdate();

    /**
     * @brief Disables tag-based material assignment for TextObject.
     *
     * @details
     * TextObject must derive its material from the associated Font, so replacing the material
     * through the generic Object API is intentionally forbidden.
     *
     * @param engineContext Unused.
     * @param tag Unused.
     */
    void SetMaterial(const EngineContext& engineContext, const std::string& tag) = delete;

    /**
     * @brief Disables direct material assignment for TextObject.
     *
     * @details
     * TextObject must keep its material consistent with the current Font resource.
     *
     * @param material_ Unused.
     */
    void SetMaterial(std::shared_ptr<Material> material_) = delete;

    /**
     * @brief Disables direct material retrieval through the TextObject-specific interface.
     *
     * @return Deleted function.
     */
    [[nodiscard]] std::shared_ptr<Material> GetMaterial() const = delete;

    /**
     * @brief Disables tag-based mesh assignment for TextObject.
     *
     * @details
     * The mesh of a TextObject is generated from font glyph geometry and text content,
     * so replacing it through the generic Object API is intentionally forbidden.
     *
     * @param engineContext Unused.
     * @param tag Unused.
     */
    void SetMesh(const EngineContext& engineContext, const std::string& tag) = delete;

    /**
     * @brief Disables direct mesh assignment for TextObject.
     *
     * @details
     * The mesh must remain synchronized with the font, text, and alignment settings.
     *
     * @param mesh_ Unused.
     */
    void SetMesh(std::shared_ptr<Mesh> mesh_) = delete;

    /**
     * @brief Disables direct mesh retrieval through the TextObject-specific interface.
     *
     * @return Deleted function.
     */
    [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const = delete;

    /**
     * @brief Disables animator attachment through a unique_ptr interface.
     *
     * @param anim Unused.
     */
    void AttachAnimator(std::unique_ptr<SpriteAnimator> anim) = delete;

    /**
     * @brief Disables animator attachment through a sprite-sheet helper interface.
     *
     * @param sheet Unused.
     * @param frameTime Unused.
     * @param loop Unused.
     */
    void AttachAnimator(SpriteSheet* sheet, float frameTime, bool loop = true) = delete;

    /**
     * @brief Disables animator detachment because TextObject never owns a sprite animator.
     */
    void DetachAnimator() = delete;
protected:

    /**
     * @brief Regenerates the text mesh immediately from the current font, text, and alignment.
     *
     * @details
     * This function safely locks the current font and:
     *
     * - if the font has expired:
     *   - clears the inherited mesh reference,
     *   - clears the owned text mesh,
     *   - resets the tracked atlas version to 0,
     *   - returns immediately.
     *
     * - otherwise:
     *   - generates a new text mesh from the font,
     *   - stores it in both the inherited mesh handle and the owned textMesh,
     *   - updates textAtlasVersionTracker to the font's current atlas version.
     *
     * Unlike CheckFontAtlasAndMeshUpdate(), this function does not compare versions first.
     * It always rebuilds when called and is used after explicit content/alignment/font changes.
     */
    void UpdateMesh();

    /**
     * @brief Current horizontal alignment used for mesh generation and positional offset calculation.
     */
    TextAlignH alignH;

    /**
     * @brief Current vertical alignment used for mesh generation and positional offset calculation.
     */
    TextAlignV alignV;

    /**
     * @brief Stored font reference and text string used to generate the text mesh.
     */
    TextInstance textInstance;

    /**
     * @brief Strong ownership of the currently generated text mesh.
     *
     * @details
     * The inherited mesh handle is updated to reference the same mesh so that the normal
     * render submission pipeline can use it.
     */
    std::shared_ptr <Mesh> textMesh;

    /**
     * @brief Tracks the font atlas version used when the current mesh was last generated.
     *
     * @details
     * This value is used by CheckFontAtlasAndMeshUpdate() to skip unnecessary mesh rebuilds
     * until the font atlas changes.
     */
    int textAtlasVersionTracker = 0;
};