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
 * @brief Groups render batches first by shader, then by batch key, then by object list.
 *
 * @details
 * The engine uses this structure while building the frame's render map.
 * For a given depth bucket inside a given render layer, objects are grouped by:
 * - Shader*
 * - InstanceBatchKey
 * - std::vector<Object*>
 *
 * This grouping lets RenderManager later decide whether to draw objects one by one
 * or through the instancing path.
 */
using ShaderMap = std::unordered_map<Shader*, std::map<InstanceBatchKey, std::vector<Object*>>>;

/**
 * @brief Stores the full frame's render organization by render layer and depth bucket.
 *
 * @details
 * RenderMap is indexed by render layer first. Each layer stores a map keyed by
 * quantized depth. Each depth entry then contains a ShaderMap.
 *
 * This means the effective ordering is:
 * - render layer
 * - quantized depth
 * - shader
 * - instance batch key
 * - object list
 *
 * RenderManager::BuildRenderMap() fills this structure, and
 * RenderManager::FlushDrawCommands() consumes and clears it.
 */
using RenderMap = std::array<std::map<int, ShaderMap>, RenderLayerManager::MAX_LAYERS>;

/**
 * @brief Represents one queued debug line draw request.
 *
 * @details
 * Debug line requests are accumulated during the frame and later flushed in
 * RenderManager::FlushDebugLineDrawCommands().
 *
 * Lines are grouped by:
 * - the camera used to compute the view transform
 * - the requested line width
 */
struct LineInstance
{
    glm::vec2 from = { 0,0 };
    glm::vec2 to = { 0,0 };
    glm::vec4 color = { 1,1,1,1 };
    float lineWidth = 1;
};

/**
 * @brief Central rendering system responsible for resource registration, batching, culling integration, and frame submission.
 *
 * @details
 * RenderManager is one of the engine's core systems. It owns and manages shared
 * render resources such as:
 * - shaders
 * - textures
 * - meshes
 * - materials
 * - fonts
 * - sprite sheets
 * - render layers
 *
 * It also manages the runtime rendering flow for each frame:
 * - frame target setup through BeginFrame()
 * - visible-object collection through Submit()
 * - render grouping through BuildRenderMap()
 * - actual draw execution through FlushDrawCommands()
 * - offscreen presentation through EndFrame()
 * - debug line accumulation and flush
 *
 * In the current implementation, Submit() asks the active state for its current
 * camera, performs frustum culling with FrustumCuller, and then builds the
 * internal render map from the visible objects only.
 *
 * FlushDrawCommands() iterates layer-by-layer and depth-by-depth, then either:
 * - issues a single instanced draw for a compatible batch
 * - or draws objects individually when instancing is not available
 *
 * The manager also owns internal fallback/default resources such as:
 * - default shader
 * - debug line shader
 * - default material
 * - default mesh
 * - default sprite sheet
 * - error texture
 *
 * @note
 * This class is tightly integrated with ObjectManager, StateManager, and JinEngine.
 * Several internal functions are intentionally private and exposed only through friendship.
 */
class RenderManager
{
    friend ObjectManager;
    friend StateManager;
    friend JinEngine;

public:
    /**
     * @brief Registers a shader from a list of shader stage/file path pairs.
     *
     * @details
     * This function creates a new Shader, attaches each supplied stage from file,
     * links the program, and stores it in shaderMap under the given tag.
     *
     * Registration is skipped if the tag already exists.
     * Registration also fails if any stage attachment or the final link step fails.
     *
     * @param tag User-defined shader tag.
     * @param sources Pairs of shader stage and source file path.
     */
    void RegisterShader(const std::string& tag, const std::vector<std::pair<ShaderStage, FilePath>>& sources);

    /**
     * @brief Registers an already-created shader object under a tag.
     *
     * @details
     * Registration is skipped if the tag already exists.
     *
     * @param tag User-defined shader tag.
     * @param shader Shared shader instance to store.
     */
    void RegisterShader(const std::string& tag, std::shared_ptr<Shader> shader);

    /**
     * @brief Registers a texture from a file path.
     *
     * @details
     * This function creates a Texture from disk using the provided settings and
     * stores it in textureMap under the given tag.
     *
     * Registration is skipped if the tag already exists.
     *
     * @param tag User-defined texture tag.
     * @param path Texture file path.
     * @param settings Texture sampling and wrapping settings.
     */
    void RegisterTexture(const std::string& tag, const FilePath& path, const TextureSettings& settings = {});

