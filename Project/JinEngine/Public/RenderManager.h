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
class JinEngine;
class StateManager;

using TextureTag = std::string;
using UniformName = std::string;
using FilePath = std::string;
using RenderCommand = std::function<void()>;

/**
 * @brief Maps a Shader pointer to depth-sorted instance batches.
 *
 * @details
 * The outer key is the Shader used for rendering.
 * The value is a depth-ordered map (std::map) whose key is InstanceBatchKey, and whose value is a list of Object*.
 *
 * This structure is produced by BuildRenderMap() and consumed by FlushDrawCommands().
 */
using ShaderMap = std::unordered_map<Shader*, std::map<InstanceBatchKey, std::vector<Object*>>>;

/**
 * @brief Layered render map used to batch and sort objects for rendering.
 *
 * @details
 * RenderMap is an array with RenderLayerManager::MAX_LAYERS entries.
 * Each layer contains a depth-sorted map:
 * - std::map<int, ShaderMap> where the int key is a quantized depth bin derived from Transform2D::GetDepth().
 *
 * The render pipeline order is:
 * 1) layer (0..MAX_LAYERS-1)
 * 2) depth bin (ascending int)
 * 3) shader
 * 4) InstanceBatchKey (mesh/material/spritesheet grouping)
 * 5) objects in the batch
 */
using RenderMap = std::array<std::map<int, ShaderMap>, RenderLayerManager::MAX_LAYERS>;

/**
 * @brief Parameters describing a single debug line draw request.
 *
 * @details
 * Debug lines are accumulated via DrawDebugLine() into an internal map keyed by (Camera2D*, lineWidth),
 * then emitted in a single flush pass by FlushDebugLineDrawCommands().
 */
struct LineInstance
{
    glm::vec2 from = { 0,0 };
    glm::vec2 to = { 0,0 };
    glm::vec4 color = { 1,1,1,1 };
    float lineWidth = 1;
};

/**
 * @brief Central rendering system responsible for resource registration and batched object rendering.
 *
 * @details
 * RenderManager serves two primary roles:
 * 1) Resource registry (tag -> Shader/Texture/Mesh/Material/Font/SpriteSheet).
 * 2) Per-frame rendering pipeline for Objects, including visibility culling, render sorting, batching, and drawing.
 *
 * Rendering flow (driven by JinEngine / StateManager / ObjectManager):
 * - BeginFrame(engineContext): optionally binds an offscreen framebuffer (sceneFBO) and clears it.
 * - Submit(objects, engineContext): frustum-culls objects using the active camera and builds a RenderMap.
 * - FlushDrawCommands(engineContext): iterates RenderMap and issues draw calls (instanced when possible).
 * - FlushDebugLineDrawCommands(engineContext): draws accumulated debug line geometry.
 * - EndFrame(engineContext): if offscreen is enabled, blits the scene target to the default framebuffer.
 *
 * Offscreen rendering:
 * - When useOffscreen is true, the manager renders into sceneFBO with a floating-point color target (GL_RGBA16F).
 * - The scene target is recreated when the window size changes (BeginFrame and OnResize).
 * - The offscreen color texture is force-registered under tag "[EngineTexture]RenderTexture".
 *
 * Batching & sorting:
 * - BuildRenderMap() groups visible objects into RenderMap[layer][zbin][shader][InstanceBatchKey] buckets.
 * - depth is quantized into an integer bin (depth * 1000, rounded) to provide stable depth ordering while still
 *   using an ordered map for deterministic iteration.
 *
 * Instancing:
 * - If the first object in a batch reports CanBeInstanced(), the entire batch is drawn using Mesh::DrawInstanced().
 * - Per-instance data is built on CPU (model matrices, colors, UV offset/scale) and uploaded via Mesh::UpdateInstanceBuffer().
 *
 * Safety for unregistering:
 * - Unregister* functions validate that no live Object references the target resource before erasing it.
 * - If references exist, the resource is not deleted and a warning is logged.
 */
class RenderManager
{
    friend ObjectManager;
    friend StateManager;
    friend JinEngine;
public:
    /**
     * @brief Registers a Shader program under a tag by compiling sources from files.
     *
     * @details
     * The shader is created, each (stage, filepath) pair is compiled/attached, then the program is linked.
     * If the tag already exists, registration is skipped.
     * If compilation or linking fails, registration is aborted.
     *
     * @param tag     Unique shader tag.
     * @param sources List of shader stage + file path pairs.
     */
    void RegisterShader(const std::string& tag, const std::vector<std::pair<ShaderStage, FilePath>>& sources);

