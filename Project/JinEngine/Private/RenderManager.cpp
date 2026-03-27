#include "Engine.h"

#include <algorithm>
#include "ext/matrix_clip_space.hpp"
#include "gl.h"

void RenderManager::Submit(const std::vector<Object*>& objects, const EngineContext& engineContext)
{
    std::vector<Object*> visibleObjects;
    Camera2D* camera = engineContext.stateManager->GetCurrentState()->GetActiveCamera();
    if (camera)
    {
        FrustumCuller::CullVisible(*camera, objects, visibleObjects, glm::vec2(camera->GetScreenWidth(), camera->GetScreenHeight()));
        BuildRenderMap(visibleObjects, camera);
    }
}

void RenderManager::CreateSceneTarget(int w, int h)
{
    DestroySceneTarget();

    rtWidth = w;
    rtHeight = h;

    glCreateFramebuffers(1, &sceneFBO);

    glCreateTextures(GL_TEXTURE_2D, 1, &sceneColor);
    glTextureStorage2D(sceneColor, 1, GL_RGBA16F, rtWidth, rtHeight);
    glTextureParameteri(sceneColor, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(sceneColor, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(sceneColor, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(sceneColor, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(sceneFBO, GL_COLOR_ATTACHMENT0, sceneColor, 0);
    const GLenum buf = GL_COLOR_ATTACHMENT0;
    glNamedFramebufferDrawBuffers(sceneFBO, 1, &buf);

    ForceRegisterTexture("[EngineTexture]RenderTexture", sceneColor, rtWidth, rtHeight, 4);

    GLenum status = glCheckNamedFramebufferStatus(sceneFBO, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        JIN_ERR("sceneFBO incomplete!" << status);
    }
}

void RenderManager::DestroySceneTarget()
{
    if (sceneColor)
    {
        glDeleteTextures(1, &sceneColor);
        sceneColor = 0;
    }
    if (sceneFBO)
    {
        glDeleteFramebuffers(1, &sceneFBO);
        sceneFBO = 0;
    }
    rtWidth = rtHeight = 0;
}

void RenderManager::BeginFrame(const EngineContext& engineContext)
{
    if (!useOffscreen)
        return;

    const int w = engineContext.windowManager->GetWidth();
    const int h = engineContext.windowManager->GetHeight();
    if (w != rtWidth || h != rtHeight || !sceneFBO)
    {
        CreateSceneTarget(w, h);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, rtWidth, rtHeight);

    glm::vec4 backgroundColor =  engineContext.windowManager->GetBackgroundColor();
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void RenderManager::EndFrame(const EngineContext& engineContext)
{
    if (!useOffscreen || !sceneFBO)
        return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    const int w = engineContext.windowManager->GetWidth();
    const int h = engineContext.windowManager->GetHeight();

    glBlitFramebuffer(
        0, 0, rtWidth, rtHeight,
        0, 0, w, h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderManager::DispatchCompute(std::shared_ptr<ComputeMaterial> material)
{
    if (!material)
        return;
    const int w = material->GetDstTexture()->GetWidth();
    const int h = material->GetDstTexture()->GetHeight();
    const GLuint gx = (w + 7) / 8;
    const GLuint gy = (h + 7) / 8;

    material->Bind();
    material->SendData();
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
        GL_TEXTURE_FETCH_BARRIER_BIT |
        GL_SHADER_STORAGE_BARRIER_BIT);
    material->UnBind();
}

void RenderManager::OnResize(int width, int height)
{
    if (!useOffscreen)
        return;

    if (width > 0 && height > 0)
    {
        CreateSceneTarget(width, height);
    }
}

void FrustumCuller::CullVisible(const Camera2D& camera, const std::vector<Object*>& allObjects,
                                std::vector<Object*>& outVisibleList, glm::vec2 viewportSize)
{
    outVisibleList.clear();
    for (Object* obj : allObjects)
    {
        if (!obj->IsAlive() || !obj->IsVisible())
            continue;
        if (obj->ShouldIgnoreCamera())
        {
            outVisibleList.push_back(obj);
            continue;
        }
        const glm::vec2& pos = obj->GetWorldPosition();
        float radius = obj->GetBoundingRadius();

        if (camera.IsInView(pos, radius, viewportSize))
            outVisibleList.push_back(obj);
    }
}

void RenderManager::FlushDrawCommands(const EngineContext& engineContext)
{
    Material* lastMaterial = nullptr;

    for (uint8_t layer = 0; layer < renderMap.size(); ++layer)
    {
        for (const auto& pair : renderMap[layer])
        {
            const ShaderMap& shaderMapForDepth = pair.second;

            for (const auto& [shader, batchMap] : shaderMapForDepth)
            {
                for (const auto& [key, batch] : batchMap)
                {
                    if (batch.empty())
                        continue;

                    Mesh* mesh = key.mesh ? key.mesh : defaultMesh.get();
                    Material* material = key.material ? key.material : defaultMaterial.get();

                    if (!mesh || !material)
                        continue;

                    if (batch.front()->CanBeInstanced())
                    {
                        std::vector<glm::mat4> transforms;
                        std::vector<glm::vec4> colors;
                        std::vector<glm::vec2> uvOffsets;
                        std::vector<glm::vec2> uvScales;

                        transforms.reserve(batch.size());
                        colors.reserve(batch.size());
                        uvOffsets.reserve(batch.size());
                        uvScales.reserve(batch.size());

                        for (const auto& obj : batch)
                        {
                            glm::mat4 model = obj->GetTransform2DMatrix();
                            glm::vec2 flip = obj->GetUVFlipVector();
                            model = model * glm::scale(glm::mat4(1.0f), glm::vec3(flip, 1.0f));
                            transforms.push_back(model);

                            colors.push_back(obj->GetColor());

                            if (obj->HasAnimation())
                            {
                                SpriteAnimator* spriteAnimator = obj->GetSpriteAnimator();
                                uvOffsets.push_back(spriteAnimator->GetUVOffset());
                                uvScales.push_back(spriteAnimator->GetUVScale());
                            }
                            else
                            {
                                uvOffsets.emplace_back(0.0f, 0.0f);
                                uvScales.emplace_back(1.0f, 1.0f);
                            }
                        }

                        if (material != lastMaterial)
                        {
                            if (lastMaterial)
                                lastMaterial->UnBind();

                            material->Bind();
                            lastMaterial = material;
                        }

                        bool ignoreCam = batch.front()->ShouldIgnoreCamera();

                        if (!material->HasTexture())
                        {
                            material->SetTexture("u_Texture", errorTexture);
                        }

                        glm::mat4 view = ignoreCam
                            ? glm::mat4(1.0f)
                            : (renderCamera ? renderCamera->GetViewMatrix() : glm::mat4(1.0f));

                        int w = renderCamera ? renderCamera->GetScreenWidth() : engineContext.windowManager->GetWidth();
                        int h = renderCamera ? renderCamera->GetScreenHeight() : engineContext.windowManager->GetHeight();

                        glm::mat4 projection = glm::ortho(
                            -static_cast<float>(w) / 2.0f,
                            static_cast<float>(w) / 2.0f,
                            -static_cast<float>(h) / 2.0f,
                            static_cast<float>(h) / 2.0f
                        );

                        material->SetUniform("u_View", view);
                        material->SetUniform("u_Projection", projection);

                        if (batch.front()->HasAnimation())
                        {
                            std::shared_ptr<Texture> animTexture = batch.front()->GetSpriteAnimator()->GetTexture();
                            material->SetTexture("u_Texture", animTexture ? animTexture : errorTexture);
                        }

                        batch.front()->Draw(engineContext);
                        material->SendData();

                        mesh->UpdateInstanceBuffer(transforms, colors, uvOffsets, uvScales);
                        mesh->DrawInstanced(static_cast<GLsizei>(transforms.size()));
                    }
                    else
                    {
                        for (const auto& obj : batch)
                        {
                            Material* objMaterial = key.material ? key.material : defaultMaterial.get();
                            Mesh* objMesh = key.mesh ? key.mesh : defaultMesh.get();

                            if (!objMaterial || !objMesh)
                                continue;

                            if (objMaterial != lastMaterial)
                            {
                                if (lastMaterial)
                                    lastMaterial->UnBind();

                                objMaterial->Bind();
                                lastMaterial = objMaterial;
                            }

                            bool ignoreCam = obj->ShouldIgnoreCamera();

                            if (!objMaterial->HasTexture())
                            {
                                objMaterial->SetTexture("u_Texture", errorTexture);
                            }

                            glm::mat4 view = ignoreCam
                                ? glm::mat4(1.0f)
                                : (renderCamera ? renderCamera->GetViewMatrix() : glm::mat4(1.0f));

                            int w = renderCamera ? renderCamera->GetScreenWidth() : engineContext.windowManager->GetWidth();
                            int h = renderCamera ? renderCamera->GetScreenHeight() : engineContext.windowManager->GetHeight();

                            glm::mat4 projection = glm::ortho(
                                -static_cast<float>(w) / 2.0f,
                                static_cast<float>(w) / 2.0f,
                                -static_cast<float>(h) / 2.0f,
                                static_cast<float>(h) / 2.0f
                            );

                            objMaterial->SetUniform("u_View", view);
                            objMaterial->SetUniform("u_Projection", projection);

                            glm::mat4 model = obj->GetTransform2DMatrix();
                            glm::vec2 flip = obj->GetUVFlipVector();
                            model = model * glm::scale(glm::mat4(1.0f), glm::vec3(flip, 1.0f));

                            objMaterial->SetUniform("u_Model", model);
                            objMaterial->SetUniform("u_Color", obj->GetColor());

                            if (obj->HasAnimation())
                            {
                                SpriteAnimator* spriteAnimator = obj->GetSpriteAnimator();

                                objMaterial->SetUniform("u_UVOffset", spriteAnimator->GetUVOffset());
                                objMaterial->SetUniform("u_UVScale", spriteAnimator->GetUVScale());

                                std::shared_ptr<Texture> animTexture = spriteAnimator->GetTexture();
                                objMaterial->SetTexture("u_Texture", animTexture ? animTexture : errorTexture);
                            }

                            obj->Draw(engineContext);
                            objMaterial->SendData();
                            objMesh->Draw();
                        }
                    }
                }
            }
        }
    }

    if (lastMaterial)
        lastMaterial->UnBind();

    for (auto& shdrMap : renderMap)
    {
        shdrMap.clear();
    }
}

void RenderManager::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void RenderManager::ClearBackground(int x, int y, int width, int height, glm::vec4 color)
{
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

void RenderManager::DrawDebugLine(const glm::vec2& from, const glm::vec2& to, Camera2D* camera, const glm::vec4& color, float lineWidth)
{
    debugLineMap[{camera, lineWidth}].push_back({ from, to, color, lineWidth });
}
void RenderManager::FlushDebugLineDrawCommands(const EngineContext& engineContext)
{
    debugLineShader->Use();

    for (const auto& [camWidth, lines] : debugLineMap)
    {
        Camera2D* camera = camWidth.first;
        float lineWidth = camWidth.second;

        glLineWidth(lineWidth);
        glm::mat4 view = camera
            ? camera->GetViewMatrix()
            : glm::mat4(1);
        glm::mat4 proj = glm::ortho(
            -static_cast<float>(engineContext.windowManager->GetWidth()) / 2,
            static_cast<float>(engineContext.windowManager->GetWidth()) / 2,
            -static_cast<float>(engineContext.windowManager->GetHeight()) / 2,
            static_cast<float>(engineContext.windowManager->GetHeight()) / 2);

        debugLineShader->SendUniform("u_View", view);
        debugLineShader->SendUniform("u_Projection", proj);

        std::vector<float> vertexData;
        vertexData.reserve(lines.size() * 12);

        for (const auto& line : lines)
        {
            vertexData.insert(vertexData.end(), {
                line.from.x, line.from.y, line.color.r, line.color.g, line.color.b, line.color.a,
                line.to.x,   line.to.y,   line.color.r, line.color.g, line.color.b, line.color.a
                });
        }

        glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);

        glBindVertexArray(debugLineVAO);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size() * 2));
        glBindVertexArray(0);
    }

    glLineWidth(1.0f);
    debugLineShader->Unuse();
    debugLineMap.clear();
}

void RenderManager::Free()
{
    if (debugLineVBO)
    {
        glDeleteBuffers(1, &debugLineVBO);
        debugLineVBO = 0;
    }
    if (debugLineVAO)
    {
        glDeleteVertexArrays(1, &debugLineVAO);
        debugLineVAO = 0;
    }

    DestroySceneTarget();

    defaultMaterial.reset();
    defaultMesh.reset();
    defaultSpriteSheet.reset();
    errorTexture.reset();
    defaultShader.reset();
    debugLineShader.reset();

    materialMap.clear();
    meshMap.clear();
    textureMap.clear();
    fontMap.clear();
    shaderMap.clear();
    spritesheetMap.clear();

    renderMap = {};
}

void RenderManager::ForceRegisterTexture(const std::string& tag, unsigned int id_, int width_, int height_, int channels_)
{
    if (textureMap.find(tag) != textureMap.end())
    {
        textureMap[tag]->ForceUpdateTexture(id_, width_, height_, channels_);;
        return;
    }
    textureMap[tag] = std::make_shared<Texture>(id_, width_, height_, channels_);
}

void RenderManager::ForceRegisterTexture(const std::string& tag, const unsigned char* data, int width_, int height_,
	int channels_, const TextureSettings& settings)
{
    if (textureMap.find(tag) != textureMap.end())
    {
        textureMap[tag]->ForceUpdateTexture(data, width_, height_, channels_,settings);
        return;
    }
    textureMap[tag] = std::make_shared<Texture>(data, width_, height_, channels_, settings);
}


RenderLayerManager& RenderManager::GetRenderLayerManager()
{
    return renderLayerManager;
}

void RenderManager::Init(const EngineContext& engineContext)
{
    auto shader = std::make_shared<Shader>();

    shader->AttachFromSource(ShaderStage::Vertex, R"(
        #version 460 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aUV;

        uniform mat4 u_Model;
        uniform mat4 u_View;
        uniform mat4 u_Projection;

        out vec2 v_TexCoord;

        void main()
        {
            v_TexCoord = aUV;
            gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 0.0, 1.0);
        }
    )");
    shader->AttachFromSource(ShaderStage::Fragment, R"(
        #version 460 core
        in vec2 v_TexCoord;
        out vec4 FragColor;

        uniform sampler2D u_FontTexture;
        uniform vec4 u_Color;

        void main()
        {
            float alpha = texture(u_FontTexture, v_TexCoord).r;
            FragColor = vec4(u_Color.rgb, alpha * u_Color.a);
        }
    )");
    shader->Link();
    shaderMap["[EngineShader]internal_text"] = shader;

    shader = std::make_shared<Shader>();
    shader->AttachFromSource(ShaderStage::Vertex, R"(
        #version 460 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec4 aColor;

        uniform mat4 u_View;
        uniform mat4 u_Projection;
        out vec4 vColor;

        void main()
        {
            vColor = aColor;
            gl_Position = u_Projection * u_View * vec4(aPos, 0.0, 1.0);
        }
    )");
    shader->AttachFromSource(ShaderStage::Fragment, R"(
        #version 460 core
        in vec4 vColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vColor;
        }
    )");
    shader->Link();
    shaderMap["[EngineShader]internal_debug_line"] = shader;
    debugLineShader = GetShaderByTag("[EngineShader]internal_debug_line");

    shader = std::make_shared<Shader>();
    shader->AttachFromSource(ShaderStage::Vertex, R"(
        #version 460 core

        layout (location = 0) in vec3 aPos;
        layout(location = 1) in vec2 a_UV;

        uniform mat4 u_Model;
        uniform mat4 u_View;
        uniform mat4 u_Projection;

        void main()
        {
            gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);
        }
    )");
    shader->AttachFromSource(ShaderStage::Fragment, R"(
        #version 460 core
        uniform vec4 u_Color;
        out vec4 FragColor;

        void main()
        {
            FragColor = u_Color;
        }
    )");
    shader->Link();
    shaderMap["[EngineShader]default"] = shader;
    defaultShader = shader;

    std::vector<unsigned char> errorTexturePixels;
    errorTexturePixels.reserve(8 * 8 * 4);

    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            bool isYellow = (x + y) % 2 == 0;
            if (isYellow)
            {
                errorTexturePixels.insert(errorTexturePixels.end(), { 255, 255, 0, 255 });
            }
            else
            {
                errorTexturePixels.insert(errorTexturePixels.end(), { 0, 0, 0, 255 });
            }
        }
    }

    RegisterTexture(
        "[EngineTexture]error",
        std::make_shared<Texture>(
            errorTexturePixels.data(),
            8,
            8,
            4,
            TextureSettings{
                TextureMinFilter::Nearest,
                TextureMagFilter::Nearest,
                TextureWrap::MirroredRepeat,
                TextureWrap::MirroredRepeat
            }
        )
    );
    errorTexture = GetTextureByTag("[EngineTexture]error");

    shader = std::make_shared<Shader>();
    shader->AttachFromSource(ShaderStage::Vertex, R"(
        #version 460 core

        layout (location = 0) in vec3 aPos;
        layout(location = 1) in vec2 a_UV;
        out vec2 v_UV;

        uniform mat4 u_Model;
        uniform mat4 u_View;
        uniform mat4 u_Projection;

        void main()
        {
            gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);
            v_UV = a_UV;
        }
    )");
    shader->AttachFromSource(ShaderStage::Fragment, R"(
        #version 460 core

        out vec4 FragColor;
        in vec2 v_UV;
        uniform vec4 u_Color;
        uniform sampler2D u_Texture;

        void main()
        {
            FragColor = texture(u_Texture, v_UV) * u_Color;
        }
    )");
    shader->Link();
    shaderMap["[EngineShader]default_texture"] = shader;

    std::shared_ptr<Material> material = std::make_shared<Material>(GetShaderByTag("[EngineShader]default_texture"));
    material->SetTexture("u_Texture", errorTexture);
    RegisterMaterial("[EngineMaterial]error", material);
    defaultMaterial = GetMaterialByTag("[EngineMaterial]error");

    RegisterMesh("[EngineMesh]default", std::vector<Vertex>{
        {{-0.5f, -0.5f, 0.f}, { 0.f, 0.f }},
        { { 0.5f, -0.5f, 0.f }, { 1.f, 0.f } },
        { { 0.5f, 0.5f, 0.f }, { 1.f, 1.f } },
        { { -0.5f, 0.5f, 0.f }, { 0.f, 1.f } }
    }, std::vector<unsigned int>{0, 1, 2, 2, 3, 0});
    defaultMesh = GetMeshByTag("[EngineMesh]default");

    RegisterSpriteSheet("[EngineSpriteSheet]default", "[EngineTexture]error", 1, 1);
    defaultSpriteSheet = GetSpriteSheetByTag("[EngineSpriteSheet]default");

    glCreateVertexArrays(1, &debugLineVAO);
    glCreateBuffers(1, &debugLineVBO);

    glBindVertexArray(debugLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 10000, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 2));

    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int w = engineContext.windowManager->GetWidth();
    int h = engineContext.windowManager->GetHeight();
    CreateSceneTarget(w, h);
}

