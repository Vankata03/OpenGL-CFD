#include "Application.h"
#include "FluidSolver.h"
#include "Renderer.h"
#include "Geometry/Slicer.h"
#include "Geometry/SceneImporter.h"
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <iostream>

Application::Application(const std::string& title, int width, int height)
    : m_Title(title), m_WindowWidth(width), m_WindowHeight(height)
{
    Init();

    m_Solver = std::make_unique<FluidSolver>(256, 128);
    m_Renderer = std::make_unique<Renderer>();
    m_Slicer = std::make_unique<Slicer>(256, 128);
}

Application::~Application()
{
    Shutdown();
}

void Application::Init()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Enable forward compatibility - MacOS

    m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, m_Title.c_str(), NULL, NULL);
    if (!m_Window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Application::Shutdown()
{
    m_Mesh.reset();
    m_Slicer.reset();
    m_Renderer.reset();
    m_Solver.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    glfwTerminate();
}

void Application::Run()
{
    while (!glfwWindowShouldClose(m_Window)) {
        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - m_LastFrameTime;
        m_LastFrameTime = currentFrame;

        ProcessInput();

        if (!m_Paused) {
            Update(m_SimulationTimeStep);
        }

        Render();
        glfwPollEvents();
    }
}

void Application::ProcessInput()
{
    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_Window, true);
}

void Application::Update(float deltaTime)
{
    if (m_Solver) {
        m_Solver->Step(deltaTime);
    }
}

