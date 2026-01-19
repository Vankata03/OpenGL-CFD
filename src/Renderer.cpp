#include "Renderer.h"
#include "FluidSolver.h"
#include "Geometry/Mesh.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer()
{
    InitRenderData();
    CreateShader();
    CreateMeshShader();
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &m_QuadVAO);
    glDeleteBuffers(1, &m_QuadVBO);
    glDeleteProgram(m_ShaderProgram.ID);
    glDeleteProgram(m_MeshShader.ID);

    if (m_FrontViewFBO) glDeleteFramebuffers(1, &m_FrontViewFBO);
    if (m_FrontViewTexture) glDeleteTextures(1, &m_FrontViewTexture);
    if (m_SideViewFBO) glDeleteFramebuffers(1, &m_SideViewFBO);
    if (m_SideViewTexture) glDeleteTextures(1, &m_SideViewTexture);
}

void Renderer::DrawMeshPreview(const Mesh& mesh, const glm::mat4& model, const glm::mat4& projection, float sliceZ, float thickness, bool wireframe)
{
    m_MeshShader.use();

    m_MeshShader.setMat4("model", model);
    m_MeshShader.setMat4("projection", projection);
    m_MeshShader.setFloat("sliceZ", sliceZ);
    m_MeshShader.setFloat("thickness", thickness);
    m_MeshShader.setInt("meshTexture", 0);

    // Enable blending for ghost effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        ((Mesh&)mesh).Draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        // Draw back faces first for better transparency
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        ((Mesh&)mesh).Draw();

        // Draw front faces
        glCullFace(GL_BACK);
        ((Mesh&)mesh).Draw();
    }

    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
}

void Renderer::InitPreviewFBOs(int width, int height)
{
    if (m_PreviewWidth == width && m_PreviewHeight == height) return;
    m_PreviewWidth = width;
    m_PreviewHeight = height;

    auto setup = [&](unsigned int& fbo, unsigned int& tex) {
        if (fbo) glDeleteFramebuffers(1, &fbo);
        if (tex) glDeleteTextures(1, &tex);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };

    setup(m_FrontViewFBO, m_FrontViewTexture);
    setup(m_SideViewFBO, m_SideViewTexture);
}

void Renderer::DrawMeshViews(const Mesh& mesh, const glm::mat4& model, float sliceZ, float thickness)
{
    if (m_PreviewWidth == 0) InitPreviewFBOs(256, 256);

    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_fbo; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_fbo);

    glViewport(0, 0, m_PreviewWidth, m_PreviewHeight);

    auto drawView = [&](unsigned int fbo, const glm::mat4& viewProj) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_MeshShader.use();
        m_MeshShader.setMat4("model", model);
        m_MeshShader.setMat4("projection", viewProj);
        m_MeshShader.setFloat("sliceZ", sliceZ);
        m_MeshShader.setFloat("thickness", thickness);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        ((Mesh&)mesh).Draw();
        glCullFace(GL_BACK);
        ((Mesh&)mesh).Draw();

        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
    };

    // Front View (XZ Plane): Look from -Y
    glm::mat4 projFront = glm::ortho(0.0f, 256.0f, -128.0f, 128.0f, -1000.0f, 1000.0f) *
                          glm::lookAt(glm::vec3(0, -500, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));

    // Side View (YZ Plane): Look from +X
    glm::mat4 projSide = glm::ortho(0.0f, 128.0f, -64.0f, 64.0f, -1000.0f, 1000.0f) *
                         glm::lookAt(glm::vec3(500, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));

    drawView(m_FrontViewFBO, projFront);
    drawView(m_SideViewFBO, projSide);

    glBindFramebuffer(GL_FRAMEBUFFER, last_fbo);
    glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);
}

void Renderer::Draw(const FluidSolver& solver, int displayWidth, int displayHeight, const glm::mat4& viewProjection)
{
    int width = solver.GetWidth();
    int height = solver.GetHeight();

    if (m_GridWidth != width || m_GridHeight != height) {
        InitTextures(width, height);
    }

    UpdateTexture(m_TextureVelocityX, width, height, solver.GetVelocityX());
    UpdateTexture(m_TextureVelocityY, width, height, solver.GetVelocityY());
    UpdateTexture(m_TexturePressure, width, height, solver.GetPressure());
    UpdateTexture(m_TextureDyeDensity, width, height, solver.GetDyeDensity());
    UpdateTexture(m_TextureObstacleMask, width, height, solver.GetSolidMask());

    m_ShaderProgram.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureVelocityX);
    m_ShaderProgram.setInt("velocityXTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_TextureVelocityY);
    m_ShaderProgram.setInt("velocityYTexture", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_TexturePressure);
    m_ShaderProgram.setInt("pressureTexture", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_TextureDyeDensity);
    m_ShaderProgram.setInt("dyeDensityTexture", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_TextureObstacleMask);
    m_ShaderProgram.setInt("solidMaskTexture", 4);

    m_ShaderProgram.setInt("displayMode", (int)m_CurrentMode);
    m_ShaderProgram.setMat4("viewProjection", viewProjection);
    m_ShaderProgram.setVec2("gridSize", (float)width, (float)height);

    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::InitRenderData()
{
    // Quad coordinates (0 to 1) and Texture Coords (0 to 1)
    float quadVertices[] = {
        // positions   // texCoords
        0.0f,  1.0f,  0.0f, 1.0f,
        0.0f,  0.0f,  0.0f, 0.0f,
        1.0f,  0.0f,  1.0f, 0.0f,

        0.0f,  1.0f,  0.0f, 1.0f,
        1.0f,  0.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);

    glBindVertexArray(m_QuadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::InitTextures(int width, int height)
{
    m_GridWidth = width;
    m_GridHeight = height;

    auto setupTexture = [&](unsigned int& texID) {
        if (texID != 0) glDeleteTextures(1, &texID);

        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
    };

    setupTexture(m_TextureVelocityX);
    setupTexture(m_TextureVelocityY);
    setupTexture(m_TexturePressure);
    setupTexture(m_TextureDyeDensity);
    setupTexture(m_TextureObstacleMask);
}

void Renderer::UpdateTexture(GLuint textureID, int width, int height, const std::vector<float>& data)
{
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, data.data());
}

void Renderer::CreateShader()
{
    m_ShaderProgram.Load("src/Shaders/fluid.vert", "src/Shaders/fluid.frag");
}

void Renderer::CreateMeshShader()
{
    m_MeshShader.Load("src/Shaders/mesh.vert", "src/Shaders/mesh.frag");
}
