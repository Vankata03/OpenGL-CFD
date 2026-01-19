#pragma once

#include <vector>

class FluidSolver {
public:
    // Grid resolution (including boundaries)
    FluidSolver(int width, int height);
    ~FluidSolver();

    void Step(float deltaTime);

    // Getters for Renderer
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    const std::vector<float>& GetVelocityX() const { return m_VelocityX; }
    const std::vector<float>& GetVelocityY() const { return m_VelocityY; }
    const std::vector<float>& GetPressure() const { return m_Pressure; }
    const std::vector<float>& GetSolidMask() const { return m_SolidMask; }
    const std::vector<float>& GetDyeDensity() const { return m_DyeDensity; }


    // Configuration
    void SetViscosity(float viscosity) { m_Viscosity = viscosity; }
    void SetDiffusion(float diffusion) { m_Diffusion = diffusion; }
    void SetInflowVelocity(float velocity) { m_InflowVelocity = velocity; }
    void InitObstacle();
    void SetObstacleMask(const std::vector<float>& mask);


    // Simulation Parameters public for UI
    float m_Viscosity = 0.000133f; // Low viscosity for air
    float m_Diffusion = 0.0f; // Low diffusion for dye
    float m_InflowVelocity = 1.6f;
    int m_Iterations = 40;    // Jacobi iterations
    bool m_FrontalSource = false;
    float m_DyeDecay = 0.01f;


private:
    void Advect(int boundaryType, std::vector<float>& dest, const std::vector<float>& source, const std::vector<float>& velocityX, const std::vector<float>& velocityY, float deltaTime);
    void Diffuse(int boundaryType, std::vector<float>& x, const std::vector<float>& xPrev, float diffusionRate, float deltaTime);
    void Project(std::vector<float>& velocityX, std::vector<float>& velocityY, std::vector<float>& pressure, std::vector<float>& divergence);
    
    // Boundary conditions
    // boundaryType: 0 = scalars, 1 = velocityX (horizontal), 2 = velocityY (vertical)
    void SetBoundaries(int boundaryType, std::vector<float>& x);

    // Helper for linear array access
    int GetIndex(int x, int y) const;

    // Inflow/Outflow specific handling
    void ApplyInflow();


private:
    int m_Width;
    int m_Height;
    int m_Size; // m_Width * m_Height

    // Fluid Fields (Current and Previous)
    std::vector<float> m_VelocityX, m_VelocityXPrev;
    std::vector<float> m_VelocityY, m_VelocityYPrev;
    std::vector<float> m_Pressure, m_Divergence; // Pressure and Divergence
    std::vector<float> m_DyeDensity, m_DyeDensityPrev; // Smoke/Dye for visualization

    // Solid Mask (0.0 = fluid, 1.0 = solid)
    std::vector<float> m_SolidMask;
};