    /**
     * @brief Registers an already-created texture object under a tag.
     *
     * @details
     * Registration is skipped if the tag already exists.
     *
     * @param tag User-defined texture tag.
     * @param texture Shared texture instance to store.
     */
    void RegisterTexture(const std::string& tag, std::shared_ptr<Texture> texture);

    /**
     * @brief Forces a texture registration or in-place update using an existing GPU texture ID.
     *
     * @details
     * If the tag already exists, the existing Texture object is updated in place
     * through ForceUpdateTexture().
     * Otherwise, a new Texture object wrapping the supplied texture ID is created.
     *
     * This path is currently used internally for render targets such as the engine's
     * offscreen scene texture.
     *
     * @param tag Texture tag to create or update.
     * @param id_ Existing OpenGL texture ID.
     * @param width_ Texture width.
     * @param height_ Texture height.
     * @param channels_ Number of texture channels.
     */
    void ForceRegisterTexture(const std::string& tag, unsigned int id_, int width_, int height_, int channels_);

    /**
     * @brief Forces a texture registration or in-place update using raw pixel data.
     *
     * @details
     * If the tag already exists, the existing Texture object is updated in place
     * through ForceUpdateTexture().
     * Otherwise, a new Texture object is created from the supplied pixel data.
     *
     * @param tag Texture tag to create or update.
     * @param data Raw pixel data.
     * @param width_ Texture width.
     * @param height_ Texture height.
     * @param channels_ Number of channels in the pixel data.
     * @param settings Texture sampling and wrapping settings.
     */
    void ForceRegisterTexture(const std::string& tag, const unsigned char* data, int width_, int height_, int channels_, const TextureSettings& settings = {});

    /**
     * @brief Registers a mesh from vertex/index data.
     *
     * @details
     * This function constructs a Mesh from the supplied geometry data and stores it
     * in meshMap under the given tag.
     *
     * Registration is skipped if the tag already exists.
     *
     * @param tag User-defined mesh tag.
     * @param vertices Vertex array for the mesh.
     * @param indices Optional index array.
     * @param primitiveType Primitive topology used when drawing the mesh.
     */
    void RegisterMesh(const std::string& tag, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices = {}, PrimitiveType primitiveType = PrimitiveType::Triangles);

    /**
     * @brief Registers an already-created mesh object under a tag.
     *
     * @details
     * Registration is skipped if the tag already exists.
     *
     * @param tag User-defined mesh tag.
     * @param mesh Shared mesh instance to store.
     */
    void RegisterMesh(const std::string& tag, std::shared_ptr<Mesh> mesh);

    /**
     * @brief Registers a material by creating it from a shader tag and texture bindings.
     *
     * @details
     * This helper resolves the shader first, creates a Material, and then binds
     * textures by looking up each texture tag in textureMap.
     *
     * If a texture binding references a texture tag that does not exist, that binding
     * is skipped and a warning is logged.
     *
     * Registration is skipped if the material tag already exists.
     *
     * @param tag User-defined material tag.
     * @param shaderTag Tag of the shader the material should use.
     * @param textureBindings Map from uniform name to texture tag.
     */
    void RegisterMaterial(const std::string& tag, const std::string& shaderTag, const std::unordered_map<UniformName, TextureTag>& textureBindings);

    /**
     * @brief Registers an already-created material object under a tag.
     *
     * @details
     * Registration is skipped if the tag already exists.
     *
     * @param tag User-defined material tag.
     * @param material Shared material instance to store.
     */
    void RegisterMaterial(const std::string& tag, std::shared_ptr<Material> material);

    /**
     * @brief Registers a font from a TTF file and pixel size.
     *
     * @details
     * This function validates the pixel size and creates a Font object.
     * In the current implementation, allowed font sizes are restricted to the range 4 to 64.
     *
     * Registration is skipped if the tag already exists.
     *
     * @param tag User-defined font tag.
     * @param ttfPath Path to the TTF file.
     * @param pixelSize Requested font pixel size.
     */
    void RegisterFont(const std::string& tag, const std::string& ttfPath, uint32_t pixelSize);

