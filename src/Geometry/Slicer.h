#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "../Shader.h"

class Slicer {
public:
    Slicer(int width, int height);
    ~Slicer();

    // Renders the mesh cross-section and returns a flat array (width * height)
    // where 1.0f = solid, 0.0f = empty.
    std::vector<float> Capture(Mesh& mesh, const glm::mat4& modelMatrix, float sliceZ, float thickness);

private:
    void InitResources();
    void CreateShader();

private:
    int m_Width;
    int m_Height;

    unsigned int m_FBO = 0;
    unsigned int m_Texture = 0; // The texture we render to
    Shader m_Shader;
};