namespace
{
    constexpr float DEPTH_SCALE = 1000.0f;
    inline int QuantizeDepth(float z)
    {
        return static_cast<int>(std::round(z * DEPTH_SCALE));
    }
}

void RenderManager::BuildRenderMap(const std::vector<Object*>& source, Camera2D* camera)
{
    renderCamera = camera;

    for (auto* obj : source)
    {
        if (!obj || !obj->IsVisible())
            continue;

        std::shared_ptr<Material> materialSP = obj->GetMaterial();
        std::shared_ptr<Mesh> meshSP = obj->GetMesh();

        SpriteAnimator* spriteAnimator = obj->GetSpriteAnimator();
        std::shared_ptr<SpriteSheet> spriteSheetSP = spriteAnimator ? spriteAnimator->GetSpriteSheet() : nullptr;

        Material* material = materialSP.get();
        Mesh* mesh = meshSP.get();
        SpriteSheet* spriteSheet = spriteSheetSP.get();
        Shader* shader = material ? material->GetShader().get() : nullptr;

        if (!material || !mesh || !shader)
            continue;

        uint8_t layer = renderLayerManager.GetLayerID(obj->GetRenderLayerTag()).value_or(0);
        if (layer >= RenderLayerManager::MAX_LAYERS)
        {
            JIN_WRN("render skipped - invalid layer\n");
            continue;
        }

        const float depth = obj->GetTransform2D().GetDepth();
        const int zbin = QuantizeDepth(depth);

        InstanceBatchKey key{ mesh, material, spriteSheet };
        auto& depthMap = renderMap[layer];
        auto& shaderMap = depthMap[zbin];
        shaderMap[shader][key].emplace_back(obj);
    }
}
/*
 * Usage:
 * renderManager.RegisterShader("basic", {
 * { ShaderStage::Vertex, "shaders/basic.vert" },
 * { ShaderStage::Fragment, "shaders/basic.frag" }
 * });
 */
