#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class FluidSolver;
class Mesh;

class Renderer {
public:
    Renderer();
    ~Renderer();

    void Draw(const FluidSolver& solver, int displayWidth, int displayHeight, const glm::mat4& viewProjection);
    void DrawMeshPreview(const Mesh& mesh, const glm::mat4& model, const glm::mat4& projection, float sliceZ, float thickness, bool wireframe);

    void InitPreviewFBOs(int width, int height);
    void DrawMeshViews(const Mesh& mesh, const glm::mat4& model, float sliceZ, float thickness);
    unsigned int GetFrontViewTexture() const { return m_FrontViewTexture; }
    unsigned int GetSideViewTexture() const { return m_SideViewTexture; }

    enum class DisplayMode {
        Dye = 0,
        Velocity = 1,
        Pressure = 2
    };

    DisplayMode m_CurrentMode = DisplayMode::Velocity;

private:
    void InitRenderData();
    void InitTextures(int width, int height);
    void UpdateTexture(GLuint textureID, int width, int height, const std::vector<float>& data);
    void CreateShader();
    void CreateMeshShader();

private:
    unsigned int m_QuadVAO = 0;
    unsigned int m_QuadVBO = 0;
    Shader m_ShaderProgram;
    Shader m_MeshShader;

    unsigned int m_FrontViewFBO = 0;
    unsigned int m_FrontViewTexture = 0;
    unsigned int m_SideViewFBO = 0;
    unsigned int m_SideViewTexture = 0;
    int m_PreviewWidth = 0;
    int m_PreviewHeight = 0;

    // Textures for mapping grid data
    unsigned int m_TextureVelocityX = 0;
    unsigned int m_TextureVelocityY = 0;
    unsigned int m_TexturePressure = 0;
    unsigned int m_TextureDyeDensity = 0;
    unsigned int m_TextureObstacleMask = 0;

    int m_GridWidth = 0;
    int m_GridHeight = 0;
};
