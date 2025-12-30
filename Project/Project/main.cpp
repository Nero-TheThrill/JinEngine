#include <iostream>

#include "Debug.h"
#include "MainMenu.h"
#include "Engine.h"
#include "Tutorial.h"
#ifdef _DEBUG
//#include<vld.h>//TODO: remove this and directories before release (VC++ Directories -> Include Directories & Library Directories)
#endif
int main(int argc, char* argv[])
{
    JinEngine jinEngine;
    DebugLogger::SetLogLevel(LogLevel::All);
    int width = 1280;
    int height = 720;

    try
    {
        if (argc == 3)
        {
            width = std::stoi(argv[1]);
            height = std::stoi(argv[2]);
        }
        else if (argc != 1)
        {
            JIN_ERR("Usage: ./MyGame [width height]");
            return -1;
        }
    }
    catch (const std::exception& e)
    {
        JIN_ERR("Invalid arguments. Width and height must be integers.");
        return -1;
    }

    if (!jinEngine.Init(width, height))
    {
        JIN_ERR("Engine initialization failed.");
        return -1;
    }
    jinEngine.RenderDebugDraws(false);
   // jinEngine.GetEngineContext().windowManager->RestrictResizing(true);


    jinEngine.GetEngineContext().renderManager->RegisterMesh(
        "[Mesh]octagon",
        std::vector<Vertex>{
            {{  0.5f, 0.0f, 0.f }, { 1.0f,       0.5f }}, // 0 (0°)
            { {  0.353553f,     0.353553f,     0.f }, { 0.853553f,  0.853553f  } }, // 1 (45°)
            { {  0.0f,          0.5f,          0.f }, { 0.5f,       1.0f       } }, // 2 (90°)
            { { -0.353553f,     0.353553f,     0.f }, { 0.146447f,  0.853553f  } }, // 3 (135°)
            { { -0.5f,          0.0f,          0.f }, { 0.0f,       0.5f       } }, // 4 (180°)
            { { -0.353553f,    -0.353553f,     0.f }, { 0.146447f,  0.146447f  } }, // 5 (225°)
            { {  0.0f,         -0.5f,          0.f }, { 0.5f,       0.0f       } }, // 6 (270°)
            { {  0.353553f,    -0.353553f,     0.f }, { 0.853553f,  0.146447f  } }, // 7 (315°)
            { {  0.0f,          0.0f,          0.f }, { 0.5f,       0.5f       } }, // 8 (center)
    },
        std::vector<unsigned int>{
        8, 0, 1, 8, 1, 2, 8, 2, 3, 8, 3, 4,
            8, 4, 5, 8, 5, 6, 8, 6, 7, 8, 7, 0
    }
    );

    jinEngine.GetEngineContext().renderManager->RegisterShader("s_instancing", { {ShaderStage::Vertex,"Shaders/instancing.vert"},{ShaderStage::Fragment,"Shaders/instancing.frag"} });
    jinEngine.GetEngineContext().renderManager->RegisterShader("[Shader]Animation", { {ShaderStage::Vertex,"Shaders/Animation.vert"},{ShaderStage::Fragment,"Shaders/Animation.frag"} });
    jinEngine.GetEngineContext().renderManager->RegisterMaterial("[Material]Animation", "[Shader]Animation", { });

    //jinEngine.GetEngineContext().renderManager->RegisterMaterial("m_instancing", "s_instancing", { std::pair<std::string, std::string>("u_Texture","default") });
    //jinEngine.GetEngineContext().renderManager->RegisterMaterial("m_instancing1", "s_instancing", { std::pair<std::string, std::string>("u_Texture","default") });
    //jinEngine.GetEngineContext().renderManager->RegisterMaterial("m_blueMButton", "s_default1", { std::pair<std::string, std::string>("u_Texture","blueMButton") });


    jinEngine.GetEngineContext().renderManager->RegisterRenderLayer("[Layer]Background", 0);
    jinEngine.GetEngineContext().renderManager->RegisterRenderLayer("[Layer]Flag", 9);
    jinEngine.GetEngineContext().renderManager->RegisterRenderLayer("[Layer]Player", 10);
    jinEngine.GetEngineContext().renderManager->RegisterRenderLayer("[Layer]UI", 11);
    jinEngine.GetEngineContext().renderManager->RegisterRenderLayer("[Layer]UIText", 12);
    jinEngine.GetEngineContext().renderManager->RegisterRenderLayer("[Layer]FrameBufferTexture", 13);
    jinEngine.GetEngineContext().renderManager->RegisterRenderLayer("[Layer]Cursor", 14);
    jinEngine.GetEngineContext().renderManager->RegisterFont("[Font]default", "Fonts/NotoSans-VariableFont_wdth,wght.ttf", 50);
    jinEngine.GetEngineContext().renderManager->RegisterFont("[Font]defaultkr", "Fonts/NanumPenScript-Regular.ttf", 50);


    jinEngine.GetEngineContext().windowManager->SetCursorVisible(false);
    jinEngine.GetEngineContext().windowManager->SetBackgroundColor({ 0.2,0.2,0.4,1 });


    jinEngine.GetEngineContext().renderManager->RegisterTexture("[Texture]Cursor", "Textures/cursor.png");
    jinEngine.GetEngineContext().renderManager->RegisterMaterial("[Material]cursor", "[EngineShader]default_texture", { {"u_Texture","[Texture]Cursor"} });

    jinEngine.GetEngineContext().stateManager->ChangeState(std::make_unique<MainMenu>());

    jinEngine.Run();

    return 0;
}