void RenderManager::RegisterShader(const std::string& tag, const std::vector<std::pair<ShaderStage, FilePath>>& sources)
{
    if (shaderMap.find(tag) != shaderMap.end())
    {
        JIN_LOG("Shader with tag \"" << tag << "\" already registered.");
        return;
    }

    auto shader = std::make_shared<Shader>();

    for (const auto& [stage, path] : sources)
    {
        if (!shader->AttachFromFile(stage, path))
        {
            JIN_ERR("Failed to register shader [" << tag << "].");
            return;
        }
    }

    if (!shader->Link())
    {
        JIN_ERR("Failed to register shader [" << tag << "].");
        return;
    }

    shaderMap[tag] = shader;
}

void RenderManager::RegisterShader(const std::string& tag, std::shared_ptr<Shader> shader)
{
    if (shaderMap.find(tag) != shaderMap.end())
    {
        JIN_LOG("Shader with tag \"" << tag << "\" already registered.");
        return;
    }

    shaderMap[tag] = std::move(shader);
}

void RenderManager::RegisterTexture(const std::string& tag, const FilePath& path, const TextureSettings& settings)
{
    if (textureMap.find(tag) != textureMap.end())
    {
        JIN_LOG("Texture with tag \"" << tag << "\" already registered.");
        return;
    }

    textureMap[tag] = std::make_shared<Texture>(path, settings);
}

