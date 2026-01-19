#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

struct GLFWwindow;

class FluidSolver;
class Renderer;
class Mesh;
class Slicer;

class Application {
public:
    Application(const std::string& title, int width, int height);
    ~Application();

    void Run();

private:
    void Init();
    void Shutdown();
    
    void ProcessInput();
    void Update(float dt);
    void Render();
    void RenderUI();

private:
    GLFWwindow* m_Window = nullptr;
    int m_WindowWidth;
    int m_WindowHeight;
    std::string m_Title;

    std::unique_ptr<FluidSolver> m_Solver;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<Mesh> m_Mesh;
    std::unique_ptr<Slicer> m_Slicer;

    // Slicing settings
    float m_SliceZ = 0.0f;
    float m_SliceThickness = 2.0f;
    float m_MeshScale = 10.0f;
    glm::vec3 m_MeshRotation = glm::vec3(90.0f, 0.0f, 0.0f);
    glm::vec3 m_MeshPosition = glm::vec3(100.0f, 62.0f, 0.0f);
    bool m_ShowMeshPreview = true;
    bool m_MeshWireframe = true;

    // Camera settings
    bool m_3DMode = false;
    float m_CameraYaw = -90.0f;
    float m_CameraPitch = -45.0f;
    float m_CameraDistance = 300.0f;
    glm::vec3 m_CameraCenter = glm::vec3(128.0f, 64.0f, 0.0f);
    
    // Mouse state
    double m_LastMouseX = 0.0;
    double m_LastMouseY = 0.0;
    bool m_MouseDragging = false;

    // Simulation settings
    bool m_Paused = false;
    float m_SimulationTimeStep = 0.01f; // Simulation time step
    
    // For calculating delta time
    float m_LastFrameTime = 0.0f;
};