    /**
     * @brief Registers an already-created Shader object under a tag.
     *
     * @details
     * Ownership of the shader is transferred into the internal shaderMap.
     * If the tag already exists, registration is skipped.
     *
     * @param tag    Unique shader tag.
     * @param shader Shader instance to store (ownership transferred).
     */
    void RegisterShader(const std::string& tag, std::unique_ptr<Shader> shader);

    /**
     * @brief Registers a Texture under a tag by loading it from a file path.
     *
     * @details
     * If the tag already exists, registration is skipped.
     *
     * @param tag      Unique texture tag.
     * @param path     Image file path.
     * @param settings Texture creation settings (filtering, wrapping, mipmaps).
     */
    void RegisterTexture(const std::string& tag, const FilePath& path, const TextureSettings& settings = {});

    /**
     * @brief Registers an already-created Texture object under a tag.
     *
     * @details
     * Ownership of the texture is transferred into the internal textureMap.
     * If the tag already exists, registration is skipped.
     *
     * @param tag     Unique texture tag.
     * @param texture Texture instance to store (ownership transferred).
     */
    void RegisterTexture(const std::string& tag, std::unique_ptr<Texture> texture);

    /**
     * @brief Registers or updates a Texture entry by wrapping an existing OpenGL texture ID.
     *
     * @details
     * If a texture with the same tag already exists, its underlying metadata is forcibly updated to
     * reference the given OpenGL texture ID via Texture::ForceUpdateTexture(id_, ...).
     * Otherwise, a new Texture wrapper is created and stored.
     *
     * This is used internally for engine-owned render targets (e.g., "[EngineTexture]RenderTexture").
     *
     * @param tag      Unique texture tag.
     * @param id_      OpenGL texture ID to wrap.
     * @param width_   Width in pixels.
     * @param height_  Height in pixels.
     * @param channels Number of channels.
     */
    void ForceRegisterTexture(const std::string& tag, unsigned int id_, int width_, int height_, int channels_);

    /**
     * @brief Registers or updates a Texture entry by uploading raw pixel data.
     *
     * @details
     * If a texture with the same tag already exists, its contents are forcibly replaced via
     * Texture::ForceUpdateTexture(data, ...) using the supplied settings.
     * Otherwise, a new Texture is created from the data and stored.
     *
     * @param tag      Unique texture tag.
     * @param data     Pointer to pixel data.
     * @param width_   Width in pixels.
     * @param height_  Height in pixels.
     * @param channels Number of channels.
     * @param settings Texture creation settings.
     */
    void ForceRegisterTexture(const std::string& tag, const unsigned char* data, int width_, int height_, int channels_, const TextureSettings& settings = {});

    /**
     * @brief Registers a Mesh under a tag by constructing it from vertices/indices.
     *
     * @details
     * If the tag already exists, registration is skipped.
     *
     * @param tag           Unique mesh tag.
     * @param vertices      Vertex buffer data.
     * @param indices       Optional index buffer data.
     * @param primitiveType Primitive topology for the mesh.
     */
    void RegisterMesh(const std::string& tag, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices = {}, PrimitiveType primitiveType = PrimitiveType::Triangles);

    /**
     * @brief Registers an already-created Mesh object under a tag.
     *
     * @details
     * Ownership of the mesh is transferred into the internal meshMap.
     * If the tag already exists, registration is skipped.
     *
     * @param tag  Unique mesh tag.
     * @param mesh Mesh instance to store (ownership transferred).
     */
    void RegisterMesh(const std::string& tag, std::unique_ptr<Mesh> mesh);

    /**
     * @brief Registers a Material under a tag by binding textures to uniforms using tags.
     *
     * @details
     * A Material is created using the Shader referenced by shaderTag, then textureBindings are resolved:
     * - key: uniform name
     * - value: texture tag
     *
     * Missing textures are warned and skipped. If the material tag already exists, registration is skipped.
     *
     * @param tag             Unique material tag.
     * @param shaderTag       Tag of a previously registered Shader.
     * @param textureBindings Uniform-to-texture tag bindings.
     */
    void RegisterMaterial(const std::string& tag, const std::string& shaderTag, const std::unordered_map<UniformName, TextureTag>& textureBindings);

    /**
     * @brief Registers an already-created Material object under a tag.
     *
     * @details
     * Ownership of the material is transferred into the internal materialMap.
     * If the tag already exists, registration is skipped.
     *
     * @param tag      Unique material tag.
     * @param material Material instance to store (ownership transferred).
     */
    void RegisterMaterial(const std::string& tag, std::unique_ptr<Material> material);