void RenderManager::RegisterTexture(const std::string& tag, std::shared_ptr<Texture> texture)
{
    if (textureMap.find(tag) != textureMap.end())
    {
        JIN_LOG("Texture with tag \"" << tag << "\" already registered.");
        return;
    }

    textureMap[tag] = std::move(texture);
}

void RenderManager::RegisterMesh(const std::string& tag, const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices, PrimitiveType primitiveType)
{
    if (meshMap.find(tag) != meshMap.end())
    {
        JIN_LOG("Mesh with tag \"" << tag << "\" already registered.");
        return;
    }

    meshMap[tag] = std::make_shared<Mesh>(vertices, indices, primitiveType);
}

void RenderManager::RegisterMesh(const std::string& tag, std::shared_ptr<Mesh> mesh)
{
    if (meshMap.find(tag) != meshMap.end())
    {
        JIN_LOG("Mesh with tag \"" << tag << "\" already registered.");
        return;
    }

    meshMap[tag] = std::move(mesh);
}

void RenderManager::RegisterMaterial(const std::string& tag, const std::string& shaderTag,
    const std::unordered_map<UniformName, TextureTag>& textureBindings)
{
    if (materialMap.find(tag) != materialMap.end())
    {
        JIN_LOG("Material tag already registered: " << tag);
        return;
    }

    std::shared_ptr<Shader> shader = GetShaderByTag(shaderTag);
    if (!shader)
    {
        JIN_WRN("Shader not found: " << shaderTag);
        return;
    }

    auto material = std::make_shared<Material>(shader);

    for (const auto& [uniformName, textureTag] : textureBindings)
    {
        auto it = textureMap.find(textureTag);
        if (it != textureMap.end())
        {
            material->SetTexture(uniformName, it->second);
        }
        else
        {
            JIN_WRN("Texture not found: " << textureTag);
        }
    }

    materialMap[tag] = material;
}

