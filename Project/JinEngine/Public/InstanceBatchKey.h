#pragma once
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"
#include <unordered_map>
#include <vector>

class SpriteSheet;

/**
 * @brief Key type used to group objects into the same instanced rendering batch.
 *
 * @details
 * InstanceBatchKey identifies whether multiple objects can share the same draw batch
 * during instanced rendering.
 *
 * The current key is composed of:
 * - Mesh*       : geometry resource
 * - Material*   : material/shader/texture binding state
 * - SpriteSheet*: optional sprite animation source
 *
 * Objects that share the same mesh, material, and sprite sheet pointer can be grouped
 * together into one batch entry and rendered more efficiently through instancing.
 *
 * @note
 * This key compares raw pointers only. It does not compare the contents of Mesh,
 * Material, or SpriteSheet.
 */
struct InstanceBatchKey
{
    /**
     * @brief Pointer to the mesh resource used by the batch.
     */
    Mesh* mesh;

    /**
     * @brief Pointer to the material resource used by the batch.
     */
    Material* material;

    /**
     * @brief Pointer to the sprite sheet resource used by the batch.
     *
     * @details
     * This may be nullptr for non-animated objects or objects that do not rely on
     * a sprite-sheet based animation workflow.
     */
    SpriteSheet* spriteSheet;

    /**
     * @brief Returns whether two batch keys represent the same instancing group.
     *
     * @details
     * Two keys are considered equal only when all three stored pointers match exactly:
     * mesh, material, and spriteSheet.
     *
     * This operator is primarily used by unordered associative containers together
     * with the std::hash specialization below.
     *
     * @param other Key to compare against.
     * @return True if all pointer fields are identical.
     */
    bool operator==(const InstanceBatchKey& other) const
    {
        return mesh == other.mesh && material == other.material && spriteSheet == other.spriteSheet;
    }

    /**
     * @brief Provides a strict ordering between two batch keys.
     *
     * @details
     * Ordering is determined lexicographically by comparing:
     * 1. mesh pointer
     * 2. material pointer
     * 3. spriteSheet pointer
     *
     * This operator is useful when InstanceBatchKey is used in ordered containers
     * or when deterministic key ordering is desired.
     *
     * @param other Key to compare against.
     * @return True if this key should be ordered before the other key.
     */
    bool operator<(const InstanceBatchKey& other) const
    {
        if (mesh != other.mesh) return mesh < other.mesh;
        if (material != other.material) return material < other.material;
        return spriteSheet < other.spriteSheet;
    }
};

namespace std
{
    /**
     * @brief Hash specialization for InstanceBatchKey.
     *
     * @details
     * This specialization enables InstanceBatchKey to be used as a key in
     * std::unordered_map and other unordered associative containers.
     *
     * The current implementation hashes the three raw pointer fields separately and
     * combines them using xor and bit shifting.
     *
     * @note
     * As with InstanceBatchKey itself, hashing is based on pointer identity, not on
     * the pointed-to object contents.
     */
    template<>
    struct hash<InstanceBatchKey>
    {
        /**
         * @brief Computes the hash value for an instancing batch key.
         *
         * @param key Key whose hash should be computed.
         * @return Combined hash of mesh, material, and spriteSheet pointers.
         */
        std::size_t operator()(const InstanceBatchKey& key) const noexcept
        {
            std::size_t h1 = std::hash<Mesh*>()(key.mesh);
            std::size_t h2 = std::hash<Material*>()(key.material);
            std::size_t h3 = std::hash<SpriteSheet*>()(key.spriteSheet);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

/**
 * @brief Nested unordered map type used to store instanced rendering batches by depth and batch key.
 *
 * @details
 * The outer int key typically represents a depth bucket, z-order bucket, or another
 * integer-based grouping level chosen by the render pipeline.
 *
 * The inner unordered_map groups objects by InstanceBatchKey, meaning objects that
 * share the same mesh, material, and sprite sheet are collected into the same vector.
 *
 * In other words, this structure can be interpreted as:
 *
 * - first level: integer grouping (such as depth)
 * - second level: shared instancing resource key
 * - value: list of GameObject pointers belonging to that batch
 *
 * This type is intended to make instanced draw submission easier by pre-grouping
 * objects that can be rendered together.
 */
using InstancedBatchMap = std::unordered_map<int, std::unordered_map<InstanceBatchKey, std::vector<GameObject*>>>;