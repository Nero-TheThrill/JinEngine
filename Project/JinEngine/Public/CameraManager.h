#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Camera2D.h"

class CameraManager
{
public:
    CameraManager();

    void RegisterCamera(const std::string& tag, std::unique_ptr<Camera2D> camera)
    {
        cameraMap[tag] = std::move(camera);
    }

    [[nodiscard]] Camera2D* GetCamera(const std::string& tag) const
    {
        auto it = cameraMap.find(tag);
        return (it != cameraMap.end()) ? it->second.get() : nullptr;
    }

    void SetActiveCamera(const std::string& tag)
    {
        if (cameraMap.find(tag) != cameraMap.end())
        {
            activeCameraTag = tag;
        }
    }

    [[nodiscard]] Camera2D* GetActiveCamera() const
    {
        return GetCamera(activeCameraTag);
    }

    [[nodiscard]] const std::string& GetActiveCameraTag() const
    {
        return activeCameraTag;
    }

    void SetScreenSizeForAll(int width, int height)
    {
        for (auto& [tag, cam] : cameraMap)
        {
            if (cam)
                cam->SetScreenSize(width, height);
        }
    }

    void SetScreenSize(const std::string& tag, int width, int height)
    {
        auto it = cameraMap.find(tag);
        if (it != cameraMap.end() && it->second)
            it->second->SetScreenSize(width, height);
    }

    void Clear()
    {
        cameraMap.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Camera2D>> cameraMap;
    std::string activeCameraTag;
};
