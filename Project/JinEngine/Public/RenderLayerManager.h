#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <array>

#include "Debug.h"

/**
 * @brief Maintains a bidirectional mapping between human-readable render-layer names and numeric layer IDs.
 *
 * @details
 * RenderLayerManager provides a fixed-capacity layer namespace where users can register up to MAX_LAYERS
 * custom render layers. A layer is identified externally by a string tag (name), and internally by a
 * compact uint8_t ID used for fast indexing into RenderManager's per-layer render structures.
 *
 * The mapping is stored in two containers:
 * - nameToID: std::unordered_map<string, uint8_t> mapping layer name -> layer ID.
 * - idToName: std::array<string, MAX_LAYERS> mapping layer ID -> layer name.
 *
 * Registration constraints:
 * - A name can only be registered once.
 * - An ID can only be assigned once.
 * - layer IDs must be in [0, MAX_LAYERS).
 *
 * RegisterLayer() and UnregisterLayer() are private and intended to be driven by RenderManager to keep
 * layer registration consistent with the renderer's expectations.
 */
class RenderLayerManager
{
    friend RenderManager;
public:
    /**
     * @brief Maximum number of user-definable render layers.
     *
     * @details
     * RenderManager::RenderMap is sized using this constant.
     */
    static constexpr uint8_t MAX_LAYERS = 16;

    /**
     * @brief Returns the numeric layer ID for a registered layer name.
     *
     * @details
     * If the name is not registered, std::nullopt is returned.
     *
     * @param name Layer name/tag to resolve.
     * @return Optional layer ID.
     */
    [[nodiscard]] std::optional<uint8_t> GetLayerID(const std::string& name) const
    {
        auto it = nameToID.find(name);
        if (it != nameToID.end())
            return it->second;
        return std::nullopt;
    }

    /**
     * @brief Returns the layer name for a given layer ID.
     *
     * @details
     * This uses std::array::at() and will throw if the ID is out of range.
     * If the ID is valid but currently unused, the returned string will be empty.
     *
     * @param id Numeric layer ID.
     * @return Reference to the layer name stored for the ID.
     */
    [[nodiscard]] const std::string& GetLayerName(uint8_t id) const
    {
        return idToName.at(id);
    }

private:
    /**
     * @brief Registers a new layer name to a specific numeric ID.
     *
     * @details
     * Fails when:
     * - The name is already registered.
     * - The numeric ID is out of range.
     * - The numeric ID is already in use (idToName[layer] is not empty).
     *
     * On success, both nameToID and idToName are updated to preserve the bidirectional mapping.
     *
     * @param tag   New layer name/tag to register.
     * @param layer Numeric layer ID to associate with the tag.
     * @return True if the layer was registered; false otherwise.
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
     * @brief Unregisters an existing layer mapping by name.
     *
     * @details
     * If the name does not exist, the function logs and returns without modifying state.
     * If the name exists, the entry is removed from nameToID and the corresponding slot in
     * idToName is cleared, making the numeric ID available for future registrations.
     *
     * @param name Layer name/tag to unregister.
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