void RenderManager::RegisterMaterial(const std::string& tag, std::shared_ptr<Material> material)
{
    if (materialMap.find(tag) != materialMap.end())
    {
        JIN_LOG("Material tag already registered: " << tag);
        return;
    }

    materialMap[tag] = std::move(material);
}

void RenderManager::RegisterFont(const std::string& tag, const std::string& ttfPath, uint32_t pixelSize)
{
    if (fontMap.find(tag) != fontMap.end())
    {
        JIN_LOG("Font tag already registered: " << tag);
        return;
    }

    const uint32_t minSize = 4;
    const uint32_t maxSize = 64;

    if (pixelSize < minSize || pixelSize > maxSize)
    {
        JIN_ERR("Font pixelSize out of bounds: " << pixelSize << " (allowed: " << minSize << " - " << maxSize << ")");
        return;
    }

    auto font = std::make_shared<Font>(*this, ttfPath, pixelSize);
    fontMap[tag] = font;
}

void RenderManager::RegisterFont(const std::string& tag, std::shared_ptr<Font> font)
{
    if (fontMap.find(tag) != fontMap.end())
    {
        JIN_LOG("Font tag already registered: " << tag);
        return;
    }

    fontMap[tag] = std::move(font);
}

void RenderManager::RegisterRenderLayer(const std::string& tag, uint8_t layer)
{
    renderLayerManager.RegisterLayer(tag, layer);
}

