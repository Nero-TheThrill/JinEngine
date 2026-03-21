#pragma once
#include "glm.hpp"

class Transform2D
{
public:
    Transform2D()
        : position(0.f), rotation(0.f), scale(1.f),
        matrix(1.f), isChanged(true), depth(0.f)
    {
    }

    void SetPosition(const glm::vec2& pos)
    {
        position = pos;
        isChanged = true;
    }

    void AddPosition(const glm::vec2& pos)
    {
        position += pos;
        isChanged = true;
    }

    void SetRotation(float rot)
    {
        rotation = rot;
        isChanged = true;
    }

    void AddRotation(float rot)
    {
        rotation += rot;
        isChanged = true;
    }

    void SetScale(const glm::vec2& scl)
    {
        scale = scl;
        isChanged = true;
    }

    void AddScale(const glm::vec2& scl)
    {
        scale += scl;
        isChanged = true;
    }

    void SetDepth(float dpth)
    {
        depth = dpth;
    }

    void SetOffset(glm::vec2 os)
    {
        offset = os;
    }

    [[nodiscard]] const glm::vec2& GetPosition() const { return position; }

    [[nodiscard]] float GetRotation() const { return rotation; }

    [[nodiscard]] const glm::vec2& GetScale() const { return scale; }

    [[nodiscard]] float GetDepth() const { return depth; }

    [[nodiscard]] glm::vec2 GetOffset() const { return offset; }

    [[nodiscard]] glm::mat4& GetMatrix();

private:
    glm::vec2 position;
    glm::vec2 offset;
    float depth;
    float rotation;
    glm::vec2 scale;
    glm::mat4 matrix;
    bool isChanged;
};
