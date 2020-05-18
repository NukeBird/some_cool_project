#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

class Camera
{
public:
    Camera()
    {
        Recalculate();
    }

    glm::vec3 getLeft() const noexcept { return { view[0][0], view[1][0], view[2][0] }; }
    glm::vec3 getUp() const noexcept { return { view[0][1], view[1][1], view[2][1] }; }
    glm::vec3 getForward() const noexcept { return { view[0][2], view[1][2], view[2][2] }; }

    const glm::vec3& getPosition() const noexcept { return position;  }
    const glm::quat& getRotation() const noexcept { return rotation; }
    const glm::mat4& getView() const noexcept { return view; }
    const glm::mat4& getProjection() const noexcept { return projection; }

    Camera& setRotation(const glm::quat& newRotation)
    {
        rotation = newRotation;
        Recalculate();

        return *this;
    }

    Camera& setView(const glm::mat4& newView)
    {
        glm::vec3 scale, skew;
        glm::vec4 perspective;
        glm::decompose(newView, scale, rotation, position, skew, perspective);

        Recalculate();

        return *this;
    }

    Camera& setProjection(const glm::mat4& newProjection)
    {
        projection = newProjection;

        return *this;
    }

private:
    void Recalculate()
    {
        //rotation = normalize(rotation);
        view = glm::translate(glm::mat4{1.0f}, position) * glm::mat4_cast(rotation);
        //glm::rotation()
    }

private:
    //primary values
    glm::vec3 position {0, 0, 0 };
    glm::quat rotation {1.0f, 0.0f, 0.0f, 0.0f};
    glm::mat4 projection {1.0};
    
    //derived values
    glm::mat4 view;
    
};