void RenderManager::RegisterSpriteSheet(const std::string& tag, const std::string& textureTag, int frameW, int frameH)
{
    if (spritesheetMap.find(tag) != spritesheetMap.end())
    {
        JIN_LOG("SpriteSheet already registered: " << tag);
        return;
    }

    std::shared_ptr<Texture> texture = GetTextureByTag(textureTag);
    if (!texture)
    {
        JIN_ERR("Texture not found for SpriteSheet: " << textureTag);
        return;
    }

    spritesheetMap[tag] = std::make_shared<SpriteSheet>(texture, frameW, frameH);
}

void RenderManager::UnregisterShader(const std::string& tag, const EngineContext&)
{
    auto it = shaderMap.find(tag);
    if (it == shaderMap.end())
    {
        JIN_LOG("Cannot delete the shader [" << tag << "] because it was not found.");
        return;
    }

    if (it->second.use_count() > 1)
    {
        JIN_WRN("Shader [" << tag << "] is still in use! use_count = " << it->second.use_count());
        return;
    }

    shaderMap.erase(it);
}

void RenderManager::UnregisterTexture(const std::string& tag, const EngineContext&)
{
    auto it = textureMap.find(tag);
    if (it == textureMap.end())
    {
        JIN_LOG("Cannot delete the texture [" << tag << "] because it was not found.");
        return;
    }

    if (it->second.use_count() > 1)
    {
        JIN_WRN("Texture [" << tag << "] is still in use! use_count = " << it->second.use_count());
        return;
    }

    textureMap.erase(it);
}

