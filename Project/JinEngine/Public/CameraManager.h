#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Camera2D.h"

/**
 * @brief Manages multiple Camera2D instances using string tags.
 *
 * @details
 * CameraManager is responsible for storing, retrieving, and managing multiple Camera2D objects.
 * Cameras are identified by string tags and stored internally using unique_ptr for ownership safety.
 *
 * Upon construction, a default camera with the tag "main" is automatically created and set as active.
 * The manager allows switching between cameras, resizing all cameras, and clearing all registered cameras.
 *
 * @note
 * The manager owns all Camera2D instances. External code should not delete cameras manually.
 */
class CameraManager
{
public:
    /**
     * @brief Constructs the CameraManager and initializes a default camera.
     *
     * @details
     * Creates a default Camera2D with screen size (1,1), sets its zoom to 1.0,
     * and registers it under the tag "main". This camera is also set as the active camera.
     *
     * @note
     * This ensures that there is always at least one valid camera available.
     */
    CameraManager();

    /**
     * @brief Registers a new camera with a tag.
     *
     * @details
     * Takes ownership of the given Camera2D instance and stores it in the internal map.
     * If a camera with the same tag already exists, it will be overwritten.
     *
     * @param tag Unique string identifier for the camera.
     * @param camera Unique pointer to the Camera2D instance.
     */
    void RegisterCamera(const std::string& tag, std::unique_ptr<Camera2D> camera)
    {
        cameraMap[tag] = std::move(camera);
    }

    /**
     * @brief Retrieves a camera by its tag.
     *
     * @details
     * Searches the internal camera map and returns a raw pointer to the camera.
     * Returns nullptr if the tag does not exist.
     *
     * @param tag The tag of the camera to retrieve.
     * @return Pointer to the Camera2D instance, or nullptr if not found.
     */
    [[nodiscard]] Camera2D* GetCamera(const std::string& tag) const
    {
        auto it = cameraMap.find(tag);
        return (it != cameraMap.end()) ? it->second.get() : nullptr;
    }

    /**
     * @brief Sets the active camera using its tag.
     *
     * @details
     * Updates the currently active camera only if the provided tag exists in the map.
     * If the tag is not found, the active camera remains unchanged.
     *
     * @param tag The tag of the camera to set as active.
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
     * Internally calls GetCamera using the stored activeCameraTag.
     *
     * @return Pointer to the active Camera2D, or nullptr if not found.
     */
    [[nodiscard]] Camera2D* GetActiveCamera() const
    {
        return GetCamera(activeCameraTag);
    }

    /**
     * @brief Returns the tag of the currently active camera.
     *
     * @return Reference to the active camera tag string.
     */
    [[nodiscard]] const std::string& GetActiveCameraTag() const
    {
        return activeCameraTag;
    }

    /**
     * @brief Sets screen size for all registered cameras.
     *
     * @details
     * Iterates through all cameras and updates their screen size.
     * Used typically when the window is resized.
     *
     * @param width New screen width.
     * @param height New screen height.
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
     * @brief Sets screen size for a specific camera.
     *
     * @details
     * Updates only the camera associated with the given tag.
     *
     * @param tag Target camera tag.
     * @param width New screen width.
     * @param height New screen height.
     */
    void SetScreenSize(const std::string& tag, int width, int height)
    {
        auto it = cameraMap.find(tag);
        if (it != cameraMap.end() && it->second)
            it->second->SetScreenSize(width, height);
    }

    /**
     * @brief Clears all registered cameras.
     *
     * @details
     * Removes all cameras from the internal map.
     * This will destroy all Camera2D instances managed by this class.
     *
     * @note
     * After calling this, activeCameraTag may become invalid.
     */
    void Clear()
    {
        cameraMap.clear();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Camera2D>> cameraMap;
    std::string activeCameraTag;
};