    /**
     * @brief Registers a Font under a tag by loading a TTF file at a given pixel size.
     *
     * @details
     * Font pixel size is validated against a fixed range (4..64). If out of range, registration fails.
     * On success, a Font is constructed as Font(*this, ttfPath, pixelSize) and stored.
     *
     * @param tag       Unique font tag.
     * @param ttfPath   TTF font file path.
     * @param pixelSize Glyph pixel size (must be within the allowed range).
     */
    void RegisterFont(const std::string& tag, const std::string& ttfPath, uint32_t pixelSize);

    /**
     * @brief Registers an already-created Font object under a tag.
     *
     * @details
     * Ownership of the font is transferred into the internal fontMap.
     * If the tag already exists, registration is skipped.
     *
     * @param tag  Unique font tag.
     * @param font Font instance to store (ownership transferred).
     */
    void RegisterFont(const std::string& tag, std::unique_ptr<Font> font);

    /**
     * @brief Registers a render layer tag to a numeric layer index.
     *
     * @details
     * Delegates to RenderLayerManager::RegisterLayer(tag, layer).
     *
     * @param tag   Render layer name/tag used by objects.
     * @param layer Numeric layer index (0..RenderLayerManager::MAX_LAYERS-1).
     */
    void RegisterRenderLayer(const std::string& tag, uint8_t layer);

    /**
     * @brief Registers a SpriteSheet under a tag using a previously registered texture.
     *
     * @details
     * The texture is resolved by textureTag. If not found, registration fails.
     * If the spritesheet tag already exists, registration is skipped.
     *
     * @param tag        Unique spritesheet tag.
     * @param textureTag Tag of an existing Texture to use.
     * @param frameW     Frame width (pixels) for grid-based spritesheet UV slicing.
     * @param frameH     Frame height (pixels) for grid-based spritesheet UV slicing.
     */
    void RegisterSpriteSheet(const std::string& tag, const std::string& textureTag, int frameW, int frameH);