void RenderManager::UnregisterMesh(const std::string& tag, const EngineContext&)
{
    auto it = meshMap.find(tag);
    if (it == meshMap.end())
    {
        JIN_LOG("Cannot delete the mesh [" << tag << "] because it was not found.");
        return;
    }

    if (it->second.use_count() > 1)
    {
        JIN_WRN("Mesh [" << tag << "] is still in use! use_count = " << it->second.use_count());
        return;
    }

    meshMap.erase(it);
}

void RenderManager::UnregisterMaterial(const std::string& tag, const EngineContext&)
{
    auto it = materialMap.find(tag);
    if (it == materialMap.end())
    {
        JIN_LOG("Cannot delete the material [" << tag << "] because it was not found.");
        return;
    }

    if (it->second.use_count() > 1)
    {
        JIN_WRN("Material [" << tag << "] is still in use! use_count = " << it->second.use_count());
        return;
    }

    materialMap.erase(it);
}

void RenderManager::UnregisterFont(const std::string& tag, const EngineContext&)
{
    auto it = fontMap.find(tag);
    if (it == fontMap.end())
    {
        JIN_LOG("Cannot delete the font [" << tag << "] because it was not found.");
        return;
    }

    if (it->second.use_count() > 1)
    {
        JIN_WRN("Font [" << tag << "] is still in use! use_count = " << it->second.use_count());
        return;
    }

    fontMap.erase(it);
}