void Application::Render()
{
    int display_w, display_h;
    glfwGetFramebufferSize(m_Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // ViewProjection (2D Orthographic)
    glm::mat4 viewProjection;
    if (m_Solver)
        viewProjection = glm::ortho(0.0f, (float)m_Solver->GetWidth(), 0.0f, (float)m_Solver->GetHeight(), -1000.0f, 1000.0f);
    else
        viewProjection = glm::mat4(1.0f);

    // Render Simulation (Background)
    if (m_Renderer && m_Solver) {
        glDepthMask(GL_FALSE); // Draw background without writing depth
        m_Renderer->Draw(*m_Solver, display_w, display_h, viewProjection);
        glDepthMask(GL_TRUE);
    }

    // Render Mesh Preview
    if (m_Mesh && m_Renderer && m_Solver) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, m_MeshPosition);
        model = glm::scale(model, glm::vec3(m_MeshScale));
        model = glm::rotate(model, glm::radians(m_MeshRotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(m_MeshRotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(m_MeshRotation.z), glm::vec3(0, 0, 1));

        if (m_ShowMeshPreview) {
            m_Renderer->DrawMeshPreview(*m_Mesh, model, viewProjection, m_SliceZ, m_SliceThickness, m_MeshWireframe);
        }
        m_Renderer->DrawMeshViews(*m_Mesh, model, m_SliceZ, m_SliceThickness);
    }

    RenderUI();
    glfwSwapBuffers(m_Window);
}

void Application::RenderUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiIO& io = ImGui::GetIO();
        float width = 350.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - width, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, io.DisplaySize.y), ImGuiCond_Always);

        ImGui::Begin("Simulation Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        ImGui::Separator();

        ImGui::Checkbox("Pause", &m_Paused);
        ImGui::SliderFloat("Time Step", &m_SimulationTimeStep, 0.001f, 0.1f);

        if (ImGui::CollapsingHeader("Solver Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (m_Solver) {
                ImGui::SliderFloat("Viscosity", &m_Solver->m_Viscosity, 0.0f, 0.001f, "%.6f");
                ImGui::SliderFloat("Diffusion", &m_Solver->m_Diffusion, 0.0f, 0.001f, "%.6f");
                ImGui::SliderFloat("Inflow Velocity", &m_Solver->m_InflowVelocity, 0.0f, 5.0f);
                ImGui::SliderInt("Jacobi Iterations", &m_Solver->m_Iterations, 1, 100);

                if (ImGui::Button("Reset Obstacle")) {
                    m_Solver->InitObstacle();
                }
            }
        }

        if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (m_Renderer) {

                const char* modes[] = { "Dye", "Velocity", "Pressure" };
                int currentMode = (int)m_Renderer->m_CurrentMode;
                if (ImGui::Combo("Display Mode", &currentMode, modes, 3)) {
                    m_Renderer->m_CurrentMode = (Renderer::DisplayMode)currentMode;
                }
            }
        }

        if (ImGui::CollapsingHeader("Geometry Slicer", ImGuiTreeNodeFlags_DefaultOpen)) {
            static char filepath[128] = "assets/car.gltf";
            ImGui::InputText("File", filepath, 128);
            auto PerformSlice = [&]() {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, m_MeshPosition);
                model = glm::scale(model, glm::vec3(m_MeshScale));
                model = glm::rotate(model, glm::radians(m_MeshRotation.x), glm::vec3(1, 0, 0));
                model = glm::rotate(model, glm::radians(m_MeshRotation.y), glm::vec3(0, 1, 0));
                model = glm::rotate(model, glm::radians(m_MeshRotation.z), glm::vec3(0, 0, 1));

                if (m_Slicer && m_Mesh) {
                    std::vector<float> mask = m_Slicer->Capture(*m_Mesh, model, m_SliceZ, m_SliceThickness);
                    if (m_Solver) m_Solver->SetObstacleMask(mask);
                }
            };

            if (ImGui::Button("Load Mesh")) {
                m_Mesh = SceneImporter::LoadGLTF(filepath);
                if (!m_Mesh) std::cerr << "Failed to load mesh: " << filepath << std::endl;
                else PerformSlice();
            }

            if (m_Mesh) {
                ImGui::Checkbox("Show Preview Overlay", &m_ShowMeshPreview);
                if (m_ShowMeshPreview) {
                    ImGui::SameLine();
                    ImGui::Checkbox("Wireframe", &m_MeshWireframe);
                }

                ImGui::Separator();

                bool changed = false;
                changed |= ImGui::SliderFloat("Slice Offset", &m_SliceZ, -100.0f, 100.0f);
                changed |= ImGui::SliderFloat("Thickness", &m_SliceThickness, 0.01f, 50.0f);

                ImGui::Separator();
                ImGui::Text("Transform");
                changed |= ImGui::DragFloat3("Position", &m_MeshPosition.x, 1.0f);
                if (ImGui::Button("Center on Grid")) {
                    m_MeshPosition = glm::vec3(128.0f, 64.0f, 0.0f);
                    changed = true;
                }

                changed |= ImGui::DragFloat("Scale", &m_MeshScale, 0.1f, 0.1f, 1000.0f);
                changed |= ImGui::DragFloat3("Rotation", &m_MeshRotation.x, 1.0f, -360.0f, 360.0f);

                ImGui::Text("Orientation Presets:");
                if (ImGui::Button("Front")) {
                    m_MeshRotation = glm::vec3(0.0f, 0.0f, 0.0f);
                    if (m_Solver) m_Solver->m_FrontalSource = false;
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Top")) {
                    m_MeshRotation = glm::vec3(90.0f, 0.0f, 0.0f);
                    if (m_Solver) m_Solver->m_FrontalSource = false;
                    changed = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Side")) {
                    m_MeshRotation = glm::vec3(0.0f, 90.0f, 0.0f);
                    if (m_Solver) m_Solver->m_FrontalSource = true;
                    changed = true;
                }

                if (m_Renderer) {
                    ImGui::Text("Front View / Side View");
                    // Flip UVs to correct OpenGL texture orientation
                    ImGui::Image((void*)(intptr_t)m_Renderer->GetFrontViewTexture(), ImVec2(150, 150), ImVec2(0, 1), ImVec2(1, 0));
                    ImGui::SameLine();
                    ImGui::Image((void*)(intptr_t)m_Renderer->GetSideViewTexture(), ImVec2(150, 150), ImVec2(0, 1), ImVec2(1, 0));
                }

                if (changed) {
                    PerformSlice();
                }
            }
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
