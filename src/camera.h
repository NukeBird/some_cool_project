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

    glm::vec3 getLeft() const noexcept { return glm::vec3(1.0f, 0.0f, 0.0f) * orientation; }
    glm::vec3 getUp() const noexcept { return glm::vec3(0.0f, 1.0f, 0.0f) * orientation; }
    glm::vec3 getForward() const noexcept { return glm::vec3(0.0f, 0.0f, 1.0) * orientation; }

    const glm::vec3& getPosition() const noexcept { return position;  }
    const glm::mat4& getView() const noexcept { return view; }
    const glm::mat4& getProjection() const noexcept { return projection; }

    Camera& setView(const glm::mat4& newView)
    {
        glm::vec3 scale, skew;
        glm::vec4 perspective;
        glm::decompose(newView, scale, orientation, position, skew, perspective);

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
        view = glm::translate(glm::mat4{1.0f}, position) * glm::mat4_cast(orientation);
        //glm::rotation()
    }

private:
    //primary values
    glm::vec3 position {0, 0, 0 };
    glm::quat orientation {1.0f, 0.0f, 0.0f, 0.0f};
    glm::mat4 projection {1.0};
    
    //cached values
    glm::mat4 view;
    
};