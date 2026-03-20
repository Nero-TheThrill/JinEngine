#pragma once
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"
#include <unordered_map>
#include <vector>

class SpriteSheet;

struct InstanceBatchKey
{
    Mesh* mesh;
    Material* material;
    SpriteSheet* spriteSheet;

    bool operator==(const InstanceBatchKey& other) const
    {
        return mesh == other.mesh && material == other.material && spriteSheet == other.spriteSheet;
    }

    bool operator<(const InstanceBatchKey& other) const
    {
        if (mesh != other.mesh) return mesh < other.mesh;
        if (material != other.material) return material < other.material;
        return spriteSheet < other.spriteSheet;
    }
};

namespace std
{
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

using InstancedBatchMap = std::unordered_map<int, std::unordered_map<InstanceBatchKey, std::vector<GameObject*>>>;
