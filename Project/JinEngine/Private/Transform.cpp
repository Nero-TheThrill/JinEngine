#include "Engine.h"

glm::mat4& Transform2D::GetMatrix()
{
    if (isChanged)
    {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
        glm::mat4 o = glm::translate(glm::mat4(1.0f), glm::vec3(offset.x, offset.y, 0.0f));
        glm::mat4 r = glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0, 0, 1));
        glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(scale, 1.0f));
        matrix = t * o * r * s;
        isChanged = false;
    }
    return matrix;
}
