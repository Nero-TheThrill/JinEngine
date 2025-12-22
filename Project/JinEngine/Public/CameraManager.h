#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Camera2D.h"

/**
 * @brief Manages multiple Camera2D instances identified by string tags.
 *
 * @details
 * CameraManager owns Camera2D objects via std::unique_ptr and provides:
 * - Registration of cameras by tag
 * - Lookup by tag
 * - Active camera selection by tag
 * - Bulk screen size propagation (useful on window resize)
 *
 * The active camera is tracked by its tag (activeCameraTag).
 * If the active tag does not exist in the map, GetActiveCamera returns nullptr.
 */
class CameraManager
{
public:
    /**
     * @brief Constructs an empty CameraManager.
     *
     * @details
     * Initializes internal containers. No camera is active until
     * RegisterCamera and SetActiveCamera are used.
     */
    CameraManager();

    /**
     * @brief Registers (or replaces) a camera under the given tag.
     *
     * @details
     * The passed unique_ptr is moved into the internal map.
     * If the tag already exists, the previous camera is destroyed.
     *
     * @param tag String identifier used to store and retrieve the camera.
     * @param camera Owning pointer to the Camera2D instance.
     *
     * @code
     * CameraManager cameraManager;
     * cameraManager.RegisterCamera("Main", std::make_unique<Camera2D>());
     * cameraManager.SetActiveCamera("Main");
     * @endcode
     */
    void RegisterCamera(const std::string& tag, std::unique_ptr<Camera2D> camera)
    {
        cameraMap[tag] = std::move(camera);
    }

    /**
     * @brief Returns the camera associated with the given tag.
     *
     * @details
     * Returns nullptr if no camera is registered under the tag.
     *
     * @param tag String identifier of the camera to retrieve.
     * @return Camera2D pointer if found; otherwise nullptr.
     */
    [[nodiscard]] Camera2D* GetCamera(const std::string& tag) const
    {
        auto it = cameraMap.find(tag);
        return (it != cameraMap.end()) ? it->second.get() : nullptr;
    }

    /**
     * @brief Sets the active camera by tag if the tag exists.
     *
     * @details
     * If the tag is not found in cameraMap, this function does nothing
     * and the current activeCameraTag is preserved.
     *
     * @param tag Tag of the camera to make active.
     */
    void SetActiveCamera(const std::string& tag)
    {
        if (cameraMap.find(tag) != cameraMap.end())
        {
            activeCameraTag = tag;
        }
    }

    /**
     * @brief Returns the currently active camera.
     *
     * @details
     * This function performs a tag lookup using activeCameraTag.
     * If the active tag is not registered, nullptr is returned.
     *
     * @return Pointer to the active Camera2D, or nullptr.
     */
    [[nodiscard]] Camera2D* GetActiveCamera() const
    {
        return GetCamera(activeCameraTag);
    }

    /**
     * @brief Returns the tag string of the active camera.
     *
     * @return Reference to the active camera tag.
     */
    [[nodiscard]] const std::string& GetActiveCameraTag() const
    {
        return activeCameraTag;
    }

    /**
     * @brief Updates screen size on all registered cameras.
     *
     * @details
     * This is typically called when the window size changes.
     * Null camera pointers are skipped defensively.
     *
     * @param width New screen width in pixels.
     * @param height New screen height in pixels.
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
     * @brief Updates screen size on a specific camera by tag.
     *
     * @details
     * This function indexes cameraMap with the given tag.
     * If the stored pointer is non-null, SetScreenSize is applied.
     *
     * Note: Using operator[] on an unordered_map may insert a default value
     * if the key does not exist.
     *
     * @param tag Tag of the camera to update.
     * @param width New screen width in pixels.
     * @param height New screen height in pixels.
     */
    void SetScreenSize(const std::string& tag, int width, int height)
    {
        if (cameraMap[tag])
            cameraMap[tag]->SetScreenSize(width, height);
    }

    /**
     * @brief Removes all registered cameras.
     *
     * @details
     * Clears the internal map and destroys owned Camera2D instances.
     * The activeCameraTag string is not modified by this function.
     */
    void Clear()
    {
        cameraMap.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Camera2D>> cameraMap;
    std::string activeCameraTag;
};
