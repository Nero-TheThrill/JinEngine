#pragma once
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>

#include "Animation.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "Camera2D.h"
#include "Font.h"
#include "GameObject.h"
#include "InstanceBatchKey.h"
#include "RenderLayerManager.h"
#include "WindowManager.h"

struct TextInstance;
class SNAKE_Engine;
class StateManager;

using TextureTag = std::string;
using UniformName = std::string;
using FilePath = std::string;
using RenderCommand = std::function<void()>;

/**
 * @brief Batched draw map by (layer -> shader -> instance-key -> object list).
 *
 * @details
 * RenderMap organizes draw submission for optimal GPU usage:
 * 1) Sort by user-defined render layer (0..MAX_LAYERS-1),
 * 2) Group by bound Shader*,
 * 3) Group by InstanceBatchKey (material, mesh, textures, states),
 * 4) Iterate vectors of Object* and issue draws (instanced when possible).
 */
using ShaderMap = std::unordered_map<Shader*, std::map<InstanceBatchKey, std::vector<Object*>>>;
using RenderMap = std::array<std::map<int, ShaderMap>, RenderLayerManager::MAX_LAYERS>;

/**
 * @brief One line segment for debug rendering.
 */
struct LineInstance
{
    glm::vec2 from = { 0,0 };               ///< Line start position in world/screen depending on camera usage.
    glm::vec2 to = { 0,0 };                 ///< Line end position.
    glm::vec4 color = { 1,1,1,1 };          ///< RGBA color.
    float lineWidth = 1;                     ///< Pixel width.
};

/**
 * @brief Central rendering subsystem: resources, frame lifecycle, and draw submission.
 *
 * @details
 * RenderManager owns GPU resources (shaders, textures, meshes, materials, fonts, sprite sheets),
 * manages render layers, prepares the offscreen scene target (FBO) when enabled,
 * builds a batched render map, and flushes draw commands. It also exposes utilities such as
 * viewport/scissor clearing, debug line drawing, and compute-dispatch support.
 *
 * Typical per-frame flow:
 * @code
 * renderManager.BeginFrame(engineContext);
 * // ... game state's ObjectManager submits objects via StateManager -> RenderManager ...
 * renderManager.EndFrame(engineContext);
 * // After EndFrame, a GameState may run PostProcessing() using the scene color texture.
 * @endcode
 */
class RenderManager
{
    friend ObjectManager;
    friend StateManager;
    friend SNAKE_Engine;
public:
    /**
     * @brief Registers and compiles a shader program from multiple stage files.
     *
     * @param tag Unique string tag.
     * @param sources Vector of (stage, filepath) pairs.
     * @note Overwrites existing shader with the same tag.
     */
    void RegisterShader(const std::string& tag, const std::vector<std::pair<ShaderStage, FilePath>>& sources);

    /**
     * @brief Registers a pre-constructed shader program.
     *
     * @param tag Unique string tag.
     * @param shader Ownership transferred to the manager.
     */
    void RegisterShader(const std::string& tag, std::unique_ptr<Shader> shader);

    /**
     * @brief Registers a texture from file.
     *
     * @param tag Unique texture tag.
     * @param path Image file path.
     * @param settings Filtering/wrap/mipmap options.
     */
    void RegisterTexture(const std::string& tag, const FilePath& path, const TextureSettings& settings = {});

    /**
     * @brief Registers a pre-created texture object.
     *
     * @param tag Unique texture tag.
     * @param texture Ownership transferred to the manager.
     */
    void RegisterTexture(const std::string& tag, std::unique_ptr<Texture> texture);

    /**
     * @brief Registers a mesh from raw vertex/index data.
     *
     * @param tag Unique mesh tag.
     * @param vertices Vertex buffer.
     * @param indices Optional index buffer.
     * @param primitiveType Primitive topology for draw calls.
     */
    void RegisterMesh(const std::string& tag, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices = {}, PrimitiveType primitiveType = PrimitiveType::Triangles);

    /**
     * @brief Registers a pre-constructed mesh.
     *
     * @param tag Unique mesh tag.
     * @param mesh Ownership transferred to the manager.
     */
    void RegisterMesh(const std::string& tag, std::unique_ptr<Mesh> mesh);

    /**
     * @brief Registers a material from shader tag and texture bindings.
     *
     * @param tag Unique material tag.
     * @param shaderTag Tag of an existing shader.
     * @param textureBindings Map of uniform name -> texture tag.
     */
    void RegisterMaterial(const std::string& tag, const std::string& shaderTag, const std::unordered_map<UniformName, TextureTag>& textureBindings);

    /**
     * @brief Registers a pre-constructed material.
     *
     * @param tag Unique material tag.
     * @param material Ownership transferred to the manager.
     */
    void RegisterMaterial(const std::string& tag, std::unique_ptr<Material> material);