void RenderManager::UnregisterSpriteSheet(const std::string& tag, const EngineContext&)
{
    auto it = spritesheetMap.find(tag);
    if (it == spritesheetMap.end())
    {
        JIN_LOG("Cannot delete the sprite sheet [" << tag << "] because it was not found.");
        return;
    }

    if (it->second.use_count() > 1)
    {
        JIN_WRN("SpriteSheet [" << tag << "] is still in use! use_count = " << it->second.use_count());
        return;
    }

    spritesheetMap.erase(it);
}

void RenderManager::UnregisterRenderLayer(const std::string& tag)
{
    renderLayerManager.UnregisterLayer(tag);
}



bool RenderManager::HasTexture(const std::string& tag) const
{
    return textureMap.find(tag) != textureMap.end();
}
bool RenderManager::HasShader(const std::string& tag)  const
{
    return shaderMap.find(tag) != shaderMap.end();
}
bool RenderManager::HasFont(const std::string& tag)    const
{
    return fontMap.find(tag) != fontMap.end();
}
bool RenderManager::HasSpriteSheet(const std::string& tag) const
{
    return spritesheetMap.find(tag) != spritesheetMap.end();
}

std::shared_ptr<SpriteSheet> RenderManager::GetSpriteSheetByTag(const std::string& tag)
{
    auto it = spritesheetMap.find(tag);
    if (it != spritesheetMap.end())
        return it->second;

    JIN_ERR("There is no SpriteSheet named '" << tag << "'");
    return defaultSpriteSheet;
}

std::shared_ptr<Shader> RenderManager::GetShaderByTag(const std::string& tag)
{
    auto it = shaderMap.find(tag);
    if (it != shaderMap.end())
        return it->second;

    JIN_ERR("There is no Shader named '" << tag << "'");
    return defaultShader;
}

std::shared_ptr<Texture> RenderManager::GetTextureByTag(const std::string& tag)
{
    auto it = textureMap.find(tag);
    if (it != textureMap.end())
        return it->second;

    JIN_ERR("There is no Texture named '" << tag << "'");
    return errorTexture;
}

std::shared_ptr<Mesh> RenderManager::GetMeshByTag(const std::string& tag)
{
    auto it = meshMap.find(tag);
    if (it != meshMap.end())
        return it->second;

    JIN_ERR("There is no Mesh named '" << tag << "'");
    return defaultMesh;
}

std::shared_ptr<Material> RenderManager::GetMaterialByTag(const std::string& tag)
{
    auto it = materialMap.find(tag);
    if (it != materialMap.end())
        return it->second;

    JIN_ERR("There is no Material named '" << tag << "'");
    return defaultMaterial;
}

std::shared_ptr<Font> RenderManager::GetFontByTag(const std::string& tag)
{
    auto it = fontMap.find(tag);
    if (it != fontMap.end())
        return it->second;

    JIN_ERR("There is no Font named '" << tag << "'");
    return nullptr; //todo: add default font later
}