#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <array>

#include "Debug.h"

/**
 * @brief Stores the bidirectional mapping between render layer names and fixed layer IDs.
 *
 * @details
 * RenderLayerManager is a small internal registry used by RenderManager to manage
 * the engine's named render layers.
 *
 * Each layer is identified by:
 * - a string tag used by user-facing code and objects
 * - a compact uint8_t layer ID used internally for ordering and batching
 *
 * The manager maintains two synchronized lookup containers:
 * - nameToID : resolves a layer name to its numeric ID
 * - idToName : resolves a numeric ID back to its layer name
 *
 * The current design supports at most MAX_LAYERS entries, and each layer ID can
 * only be assigned once. Registration fails if:
 * - the name already exists
 * - the requested layer ID is out of range
 * - the requested layer ID is already occupied
 *
 * In the current engine flow, RenderManager owns this manager and uses it when:
 * - registering a new render layer by tag
 * - unregistering a layer
 * - resolving an object's render layer tag during draw batching
 *
 * @note
 * This class is primarily an internal helper for RenderManager rather than a
 * high-level standalone system.
 */
class RenderLayerManager
{
    friend RenderManager;

public:
    /**
     * @brief Maximum number of render layers supported by the manager.
     *
     * @details
     * Layer IDs are valid only in the range [0, MAX_LAYERS - 1].
     * The current engine design limits the total number of named render layers to 16.
     */
    static constexpr uint8_t MAX_LAYERS = 16;

    /**
     * @brief Returns the numeric layer ID associated with a given layer name.
     *
     * @details
     * This function performs a lookup in the internal name-to-ID table.
     * If the layer name exists, the corresponding ID is returned inside std::optional.
     * If the name is not registered, std::nullopt is returned.
     *
     * RenderManager uses this during render submission to resolve
     * Object::GetRenderLayerTag() into a numeric layer value.
     *
     * @param name Layer name to search for.
     * @return The layer ID if found, otherwise std::nullopt.
     */
    [[nodiscard]] std::optional<uint8_t> GetLayerID(const std::string& name) const
    {
        auto it = nameToID.find(name);
        if (it != nameToID.end())
            return it->second;
        return std::nullopt;
    }

    /**
     * @brief Returns the layer name associated with a numeric layer ID.
     *
     * @details
     * This function accesses the internal fixed-size ID-to-name table and returns
     * the stored layer name for the given ID.
     *
     * Unlike GetLayerID(), this function does not return an optional value.
     * It uses std::array::at(), which means an out-of-range ID will throw.
     *
     * @param id Numeric layer ID to resolve.
     * @return Reference to the registered layer name for the given ID.
     *
     * @note
     * This function assumes the caller provides a valid layer index.
     */
    [[nodiscard]] const std::string& GetLayerName(uint8_t id) const
    {
        return idToName.at(id);
    }

private:
    /**
     * @brief Registers a new layer name at a specific numeric layer ID.
     *
     * @details
     * This is an internal registration function used by RenderManager.
     * A registration succeeds only when:
     * - the tag is not already registered
     * - the requested layer ID is smaller than MAX_LAYERS
     * - the requested layer slot is currently empty
     *
     * On success:
     * - nameToID[tag] is assigned
     * - idToName[layer] is assigned
     *
     * On failure, the function logs a warning or error and returns false.
     *
     * @param tag Layer name to register.
     * @param layer Numeric layer ID to reserve for the name.
     * @return True if registration succeeded, otherwise false.
     *
     * @note
     * Because this function is private, normal access is expected to happen
     * through RenderManager::RegisterRenderLayer().
     */
    [[maybe_unused]] bool RegisterLayer(const std::string& tag, uint8_t layer)
    {
        if (nameToID.find(tag) != nameToID.end())
        {
            JIN_WRN("Layer already exists: " << tag);
            return false;
        }

        if (layer >= MAX_LAYERS || !idToName[layer].empty())
        {
            JIN_ERR("Layer ID " << layer << " is already in use or out of range");
            return false;
        }

        nameToID[tag] = layer;
        idToName[layer] = tag;
        return true;
    }

    /**
     * @brief Unregisters an existing render layer by name.
     *
     * @details
     * If the requested layer name exists:
     * - the name-to-ID entry is removed
     * - the corresponding ID slot in idToName is cleared
     *
     * If the name does not exist, the function logs a message and returns without
     * modifying the registry.
     *
     * @param name Layer name to unregister.
     *
     * @note
     * Because this function is private, normal access is expected to happen
     * through RenderManager::UnregisterRenderLayer().
     */
    void UnregisterLayer(const std::string& name)
    {
        auto it = nameToID.find(name);
        if (it == nameToID.end())
        {
            JIN_LOG("Cannot unregister: layer '" << name << "' not found");
            return;
        }

        uint8_t id = it->second;
        nameToID.erase(it);
        idToName[id].clear();
    }

    std::unordered_map<std::string, uint8_t> nameToID;
    std::array<std::string, MAX_LAYERS> idToName;
};