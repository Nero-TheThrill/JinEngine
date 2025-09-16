#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <array>

#include "Debug.h"

/**
 * @brief Manages mapping between custom render-layer names and numeric layer IDs.
 *
 * @details
 * This manager provides a bidirectional mapping between user-defined layer tags (names)
 * and small integer layer IDs in the range [0, MAX_LAYERS-1]. The mapping is unique:
 *   - each name maps to exactly one ID,
 *   - each ID can be bound to at most one name at a time.
 *
 * The maximum number of layers is limited to @ref MAX_LAYERS (default 16). Attempting to
 * register a duplicate layer name or to reuse an occupied/out-of-range ID will fail with
 * a debug log message.
 *
 * @note
 * Layer registration/unregistration is intended to be driven by RenderManager.
 * Calling code should prefer name-based APIs elsewhere and avoid hard-coding IDs.
 */
class RenderLayerManager
{
    friend RenderManager;
public:
    /**
     * @brief Maximum number of layers that can be registered at the same time.
     */
    static constexpr uint8_t MAX_LAYERS = 16;

    /**
     * @brief Looks up a layer ID by its name.
     *
     * @details
     * Returns an optional containing the layer ID if the name is registered,
     * or std::nullopt otherwise.
     *
     * @param name The registered layer name (tag), e.g. "[Layer]Background".
     * @return std::optional<uint8_t> The ID if found; std::nullopt if not found.
     * @code
     * if (auto idOpt = layerMgr.GetLayerID("[Layer]Player")) {
     *     uint8_t id = *idOpt;
     *     // use id ...
     * }
     * @endcode
     */
    [[nodiscard]] std::optional<uint8_t> GetLayerID(const std::string& name) const
    {
        auto it = nameToID.find(name);
        if (it != nameToID.end())
            return it->second;
        return std::nullopt;
    }

    /**
     * @brief Returns the layer name for a given ID.
     *
     * @details
     * Uses bounds-checked access. If the ID is valid but currently unassigned,
     * this returns an empty string.
     *
     * @param id The numeric layer ID in [0, MAX_LAYERS-1].
     * @return const std::string& The registered name (empty if unassigned).
     * @warning No existence check is performed beyond at(). Ensure the ID is within range.
     */
    [[nodiscard]] const std::string& GetLayerName(uint8_t id) const
    {
        return idToName.at(id);
    }

private:
    /**
     * @brief Registers a name->ID mapping for a render layer.
     *
     * @details
     * Fails if:
     *  - the name is already registered, or
     *  - the ID is out of range [0, MAX_LAYERS-1], or
     *  - the ID is already in use.
     *
     * On success, both forward and reverse maps are updated.
     *
     * @param tag Unique layer name (e.g., "[Layer]UI").
     * @param layer Numeric ID to bind to this name.
     * @return true on success; false if registration fails.
     */
    [[maybe_unused]] bool RegisterLayer(const std::string& tag, uint8_t layer)
    {
        if (nameToID.find(tag) != nameToID.end())
        {
            SNAKE_WRN("Layer already exists: " << tag);
            return false;
        }

        if (layer >= MAX_LAYERS || !idToName[layer].empty())
        {
            SNAKE_ERR("Layer ID " << layer << " is already in use or out of range");
            return false;
        }

        nameToID[tag] = layer;
        idToName[layer] = tag;
        return true;
    }

    /**
     * @brief Removes a previously registered layer name->ID mapping.
     *
     * @details
     * If the name is not registered, logs a message and returns.
     * On success, clears the reverse mapping slot and erases the name.
     *
     * @param name The registered layer name to remove.
     */
    void UnregisterLayer(const std::string& name)
    {
        auto it = nameToID.find(name);
        if (it == nameToID.end())
        {
            SNAKE_LOG("Cannot unregister: layer '" << name << "' not found");
            return;
        }

        uint8_t id = it->second;
        nameToID.erase(it);
        idToName[id].clear();
    }

    std::unordered_map<std::string, uint8_t> nameToID;        ///< Maps layer names to numeric IDs.
    std::array<std::string, MAX_LAYERS> idToName;             ///< Reverse map: ID-to-name (empty if unassigned).
};