    /**
     * @brief Registers an already-created font object under a tag.
     *
     * @details
     * Registration is skipped if the tag already exists.
     *
     * @param tag User-defined font tag.
     * @param font Shared font instance to store.
     */
    void RegisterFont(const std::string& tag, std::shared_ptr<Font> font);

    /**
     * @brief Registers a named render layer at a specific numeric layer slot.
     *
     * @details
     * This forwards the request to RenderLayerManager.
     * Layer registration is limited by RenderLayerManager::MAX_LAYERS and by
     * whether the target slot is already occupied.
     *
     * @param tag Render layer name.
     * @param layer Numeric layer index.
     */
    void RegisterRenderLayer(const std::string& tag, uint8_t layer);

    /**
     * @brief Registers a sprite sheet from an already-registered texture tag.
     *
     * @details
     * This function resolves the texture first, then constructs a SpriteSheet
     * using the frame width and frame height.
     *
     * Registration is skipped if the sprite sheet tag already exists.
     *
     * @param tag User-defined sprite sheet tag.
     * @param textureTag Texture tag to use as the sprite sheet source.
     * @param frameW Width of one frame in pixels.
     * @param frameH Height of one frame in pixels.
     */
    void RegisterSpriteSheet(const std::string& tag, const std::string& textureTag, int frameW, int frameH);