    /**
     * @brief Registers a font by loading a TTF at a pixel size.
     *
     * @param tag Unique font tag.
     * @param ttfPath TTF file path.
     * @param pixelSize Requested glyph size in pixels.
     */
    void RegisterFont(const std::string& tag, const std::string& ttfPath, uint32_t pixelSize);

    /**
     * @brief Registers a pre-constructed font.
     *
     * @param tag Unique font tag.
     * @param font Ownership transferred to the manager.
     */
    void RegisterFont(const std::string& tag, std::unique_ptr<Font> font);

    /**
     * @brief Maps a user tag to a numeric render layer (0..MAX_LAYERS-1).
     *
     * @param tag Human-readable layer tag.
     * @param layer Target layer index.
     */
    void RegisterRenderLayer(const std::string& tag, uint8_t layer);

    /**
     * @brief Registers a sprite sheet from a texture tag and frame size.
     *
     * @param tag Unique spritesheet tag.
     * @param textureTag Tag of texture to slice.
     * @param frameW Frame width in pixels.
     * @param frameH Frame height in pixels.
     */
    void RegisterSpriteSheet(const std::string& tag, const std::string& textureTag, int frameW, int frameH);

    /**
     * @brief Unregisters (and destroys) a shader by tag.
     *
     * @param tag Shader tag.
     * @param engineContext Context for safe GL teardown if required.
     */
    void UnregisterShader(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters (and destroys) a texture by tag.
     */
    void UnregisterTexture(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters (and destroys) a mesh by tag.
     */
    void UnregisterMesh(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters (and destroys) a material by tag.
     */
    void UnregisterMaterial(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters (and destroys) a font by tag.
     */
    void UnregisterFont(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Removes a layer mapping for a tag.
     */
    void UnregisterRenderLayer(const std::string& tag);

    /**
     * @brief Unregisters (and destroys) a sprite sheet by tag.
     */
    void UnregisterSpriteSheet(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Checks if a texture with tag exists.
     */
    [[nodiscard]] bool HasTexture(const std::string& tag) const;

    /**
     * @brief Checks if a shader with tag exists.
     */
    [[nodiscard]] bool HasShader(const std::string& tag) const;

    /**
     * @brief Checks if a font with tag exists.
     */
    [[nodiscard]] bool HasFont(const std::string& tag) const;

    /**
     * @brief Checks if a sprite sheet with tag exists.
     */
    [[nodiscard]] bool HasSpriteSheet(const std::string& tag) const;

    /**
     * @brief Looks up a shader by tag.
     *
     * @return Pointer or nullptr if not found.
     */
    [[nodiscard]] Shader* GetShaderByTag(const std::string& tag);

    /**
     * @brief Looks up a texture by tag.
     */
    [[nodiscard]] Texture* GetTextureByTag(const std::string& tag);

    /**
     * @brief Looks up a mesh by tag.
     */
    [[nodiscard]] Mesh* GetMeshByTag(const std::string& tag);

    /**
     * @brief Looks up a material by tag.
     */
    [[nodiscard]] Material* GetMaterialByTag(const std::string& tag);

    /**
     * @brief Looks up a font by tag.
     */
    [[nodiscard]] Font* GetFontByTag(const std::string& tag);

    /**
     * @brief Looks up a sprite sheet by tag.
     */
    [[nodiscard]] SpriteSheet* GetSpriteSheetByTag(const std::string& tag);

    /**
     * @brief Flushes all queued draw commands (render map + debug lines).
     *
     * @details
     * Iterates the RenderMap in layer order, binds shaders/materials once per batch,
     * performs instanced draws when supported, and issues non-instanced draws otherwise.
     * Finally flushes debug line batches.
     */
    void FlushDrawCommands(const EngineContext& engineContext);

    /**
     * @brief Sets the GL viewport rectangle.
     *
     * @param x Origin x (pixels).
     * @param y Origin y (pixels).
     * @param width Width (pixels).
     * @param height Height (pixels).
     */
    void SetViewport(int x, int y, int width, int height);

    /**
     * @brief Clears a rectangular region with a solid color.
     *
     * @details
     * Uses scissor + clear or a fast rect to fill (x,y,width,height) in the current target.
     */
    void ClearBackground(int x, int y, int width, int height, glm::vec4 color);

    /**
     * @brief Queues a debug line to be drawn with an optional camera.
     *
     * @param from Start position.
     * @param to End position.
     * @param camera If null, treated as screen-space; otherwise world-space with this camera.
     * @param color RGBA color.
     * @param lineWidth Pixel width.
     */
    void DrawDebugLine(const glm::vec2& from, const glm::vec2& to, Camera2D* camera = nullptr, const glm::vec4& color = { 1,1,1,1 }, float lineWidth = 1.0f);

    /**
     * @brief Returns the render-layer manager for name <-> index mapping.
     */
    [[nodiscard]] RenderLayerManager& GetRenderLayerManager();

    /**
     * @brief Begins a new frame: binds offscreen FBO (if enabled), clears, and prepares state.
     *
     * @details
     * Also rebuilds default resources on first use and re-creates the scene target
     * when the window size changes.
     */
    void BeginFrame(const EngineContext& engineContext);

    /**
     * @brief Ends the frame: resolves offscreen to screen and swaps buffers (via WindowManager).
     */
    void EndFrame(const EngineContext& engineContext);

    /**
     * @brief Dispatches a compute pass using the given ComputeMaterial.
     *
     * @details
     * Expects images/uniforms to be set on the material; computes group size from current RT.
     */
    void DispatchCompute(ComputeMaterial* material);

    /**
     * @brief Handles window resize: updates viewport and recreates the scene render target.
     */
    void OnResize(int width, int height);
private:
    /**
     * @brief Initializes GL state, default resources, and debug-line buffers.
     */
    void Init(const EngineContext& engineContext);

    /**
     * @brief Builds the batched RenderMap from a source object list for a given camera.
     *
     * @param source All objects to consider (visibility handled by caller if needed).
     * @param camera Camera used for transforms (nullptr for HUD/screen-space).
     * @note Objects are grouped by layer, then by shader and instance key.
     */
    void BuildRenderMap(const std::vector<Object*>& source, Camera2D* camera);

    /**
     * @brief Submits a list of objects into internal render structures.
     *
     * @details
     * Called by StateManager/ObjectManager to push frame’s visible objects.
     */
    void Submit(const std::vector<Object*>& objects, const EngineContext& engineContext);

    /**
     * @brief Flushes queued debug lines (batched by (camera*, width)) to GPU.
     */
    void FlushDebugLineDrawCommands(const EngineContext& engineContext);

    /**
     * @brief Destroys all GL resources owned by the RenderManager.
     */
    void Free();

    std::unordered_map<std::string, std::unique_ptr<Shader>> shaderMap;      ///< Tag -> Shader.
    std::unordered_map<std::string, std::unique_ptr<Texture>> textureMap;    ///< Tag -> Texture.
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshMap;          ///< Tag -> Mesh.
    std::unordered_map<std::string, std::unique_ptr<Material>> materialMap;  ///< Tag -> Material.
    std::unordered_map<std::string, std::unique_ptr<Font>> fontMap;          ///< Tag -> Font.
    std::unordered_map<std::string, std::unique_ptr<SpriteSheet>> spritesheetMap; ///< Tag -> SpriteSheet.

    using CameraAndWidth = std::pair<Camera2D*, float>;
    struct CameraAndWidthHash
    {
        std::size_t operator()(const CameraAndWidth& key) const
        {
            return std::hash<Camera2D*>()(key.first) ^ std::hash<float>()(key.second);
        }
    };
    std::unordered_map<CameraAndWidth, std::vector<LineInstance>, CameraAndWidthHash> debugLineMap; ///< Debug lines grouped per camera/width.
    GLuint debugLineVAO = 0, debugLineVBO = 0;                                                     ///< GL buffers for line rendering.

    Shader* defaultShader, * debugLineShader;   ///< Default textured shader, and debug-line shader.
    Material* defaultMaterial;                 ///< Default material bound for fallback rendering.
    SpriteSheet* defaultSpriteSheet;           ///< Default sprite sheet (for sprite rendering).
    Mesh* defaultMesh;                         ///< Default quad mesh for 2D.
    RenderMap renderMap;                       ///< Batched draw containers.
    RenderLayerManager renderLayerManager;     ///< Name<->index mapping for layers.

    Camera2D* renderCamera;                    ///< Camera used for current build/flush.

    Texture* errorTexture;                     ///< Placeholder texture for missing assets.

    unsigned int sceneFBO = 0;                 ///< Offscreen framebuffer (if useOffscreen).
    unsigned int sceneColor = 0;               ///< Offscreen color texture (scene result).
    int rtWidth = 0;
    int rtHeight = 0;
    bool useOffscreen = true;                  ///< Whether to render to scene FBO first.

    /**
     * @brief Creates or re-creates the offscreen scene target (FBO + color texture).
     */
    void CreateSceneTarget(int w, int h);

    /**
     * @brief Destroys the offscreen scene target.
     */
    void DestroySceneTarget();
};



/**
 * @brief View-frustum culling helper for 2D camera.
 *
 * @details
 * Removes objects that are outside the camera’s visible rectangle. Uses each
 * object’s bounding radius and transform to produce a conservative visibility test.
 */
class FrustumCuller
{
public:
    /**
     * @brief Filters visible objects against the 2D camera frustum.
     *
     * @param camera Reference camera.
     * @param allObjects All candidates to test.
     * @param outVisibleList Output list filled with visible objects.
     * @param viewportSize Size in pixels (width,height) used with camera zoom.
     * @note Out list is not cleared; the function appends visible objects.
     */
    static void CullVisible(const Camera2D& camera, const std::vector<Object*>& allObjects,
        std::vector<Object*>& outVisibleList, glm::vec2 viewportSize);
};