    /**
     * @brief Unregisters a Shader if no live Object references it.
     *
     * @details
     * Scans all objects in the current GameState and checks their materials for shader references.
     * If any object is still referencing the shader, the shader is not removed.
     *
     * @param tag           Shader tag to remove.
     * @param engineContext Engine context used to access the current state and objects.
     */
    void UnregisterShader(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a Texture if no live Object references it.
     *
     * @details
     * Scans all objects in the current GameState and checks:
     * - material texture bindings
     * - sprite animator spritesheet texture reference
     * If any object is still referencing the texture, it is not removed.
     *
     * @param tag           Texture tag to remove.
     * @param engineContext Engine context used to access the current state and objects.
     */
    void UnregisterTexture(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a Mesh if no live Object references it.
     *
     * @details
     * Scans all objects in the current GameState and verifies no object has GetMesh() == target.
     *
     * @param tag           Mesh tag to remove.
     * @param engineContext Engine context used to access the current state and objects.
     */
    void UnregisterMesh(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a Material if no live Object references it.
     *
     * @details
     * Scans all objects in the current GameState and verifies no object has GetMaterial() == target.
     *
     * @param tag           Material tag to remove.
     * @param engineContext Engine context used to access the current state and objects.
     */
    void UnregisterMaterial(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a Font if no live TextObject references it.
     *
     * @details
     * Scans all objects in the current GameState; for objects with ObjectType::TEXT, it checks
     * dynamic_cast<TextObject*>(obj)->GetTextInstance()->font against the target font pointer.
     *
     * @param tag           Font tag to remove.
     * @param engineContext Engine context used to access the current state and objects.
     */
    void UnregisterFont(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a render layer tag mapping.
     *
     * @details
     * Delegates to RenderLayerManager::UnregisterLayer(tag).
     *
     * @param tag Render layer tag to remove.
     */
    void UnregisterRenderLayer(const std::string& tag);

    /**
     * @brief Unregisters a SpriteSheet if no live Object references it.
     *
     * @details
     * Scans all objects in the current GameState and checks sprite animators:
     * if any animator's GetSpriteSheet() equals the target, it is not removed.
     *
     * @param tag           SpriteSheet tag to remove.
     * @param engineContext Engine context used to access the current state and objects.
     */
    void UnregisterSpriteSheet(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Returns whether a Texture with the given tag is registered.
     */
    [[nodiscard]] bool HasTexture(const std::string& tag) const;

    /**
     * @brief Returns whether a Shader with the given tag is registered.
     */
    [[nodiscard]] bool HasShader(const std::string& tag) const;

    /**
     * @brief Returns whether a Font with the given tag is registered.
     */
    [[nodiscard]] bool HasFont(const std::string& tag) const;

    /**
     * @brief Returns whether a SpriteSheet with the given tag is registered.
     */
    [[nodiscard]] bool HasSpriteSheet(const std::string& tag) const;

    /**
     * @brief Returns the Shader registered under the given tag.
     *
     * @details
     * If the tag is not found, an error is logged and a default shader pointer is returned.
     *
     * @param tag Shader tag.
     * @return Shader pointer (fallback to default if missing).
     */
    [[nodiscard]] Shader* GetShaderByTag(const std::string& tag);

    /**
     * @brief Returns the Texture registered under the given tag.
     *
     * @details
     * If the tag is not found, an error is logged and the engine errorTexture is returned.
     *
     * @param tag Texture tag.
     * @return Texture pointer (fallback to errorTexture if missing).
     */
    [[nodiscard]] Texture* GetTextureByTag(const std::string& tag);

    /**
     * @brief Returns the Mesh registered under the given tag.
     *
     * @details
     * If the tag is not found, an error is logged and the engine default mesh pointer is returned.
     *
     * @param tag Mesh tag.
     * @return Mesh pointer (fallback to defaultMesh if missing).
     */
    [[nodiscard]] Mesh* GetMeshByTag(const std::string& tag);

    /**
     * @brief Returns the Material registered under the given tag.
     *
     * @details
     * If the tag is not found, an error is logged and the engine default material pointer is returned.
     *
     * @param tag Material tag.
     * @return Material pointer (fallback to defaultMaterial if missing).
     */
    [[nodiscard]] Material* GetMaterialByTag(const std::string& tag);

    /**
     * @brief Returns the Font registered under the given tag.
     *
     * @details
     * If the tag is not found, an error is logged and nullptr is returned.
     *
     * @param tag Font tag.
     * @return Font pointer, or nullptr if missing.
     */
    [[nodiscard]] Font* GetFontByTag(const std::string& tag);

    /**
     * @brief Returns the SpriteSheet registered under the given tag.
     *
     * @details
     * If the tag is not found, an error is logged and the engine default spritesheet pointer is returned.
     *
     * @param tag SpriteSheet tag.
     * @return SpriteSheet pointer (fallback to defaultSpriteSheet if missing).
     */
    [[nodiscard]] SpriteSheet* GetSpriteSheetByTag(const std::string& tag);

    /**
     * @brief Executes the batched draw commands accumulated in renderMap, then clears the map.
     *
     * @details
     * For each batch:
     * - If batch.front()->CanBeInstanced() is true, per-instance arrays are built and drawn using instancing.
     * - Otherwise, objects are drawn one-by-one with per-object uniforms (u_Model, u_Color, u_UVOffset, u_UVScale).
     *
     * The function binds materials lazily to minimize redundant Bind/UnBind calls.
     * If a material has no texture bound, it force-sets "u_Texture" to errorTexture.
     *
     * The view/projection used:
     * - If ShouldIgnoreCamera() is true: view is identity.
     * - Otherwise: view is renderCamera->GetViewMatrix() if renderCamera exists, else identity.
     * Projection is orthographic based on either renderCamera screen size or window size.
     *
     * After flushing, renderMap entries are cleared per layer.
     *
     * @param engineContext Engine context providing access to window size and state.
     */
    void FlushDrawCommands(const EngineContext& engineContext);

    /**
     * @brief Sets the OpenGL viewport.
     *
     * @details
     * Wraps glViewport(x, y, width, height).
     *
     * @param x      Viewport origin X.
     * @param y      Viewport origin Y.
     * @param width  Viewport width.
     * @param height Viewport height.
     */
    void SetViewport(int x, int y, int width, int height);

    /**
     * @brief Clears a rectangular region with a color using scissor test.
     *
     * @details
     * Enables GL_SCISSOR_TEST, sets glScissor(), then clears GL_COLOR_BUFFER_BIT with the provided color.
     * Scissor test is disabled before returning.
     *
     * @param x      Rect origin X.
     * @param y      Rect origin Y.
     * @param width  Rect width.
     * @param height Rect height.
     * @param color  Clear color.
     */
    void ClearBackground(int x, int y, int width, int height, glm::vec4 color);

    /**
     * @brief Queues a debug line to be drawn during the debug-line flush pass.
     *
     * @details
     * The line is stored in debugLineMap keyed by (camera, lineWidth). If camera is nullptr,
     * the line is drawn in screen space (view = identity) using window-based orthographic projection.
     *
     * @param from      Line start position.
     * @param to        Line end position.
     * @param camera    Camera to use for view transform (nullptr for identity view).
     * @param color     Line color.
     * @param lineWidth OpenGL line width (used as a batching key).
     */
    void DrawDebugLine(const glm::vec2& from, const glm::vec2& to, Camera2D* camera = nullptr, const glm::vec4& color = { 1,1,1,1 }, float lineWidth = 1.0f);

    /**
     * @brief Returns the internal RenderLayerManager used for tag->layer mapping.
     */
    [[nodiscard]] RenderLayerManager& GetRenderLayerManager();

    /**
     * @brief Begins a new frame. Optionally binds and clears the offscreen render target.
     *
     * @details
     * If useOffscreen is false, this returns immediately.
     * Otherwise, it ensures the scene target matches the current window size and recreates it if needed,
     * then binds sceneFBO and clears it using WindowManager::GetBackgroundColor().
     *
     * @param engineContext Engine context used to query the window size and background color.
     */
    void BeginFrame(const EngineContext& engineContext);

    /**
     * @brief Ends the frame by blitting the offscreen render target to the default framebuffer.
     *
     * @details
     * If useOffscreen is false or sceneFBO is not created, this returns immediately.
     * Otherwise it uses glBlitFramebuffer to copy the color buffer to the window framebuffer.
     *
     * @param engineContext Engine context used to query the current window size.
     */
    void EndFrame(const EngineContext& engineContext);

    /**
     * @brief Dispatches a compute workload using a ComputeMaterial.
     *
     * @details
     * The dispatch group size is computed as (w+7)/8 and (h+7)/8 based on the destination texture size.
     * After dispatch, memory barriers are issued to ensure subsequent texture fetch and image access are visible.
     *
     * @param material Compute material providing the shader, destination image binding, and uniform/data upload.
     */
    void DispatchCompute(ComputeMaterial* material);

    /**
     * @brief Handles window resize events for offscreen rendering.
     *
     * @details
     * If useOffscreen is enabled and width/height are positive, recreates the scene target at the new size.
     *
     * @param width  New width in pixels.
     * @param height New height in pixels.
     */
    void OnResize(int width, int height);
private:
    /**
     * @brief Initializes internal engine resources required by the renderer.
     *
     * @details
     * Creates and registers engine-internal shaders and default resources, including:
     * - Internal text shader ("[EngineShader]internal_text")
     * - Internal debug line shader ("[EngineShader]internal_debug_line")
     * - Default solid-color shader ("[EngineShader]default")
     * - Default textured shader ("[EngineShader]default_texture")
     * - Error texture ("[EngineTexture]error") generated as an 8x8 checker pattern
     * - Default material ("[EngineMaterial]error") using the default textured shader and error texture
     * - Default quad mesh ("[EngineMesh]default")
     * - Default spritesheet ("[EngineSpriteSheet]default")
     * - Debug line VAO/VBO and vertex attribute layout
     * - Blending setup (SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
     * - Initial scene target creation matching the current window size
     *
     * @param engineContext Engine context providing access to window size.
     */
    void Init(const EngineContext& engineContext);

    /**
     * @brief Builds the internal RenderMap from a list of visible objects.
     *
     * @details
     * This function:
     * - Stores the camera pointer into renderCamera.
     * - Filters out null or invisible objects.
     * - Extracts Material, Mesh, SpriteSheet (via SpriteAnimator), and Shader pointers.
     * - Resolves the numeric layer ID from the object's render layer tag via RenderLayerManager.
     * - Quantizes Transform2D depth into an integer bin used for stable ordering.
     * - Groups objects into renderMap[layer][zbin][shader][InstanceBatchKey].
     *
     * Objects missing required components (material/mesh/shader) are skipped.
     *
     * @param source Visible objects (typically frustum-culled).
     * @param camera Active camera used for view/projection decisions in FlushDrawCommands().
     */
    void BuildRenderMap(const std::vector<Object*>& source, Camera2D* camera);

    /**
     * @brief Internal submission entry point used by engine systems to enqueue objects for rendering.
     *
     * @details
     * Determines the active camera from engineContext.stateManager->GetCurrentState()->GetActiveCamera().
     * If a camera exists, performs frustum culling (FrustumCuller::CullVisible) and then calls BuildRenderMap().
     *
     * @param objects       Source list of objects to consider.
     * @param engineContext Engine context used to access the current state and window info.
     */
    void Submit(const std::vector<Object*>& objects, const EngineContext& engineContext);

    /**
     * @brief Draws all queued debug lines and clears the queue.
     *
     * @details
     * Uses the internal debug line shader and a dynamic VBO update per (camera, lineWidth) batch.
     * For each batch:
     * - Sets line width
     * - Sets view matrix based on camera (identity if null)
     * - Uses window-size orthographic projection
     * - Uploads vertex data as an array of (pos.xy, color.rgba) for each endpoint
     * - Issues glDrawArrays(GL_LINES)
     *
     * After flushing, resets line width to 1.0 and clears debugLineMap.
     *
     * @param engineContext Engine context used to query window size.
     */
    void FlushDebugLineDrawCommands(const EngineContext& engineContext);

    /**
     * @brief Frees GPU resources and clears all registered render resources.
     *
     * @details
     * Deletes debug-line VAO/VBO, destroys the offscreen scene target, clears all resource maps,
     * and resets renderMap to an empty state.
     */
    void Free();



    std::unordered_map<std::string, std::unique_ptr<Shader>> shaderMap;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textureMap;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshMap;
    std::unordered_map<std::string, std::unique_ptr<Material>> materialMap;
    std::unordered_map<std::string, std::unique_ptr<Font>> fontMap;
    std::unordered_map<std::string, std::unique_ptr<SpriteSheet>> spritesheetMap;


    using CameraAndWidth = std::pair<Camera2D*, float>;

    /**
     * @brief Hash functor for (Camera2D*, lineWidth) keys used in debugLineMap.
     */
    struct CameraAndWidthHash
    {
        std::size_t operator()(const CameraAndWidth& key) const
        {
            return std::hash<Camera2D*>()(key.first) ^ std::hash<float>()(key.second);
        }
    };

    /**
     * @brief Accumulates debug line requests grouped by camera and line width.
     */
    std::unordered_map<CameraAndWidth, std::vector<LineInstance>, CameraAndWidthHash> debugLineMap;
    GLuint debugLineVAO = 0, debugLineVBO = 0;

    Shader* defaultShader, * debugLineShader;
    Material* defaultMaterial;
    SpriteSheet* defaultSpriteSheet;
    Mesh* defaultMesh;
    RenderMap renderMap;
    RenderLayerManager renderLayerManager;

    Camera2D* renderCamera;

    Texture* errorTexture;


    unsigned int sceneFBO = 0;
    unsigned int sceneColor = 0;
    int rtWidth = 0;
    int rtHeight = 0;
    bool useOffscreen = true;

    /**
     * @brief Creates or recreates the offscreen scene framebuffer and color texture.
     *
     * @details
     * Destroys any existing scene target, then:
     * - Creates a framebuffer (sceneFBO).
     * - Creates a 2D texture (sceneColor) as GL_RGBA16F storage at (w,h).
     * - Attaches it to GL_COLOR_ATTACHMENT0.
     * - Force-registers the texture under tag "[EngineTexture]RenderTexture".
     * - Validates framebuffer completeness and logs an error if incomplete.
     *
     * @param w Render target width.
     * @param h Render target height.
     */
    void CreateSceneTarget(int w, int h);

    /**
     * @brief Destroys the offscreen scene framebuffer and color texture if they exist.
     */
    void DestroySceneTarget();
};



/**
 * @brief Simple visibility culling utility based on a Camera2D view test and object bounding radius.
 *
 * @details
 * CullVisible() filters objects using:
 * - IsAlive() and IsVisible() checks
 * - ShouldIgnoreCamera(): such objects are always included
 * - Camera2D::IsInView(position, radius, viewportSize) for camera-relative objects
 *
 * The output list is cleared and rebuilt every call.
 */
class FrustumCuller
{
public:
    /**
     * @brief Builds a list of objects visible to the camera.
     *
     * @param camera          Camera used for view test.
     * @param allObjects      Input list of candidate objects.
     * @param outVisibleList  Output list that will be cleared and filled with visible objects.
     * @param viewportSize    Viewport size used by Camera2D::IsInView.
     */
    static void CullVisible(const Camera2D& camera, const std::vector<Object*>& allObjects,
        std::vector<Object*>& outVisibleList, glm::vec2 viewportSize);
};
