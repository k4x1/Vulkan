    #pragma once

    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>

    class Camera {
    public:
        Camera(float width, float height, const glm::vec3& position);

        void SetPosition(const glm::vec3& position);
        void SetOrientation(float yaw, float pitch);
        void SetPerspective(float fov, float nearPlane, float farPlane);

        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectionMatrix() const;

    private:
        glm::vec3 m_position;
        glm::vec3 m_orientation;
        glm::vec3 m_up;

        float m_width;
        float m_height;
        float m_fov;
        float m_nearPlane;
        float m_farPlane;

        glm::mat4 m_viewMatrix;
        glm::mat4 m_projectionMatrix;

        void UpdateViewMatrix();
        void UpdateProjectionMatrix();
    };
