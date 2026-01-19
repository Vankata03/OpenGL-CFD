#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();

    void Draw();

    void SetTexture(unsigned int textureID) { m_TextureID = textureID; }
    unsigned int GetTexture() const { return m_TextureID; }

private:
    unsigned int m_TextureID = 0;
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
    unsigned int m_IndexCount = 0;

    void SetupMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
};