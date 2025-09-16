/**
 * @brief Key structure used for grouping objects into instanced batches.
 *
 * @details
 * The batch key is defined by a unique combination of:
 * - Mesh pointer
 * - Material pointer
 * - SpriteSheet pointer
 *
 * Objects that share the same key can be drawn together using GPU instancing.
 */
struct InstanceBatchKey
{
    Mesh* mesh;          ///< Shared mesh pointer.
    Material* material;  ///< Shared material pointer.
    SpriteSheet* spriteSheet; ///< Shared sprite sheet pointer (for animated sprites).

    /**
     * @brief Equality comparison operator.
     *
     * @details
     * Two keys are considered equal if all three pointers (mesh, material, spriteSheet) match.
     *
     * @param other Key to compare against.
     * @return true if all members are equal.
     */
    bool operator==(const InstanceBatchKey& other) const
    {
        return mesh == other.mesh && material == other.material && spriteSheet == other.spriteSheet;
    }

    /**
     * @brief Ordering operator for associative containers.
     *
     * @details
     * Comparison is lexicographical by mesh, then material, then spriteSheet pointer addresses.
     *
     * @param other Key to compare against.
     * @return true if this key is ordered before @p other.
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
     * @brief Custom hash specialization for InstanceBatchKey.
     *
     * @details
     * Combines the hashes of mesh, material, and spriteSheet pointers.
     * This enables use of InstanceBatchKey as a key in unordered_map.
     */
    template<>
    struct hash<InstanceBatchKey>
    {
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
 * @brief Map type for grouping objects by layer and batch key.
 *
 * @details
 * The outer map is keyed by layer index (int).
 * The inner map groups GameObjects by their @ref InstanceBatchKey.
 *
 * This allows RenderManager to quickly build per-layer instanced draw calls.
 */
using InstancedBatchMap = std::unordered_map<int, std::unordered_map<InstanceBatchKey, std::vector<GameObject*>>>;
