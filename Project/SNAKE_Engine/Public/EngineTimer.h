#pragma once

struct EngineTimer
{
    void Start();

    [[nodiscard]] float Tick();

    [[nodiscard]] bool ShouldUpdateFPS(float& outFPS);

    float lastTime = 0.0f;
    float fpsTimer = 0.0f;
    int frameCount = 0;
};