    /**
     * @brief Unregisters a shader resource if it is no longer in use.
     *
     * @details
     * The current implementation refuses to erase the shader if:
     * - the tag does not exist
     * - the stored shared_ptr use_count is greater than 1
     *
     * @param tag Shader tag to unregister.
     * @param engineContext Shared engine services.
     */
    void UnregisterShader(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a texture resource if it is no longer in use.
     *
     * @details
     * The current implementation refuses to erase the texture if:
     * - the tag does not exist
     * - the stored shared_ptr use_count is greater than 1
     *
     * @param tag Texture tag to unregister.
     * @param engineContext Shared engine services.
     */
    void UnregisterTexture(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a mesh resource if it is no longer in use.
     *
     * @details
     * The current implementation refuses to erase the mesh if:
     * - the tag does not exist
     * - the stored shared_ptr use_count is greater than 1
     *
     * @param tag Mesh tag to unregister.
     * @param engineContext Shared engine services.
     */
    void UnregisterMesh(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a material resource if it is no longer in use.
     *
     * @details
     * The current implementation refuses to erase the material if:
     * - the tag does not exist
     * - the stored shared_ptr use_count is greater than 1
     *
     * @param tag Material tag to unregister.
     * @param engineContext Shared engine services.
     */
    void UnregisterMaterial(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a font resource if it is no longer in use.
     *
     * @details
     * The current implementation refuses to erase the font if:
     * - the tag does not exist
     * - the stored shared_ptr use_count is greater than 1
     *
     * @param tag Font tag to unregister.
     * @param engineContext Shared engine services.
     */
    void UnregisterFont(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Unregisters a named render layer.
     *
     * @details
     * This forwards the request to RenderLayerManager.
     *
     * @param tag Render layer tag to unregister.
     */
    void UnregisterRenderLayer(const std::string& tag);

    /**
     * @brief Unregisters a sprite sheet resource if it is no longer in use.
     *
     * @details
     * The current implementation refuses to erase the sprite sheet if:
     * - the tag does not exist
     * - the stored shared_ptr use_count is greater than 1
     *
     * @param tag Sprite sheet tag to unregister.
     * @param engineContext Shared engine services.
     */
    void UnregisterSpriteSheet(const std::string& tag, const EngineContext& engineContext);

    /**
     * @brief Returns whether a texture tag is registered.
     *
     * @param tag Texture tag to query.
     * @return True if the texture exists in textureMap.
     */
    [[nodiscard]] bool HasTexture(const std::string& tag) const;

    /**
     * @brief Returns whether a shader tag is registered.
     *
     * @param tag Shader tag to query.
     * @return True if the shader exists in shaderMap.
     */
    [[nodiscard]] bool HasShader(const std::string& tag) const;

    /**
     * @brief Returns whether a font tag is registered.
     *
     * @param tag Font tag to query.
     * @return True if the font exists in fontMap.
     */
    [[nodiscard]] bool HasFont(const std::string& tag) const;

    /**
     * @brief Returns whether a sprite sheet tag is registered.
     *
     * @param tag Sprite sheet tag to query.
     * @return True if the sprite sheet exists in spritesheetMap.
     */
    [[nodiscard]] bool HasSpriteSheet(const std::string& tag) const;

    /**
     * @brief Resolves a shader by tag.
     *
     * @details
     * If the tag is not found, the current implementation logs an error and returns
     * the engine's default shader.
     *
     * @param tag Shader tag to resolve.
     * @return Shared pointer to the shader resource.
     */
    [[nodiscard]] std::shared_ptr<Shader> GetShaderByTag(const std::string& tag);

    /**
     * @brief Resolves a texture by tag.
     *
     * @details
     * If the tag is not found, the current implementation logs an error and returns
     * the engine's error texture.
     *
     * @param tag Texture tag to resolve.
     * @return Shared pointer to the texture resource.
     */
    [[nodiscard]] std::shared_ptr<Texture> GetTextureByTag(const std::string& tag);

    /**
     * @brief Resolves a mesh by tag.
     *
     * @details
     * If the tag is not found, the current implementation logs an error and returns
     * the engine's default mesh.
     *
     * @param tag Mesh tag to resolve.
     * @return Shared pointer to the mesh resource.
     */
    [[nodiscard]] std::shared_ptr<Mesh> GetMeshByTag(const std::string& tag);

    /**
     * @brief Resolves a material by tag.
     *
     * @details
     * If the tag is not found, the current implementation logs an error and returns
     * the engine's default material.
     *
     * @param tag Material tag to resolve.
     * @return Shared pointer to the material resource.
     */
    [[nodiscard]] std::shared_ptr<Material> GetMaterialByTag(const std::string& tag);

    /**
     * @brief Resolves a font by tag.
     *
     * @details
     * If the tag is not found, the current implementation logs an error and returns nullptr.
     *
     * @param tag Font tag to resolve.
     * @return Shared pointer to the font resource, or nullptr if not found.
     */
    [[nodiscard]] std::shared_ptr<Font> GetFontByTag(const std::string& tag);

    /**
     * @brief Resolves a sprite sheet by tag.
     *
     * @details
     * If the tag is not found, the current implementation logs an error and returns
     * the engine's default sprite sheet.
     *
     * @param tag Sprite sheet tag to resolve.
     * @return Shared pointer to the sprite sheet resource.
     */
    [[nodiscard]] std::shared_ptr<SpriteSheet> GetSpriteSheetByTag(const std::string& tag);

    /**
     * @brief Flushes the current frame's accumulated render batches and executes draw calls.
     *
     * @details
     * This is the main render execution function after batches have been built.
     * The current implementation iterates in this order:
     * - render layer
     * - quantized depth
     * - shader group
     * - instance batch key
     *
     * For each batch:
     * - if the front object can be instanced, per-instance data is collected,
     *   uploaded, and drawn through Mesh::DrawInstanced()
     * - otherwise each object is drawn individually with its own model/color/UV state
     *
     * This function also:
     * - handles material bind reuse through lastMaterial
     * - selects camera view or identity view depending on ShouldIgnoreCamera()
     * - builds an orthographic projection from the active render camera or window size
     * - injects the error texture when a material has no texture bound
     * - clears renderMap at the end of the flush
     *
     * @param engineContext Shared engine services used during the flush.
     */
    void FlushDrawCommands(const EngineContext& engineContext);

    /**
     * @brief Sets the OpenGL viewport.
     *
     * @param x Viewport X position.
     * @param y Viewport Y position.
     * @param width Viewport width.
     * @param height Viewport height.
     */
    void SetViewport(int x, int y, int width, int height);

    /**
     * @brief Clears a rectangular background region using scissor testing.
     *
     * @details
     * This function enables GL_SCISSOR_TEST, restricts clearing to the specified
     * rectangle, clears the color buffer using the supplied color, and then disables
     * scissor testing again.
     *
     * @param x Region X position.
     * @param y Region Y position.
     * @param width Region width.
     * @param height Region height.
     * @param color Clear color.
     */
    void ClearBackground(int x, int y, int width, int height, glm::vec4 color);

    /**
     * @brief Queues a debug line to be drawn later in the frame.
     *
     * @details
     * The line is not drawn immediately. Instead, it is appended into debugLineMap
     * under a key composed of:
     * - the camera pointer
     * - the requested line width
     *
     * The accumulated lines are later rendered in FlushDebugLineDrawCommands().
     *
     * @param from Start position of the line.
     * @param to End position of the line.
     * @param camera Camera used for the debug line's view transform. May be nullptr.
     * @param color Line color.
     * @param lineWidth Requested OpenGL line width.
     */
    void DrawDebugLine(const glm::vec2& from, const glm::vec2& to, Camera2D* camera = nullptr, const glm::vec4& color = { 1,1,1,1 }, float lineWidth = 1.0f);

    /**
     * @brief Returns the internal render layer manager.
     *
     * @details
     * This exposes direct mutable access to the layer-name/ID registry.
     *
     * @return Reference to the internal RenderLayerManager.
     */
    [[nodiscard]] RenderLayerManager& GetRenderLayerManager();

    /**
     * @brief Prepares the frame's render target and clears it.
     *
     * @details
     * In the current implementation, when offscreen rendering is enabled this function:
     * - ensures the offscreen scene target matches the window size
     * - recreates the target if needed
     * - binds the scene framebuffer
     * - sets the viewport to the render target size
     * - clears it with the window background color
     *
     * If offscreen rendering is disabled, the function returns immediately.
     *
     * @param engineContext Shared engine services used to query window information.
     */
    void BeginFrame(const EngineContext& engineContext);

    /**
     * @brief Presents the frame from the offscreen render target to the default framebuffer.
     *
     * @details
     * In the current implementation, when offscreen rendering is enabled this function:
     * - binds the scene FBO for reading
     * - binds the default framebuffer for drawing
     * - blits the scene color attachment to the window framebuffer
     *
     * If offscreen rendering is disabled or no scene framebuffer exists, the function returns immediately.
     *
     * @param engineContext Shared engine services used to query current window size.
     */
    void EndFrame(const EngineContext& engineContext);

    /**
     * @brief Dispatches a compute material over its destination texture.
     *
     * @details
     * This function binds the compute material, sends its data, dispatches compute
     * work groups sized from the destination texture dimensions, issues the relevant
     * memory barrier, and then unbinds the material.
     *
     * The current implementation uses 8x8 thread-group assumptions when computing
     * group counts.
     *
     * @param material Compute material to dispatch.
     */
    void DispatchCompute(std::shared_ptr<ComputeMaterial> material);

    /**
     * @brief Recreates the offscreen scene target when the window size changes.
     *
     * @details
     * If offscreen rendering is disabled, this function does nothing.
     * Otherwise, if width and height are both positive, CreateSceneTarget() is called.
     *
     * @param width New window width.
     * @param height New window height.
     */
    void OnResize(int width, int height);

private:
    /**
     * @brief Initializes internal engine-owned rendering resources.
     *
     * @details
     * This internal setup creates and registers:
     * - internal text shader
     * - internal debug line shader
     * - default shader
     * - error texture
     * - default textured material
     * - default quad mesh
     * - default sprite sheet
     * - debug line VAO/VBO
     * - blend state
     * - initial offscreen scene target
     *
     * This is an engine bootstrap function and is not intended for normal user calls.
     *
     * @param engineContext Shared engine services used during renderer bootstrap.
     */
    void Init(const EngineContext& engineContext);

    /**
     * @brief Builds the internal render map from a visible object list and the current camera.
     *
     * @details
     * This function groups visible objects by:
     * - render layer
     * - quantized depth
     * - shader
     * - mesh/material/sprite sheet batch key
     *
     * Only objects that successfully provide a material, mesh, and shader are inserted.
     * The current render camera pointer is also stored so FlushDrawCommands() can later
     * build the correct view and projection matrices.
     *
     * @param source Visible object pointers to organize for drawing.
     * @param camera Active camera used for this submission.
     */
    void BuildRenderMap(const std::vector<Object*>& source, Camera2D* camera);

    /**
     * @brief Performs frustum culling and submits visible objects into the render map.
     *
     * @details
     * The current implementation:
     * - asks the active state for its active camera
     * - performs frustum culling through FrustumCuller::CullVisible()
     * - passes the visible object list into BuildRenderMap()
     *
     * If no active camera exists, nothing is submitted.
     *
     * This is an internal helper called by ObjectManager and not a normal public API.
     *
     * @param objects Source object list.
     * @param engineContext Shared engine services used to access the active state and camera.
     */
    void Submit(const std::vector<Object*>& objects, const EngineContext& engineContext);

    /**
     * @brief Flushes all queued debug line draws.
     *
     * @details
     * This function:
     * - binds the internal debug line shader
     * - iterates grouped line batches by camera and line width
     * - computes view/projection
     * - uploads line vertex data dynamically
     * - issues GL_LINES draw calls
     * - restores line width
     * - clears debugLineMap
     *
     * @param engineContext Shared engine services used to compute the projection size.
     */
    void FlushDebugLineDrawCommands(const EngineContext& engineContext);

    /**
     * @brief Releases internal renderer-owned GPU objects and clears stored resource maps.
     *
     * @details
     * This function deletes debug line buffers, destroys the offscreen scene target,
     * releases default internal resources, clears all resource maps, and resets renderMap.
     *
     * This is an engine shutdown/helper function and is not intended for normal user calls.
     */
    void Free();

    std::unordered_map<std::string, std::shared_ptr<Shader>> shaderMap;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textureMap;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshMap;
    std::unordered_map<std::string, std::shared_ptr<Material>> materialMap;
    std::unordered_map<std::string, std::shared_ptr<Font>> fontMap;
    std::unordered_map<std::string, std::shared_ptr<SpriteSheet>> spritesheetMap;

    using CameraAndWidth = std::pair<Camera2D*, float>;

    /**
     * @brief Hash functor for grouping debug lines by camera pointer and line width.
     *
     * @details
     * This is used as the hash function for debugLineMap.
     */
    struct CameraAndWidthHash
    {
        std::size_t operator()(const CameraAndWidth& key) const
        {
            return std::hash<Camera2D*>()(key.first) ^ std::hash<float>()(key.second);
        }
    };

    std::unordered_map<CameraAndWidth, std::vector<LineInstance>, CameraAndWidthHash> debugLineMap;
    GLuint debugLineVAO = 0, debugLineVBO = 0;

    std::shared_ptr<Shader> defaultShader, debugLineShader;
    std::shared_ptr<Material> defaultMaterial;
    std::shared_ptr<SpriteSheet> defaultSpriteSheet;
    std::shared_ptr<Mesh> defaultMesh;
    std::shared_ptr<Texture> errorTexture;

    RenderMap renderMap;
    RenderLayerManager renderLayerManager;

    Camera2D* renderCamera;

    unsigned int sceneFBO = 0;
    unsigned int sceneColor = 0;
    int rtWidth = 0;
    int rtHeight = 0;
    bool useOffscreen = true;

    /**
     * @brief Creates or recreates the offscreen scene render target.
     *
     * @details
     * The current implementation:
     * - destroys the old target first
     * - creates an FBO
     * - creates a color texture with RGBA16F storage
     * - attaches it to the FBO
     * - registers or updates the engine-owned render texture tag
     * - validates framebuffer completeness
     *
     * @param w Target width.
     * @param h Target height.
     */
    void CreateSceneTarget(int w, int h);

    /**
     * @brief Destroys the current offscreen scene render target.
     *
     * @details
     * This deletes the scene color texture and the framebuffer if they exist,
     * then resets target size tracking members.
     */
    void DestroySceneTarget();
};

/**
 * @brief Performs visibility culling against a 2D camera view.
 *
 * @details
 * FrustumCuller is a stateless helper used by RenderManager before batching.
 * The current implementation checks each object and:
 * - skips objects that are not alive
 * - skips objects that are not visible
 * - always keeps objects that ignore the camera
 * - otherwise tests the object's world position and bounding radius through Camera2D::IsInView()
 *
 * The output list is cleared before results are appended.
 */
class FrustumCuller
{
public:
    /**
     * @brief Filters a source object list down to only visible objects for the given camera.
     *
     * @details
     * For each object:
     * - dead objects are ignored
     * - invisible objects are ignored
     * - camera-ignoring objects are kept automatically
     * - all other objects are tested using camera.IsInView()
     *
     * The object's world position and approximate bounding radius are used for the visibility test.
     *
     * @param camera Camera used for the visibility test.
     * @param allObjects Source object list.
     * @param outVisibleList Output list that receives visible objects.
     * @param viewportSize Viewport size used by the camera visibility check.
     */
    static void CullVisible(const Camera2D& camera, const std::vector<Object*>& allObjects,
        std::vector<Object*>& outVisibleList, glm::vec2 viewportSize);
};