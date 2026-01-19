#include "Slicer.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

Slicer::Slicer(int width, int height)
    : m_Width(width), m_Height(height)
{
    InitResources();
    CreateShader();
}

Slicer::~Slicer()
{
    glDeleteFramebuffers(1, &m_FBO);
    glDeleteTextures(1, &m_Texture);
    glDeleteProgram(m_Shader.ID);
}

void Slicer::InitResources()
{
    // Create Framebuffer
    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    // Create Texture Attachment
    glGenTextures(1, &m_Texture);
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_Width, m_Height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Texture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::SLICER::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<float> Slicer::Capture(Mesh& mesh, const glm::mat4& modelMatrix, float sliceZ, float thickness)
{
    // Save current state
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_fbo; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_Width, m_Height);

    // Clear to black (empty fluid)
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_Shader.use();

    // Projection matching grid coordinates exactly (0,0) to (width, height)
    glm::mat4 projection = glm::ortho(0.0f, (float)m_Width, 0.0f, (float)m_Height, -1000.0f, 1000.0f);

    m_Shader.setMat4("projection", projection);
    m_Shader.setMat4("model", modelMatrix);
    m_Shader.setFloat("sliceZ", sliceZ);
    m_Shader.setFloat("thickness", thickness);

    // Disable culling to ensure backfaces are rendered (solid object might be cut open)
    glDisable(GL_CULL_FACE);

    mesh.Draw();

    glEnable(GL_CULL_FACE);

    std::vector<float> pixels(m_Width * m_Height);
    glReadPixels(0, 0, m_Width, m_Height, GL_RED, GL_FLOAT, pixels.data());

    // Restore state
    glBindFramebuffer(GL_FRAMEBUFFER, last_fbo);
    glViewport(last_viewport[0], last_viewport[1], last_viewport[2], last_viewport[3]);

    return pixels;
}

void Slicer::CreateShader()
{
    m_Shader.Load("src/Shaders/slicer.vert", "src/Shaders/slicer.frag");
}
