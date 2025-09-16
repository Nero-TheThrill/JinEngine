#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Camera2D.h"

/**
 * @brief Manages multiple 2D cameras and the active camera selection.
 *
 * @details
 * The CameraManager owns a set of named Camera2D instances and tracks which
 * camera is currently active. Cameras are registered by string tag and stored
 * with unique ownership. The manager also propagates screen size changes to
 * cameras (either to all, or to a specific one by tag).
 *
 * Lifecycle & defaults:
 * - The constructor creates a default camera with tag "main" (see implementation)
 *   and marks it active.
 *
 * Threading:
 * - This class is not thread-safe; access it from the engine's main thread.
 */
class CameraManager
{
public:
    /**
     * @brief Construct a camera manager.
     *
     * @details
     * Implementation initializes a default "main" camera and sets it active.
     */
    CameraManager();

    /**
     * @brief Register (or replace) a camera under a tag.
     *
     * @param tag      Unique string identifier for the camera.
     * @param camera   Ownership of the camera to store.
     *
     * @note If a camera already exists with the same tag, it will be replaced.
     */
    void RegisterCamera(const std::string& tag, std::unique_ptr<Camera2D> camera)
    {
        cameraMap[tag] = std::move(camera);
    }

    /**
     * @brief Retrieve a camera by tag.
     *
     * @param tag  String identifier used during registration.
     * @return Pointer to the camera if found, otherwise nullptr.
     *
     * @note The returned pointer is non-owning and remains valid while the
     *       camera is registered in this manager.
     */
    [[nodiscard]] Camera2D* GetCamera(const std::string& tag) const
    {
        auto it = cameraMap.find(tag);
        return (it != cameraMap.end()) ? it->second.get() : nullptr;
    }

    /**
     * @brief Set the active camera by tag.
     *
     * @param tag  Tag of the camera to activate.
     *
     * @note If the tag does not exist, the active camera is not changed.
     */
    void SetActiveCamera(const std::string& tag)
    {
        if (cameraMap.find(tag) != cameraMap.end())
        {
            activeCameraTag = tag;
        }
    }

    /**
     * @brief Get the currently active camera.
     *
     * @return Pointer to the active Camera2D, or nullptr if none is active.
     */
    [[nodiscard]] Camera2D* GetActiveCamera() const
    {
        return GetCamera(activeCameraTag);
    }

    /**
     * @brief Get the tag of the currently active camera.
     *
     * @return String reference naming the active camera tag.
     */
    [[nodiscard]] const std::string& GetActiveCameraTag() const
    {
        return activeCameraTag;
    }

    /**
     * @brief Apply screen size (in pixels) to all registered cameras.
     *
     * @param width   New screen width.
     * @param height  New screen height.
     *
     * @details
     * Invokes Camera2D::SetScreenSize on each stored camera if it exists.
     */
    void SetScreenSizeForAll(int width, int height)
    {
        for (auto& [tag, cam] : cameraMap)
        {
            if (cam)
                cam->SetScreenSize(width, height);
        }
    }

    /**
     * @brief Apply screen size (in pixels) to a specific camera by tag.
     *
     * @param tag     Target camera tag.
     * @param width   New screen width.
     * @param height  New screen height.
     *
     * @note If the tag does not exist or is null, the call is ignored.
     */
    void SetScreenSize(const std::string& tag, int width, int height)
    {
        if (cameraMap[tag])
            cameraMap[tag]->SetScreenSize(width, height);
    }

    /**
     * @brief Remove all registered cameras and reset the manager.
     *
     * @details
     * Clears the internal map. The active tag string is preserved, but will
     * no longer resolve to a camera until one is registered under the same tag.
     */
    void Clear()
    {
        cameraMap.clear();
    }

private:
    /** @brief Map of camera tags to their owned Camera2D instances. */
    std::unordered_map<std::string, std::unique_ptr<Camera2D>> cameraMap;

    /** @brief Tag of the currently active camera (e.g., "main"). */
    std::string activeCameraTag;
};
