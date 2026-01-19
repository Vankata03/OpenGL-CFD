#include "FluidSolver.h"
#include <algorithm>
#include <cmath>

FluidSolver::FluidSolver(int width, int height)
    : m_Width(width), m_Height(height), m_Size(width * height)
{
    m_VelocityX.resize(m_Size, 0.0f);
    m_VelocityXPrev.resize(m_Size, 0.0f);
    m_VelocityY.resize(m_Size, 0.0f);
    m_VelocityYPrev.resize(m_Size, 0.0f);
    m_Pressure.resize(m_Size, 0.0f);
    m_Divergence.resize(m_Size, 0.0f);
    m_DyeDensity.resize(m_Size, 0.0f);
    m_DyeDensityPrev.resize(m_Size, 0.0f);
    m_SolidMask.resize(m_Size, 0.0f); // 0.0 = fluid
    InitObstacle();
}

FluidSolver::~FluidSolver()
{

}

int FluidSolver::GetIndex(int x, int y) const
{
    x = std::max(0, std::min(x, m_Width - 1));
    y = std::max(0, std::min(y, m_Height - 1));
    return x + y * m_Width;
}

void FluidSolver::Step(float dt)
{
    std::swap(m_VelocityX, m_VelocityXPrev);
    std::swap(m_VelocityY, m_VelocityYPrev);

    // Diffuse velocity (Viscosity)
    Diffuse(1, m_VelocityX, m_VelocityXPrev, m_Viscosity, dt);
    Diffuse(2, m_VelocityY, m_VelocityYPrev, m_Viscosity, dt);

    // Compute Pressure and remove divergence
    Project(m_VelocityX, m_VelocityY, m_Pressure, m_Divergence);

    std::swap(m_VelocityX, m_VelocityXPrev);
    std::swap(m_VelocityY, m_VelocityYPrev);

    // Advect velocity
    Advect(1, m_VelocityX, m_VelocityXPrev, m_VelocityXPrev, m_VelocityYPrev, dt);
    Advect(2, m_VelocityY, m_VelocityYPrev, m_VelocityXPrev, m_VelocityYPrev, dt);

    // Project again to keep it mass-conserving
    Project(m_VelocityX, m_VelocityY, m_Pressure, m_Divergence);

    // Advect and Diffuse Dye
    std::swap(m_DyeDensity, m_DyeDensityPrev);
    Diffuse(0, m_DyeDensity, m_DyeDensityPrev, m_Diffusion, dt);

    std::swap(m_DyeDensity, m_DyeDensityPrev);
    Advect(0, m_DyeDensity, m_DyeDensityPrev, m_VelocityX, m_VelocityY, dt);

    // Apply forces and inflow
    ApplyInflow();
}

void FluidSolver::Advect(int boundaryType, std::vector<float>& destField, const std::vector<float>& sourceField,
                        const std::vector<float>& velocityX, const std::vector<float>& velocityY, float deltaTime)
{
    float dt0_x = deltaTime * (m_Width - 2);
    float dt0_y = deltaTime * (m_Height - 2);

    for (int j = 1; j < m_Height - 1; j++) {
        for (int i = 1; i < m_Width - 1; i++) {

            if (m_SolidMask[GetIndex(i, j)] > 0.0f) {
                destField[GetIndex(i, j)] = 0.0f;
                continue;
            }

            // Backtrace
            float x = i - dt0_x * velocityX[GetIndex(i, j)];
            float y = j - dt0_y * velocityY[GetIndex(i, j)];

            // Clamp to grid
            if (x < 0.5f) x = 0.5f;
            if (x > m_Width - 1.5f) x = m_Width - 1.5f;
            if (y < 0.5f) y = 0.5f;
            if (y > m_Height - 1.5f) y = m_Height - 1.5f;

            // Bilinear interpolation indices
            int cellLeft = (int)x;
            int cellRight = cellLeft + 1;
            int cellBottom = (int)y;
            int cellTop = cellBottom + 1;

            // Interpolation weights
            float lerpWeightRight = x - cellLeft;
            float lerpWeightLeft = 1.0f - lerpWeightRight;
            float lerpWeightTop = y - cellBottom;
            float lerpWeightBottom = 1.0f - lerpWeightTop;

            // Sample source field
            destField[GetIndex(i, j)] =
                lerpWeightLeft * (lerpWeightBottom * sourceField[GetIndex(cellLeft, cellBottom)] + lerpWeightTop * sourceField[GetIndex(cellLeft, cellTop)]) +
                lerpWeightRight * (lerpWeightBottom * sourceField[GetIndex(cellRight, cellBottom)] + lerpWeightTop * sourceField[GetIndex(cellRight, cellTop)]);
        }
    }
    SetBoundaries(boundaryType, destField);
}

void FluidSolver::Diffuse(int boundaryType, std::vector<float>& destField, const std::vector<float>& sourceField, float diffRate, float deltaTime)
{
    // Diffusion coefficient used by the Gauss-Seidel/Jacobi relaxation
    float diffusionCoefficient = deltaTime * diffRate * (m_Width - 2) * (m_Height - 2);

    for (int k = 0; k < m_Iterations; k++) {
        for (int j = 1; j < m_Height - 1; j++) {
            for (int i = 1; i < m_Width - 1; i++) {

                if (m_SolidMask[GetIndex(i, j)] > 0.0f) continue;

                float valLeft   = destField[GetIndex(i - 1, j)];
                float valRight  = destField[GetIndex(i + 1, j)];
                float valBottom = destField[GetIndex(i, j - 1)];
                float valTop    = destField[GetIndex(i, j + 1)];

                // Handle obstacle boundaries for diffusion
                if (boundaryType == 0) {
                    // Scalar diffusion simply extends value at boundary (no flux into solid)
                    if (m_SolidMask[GetIndex(i - 1, j)] > 0.0f) valLeft   = destField[GetIndex(i, j)];
                    if (m_SolidMask[GetIndex(i + 1, j)] > 0.0f) valRight  = destField[GetIndex(i, j)];
                    if (m_SolidMask[GetIndex(i, j - 1)] > 0.0f) valBottom = destField[GetIndex(i, j)];
                    if (m_SolidMask[GetIndex(i, j + 1)] > 0.0f) valTop    = destField[GetIndex(i, j)];
                } else {
                    // Velocity diffusion assumes no-slip (zero velocity inside solid)
                    if (m_SolidMask[GetIndex(i - 1, j)] > 0.0f) valLeft   = 0.0f;
                    if (m_SolidMask[GetIndex(i + 1, j)] > 0.0f) valRight  = 0.0f;
                    if (m_SolidMask[GetIndex(i, j - 1)] > 0.0f) valBottom = 0.0f;
                    if (m_SolidMask[GetIndex(i, j + 1)] > 0.0f) valTop    = 0.0f;
                }

                // Jacobi iteration step
                destField[GetIndex(i, j)] = (sourceField[GetIndex(i, j)] + diffusionCoefficient * (valLeft + valRight + valBottom + valTop)) / (1 + 4 * diffusionCoefficient);
            }
        }
        SetBoundaries(boundaryType, destField);
    }
}

void FluidSolver::Project(std::vector<float>& velocX, std::vector<float>& velocY, std::vector<float>& pressure, std::vector<float>& divergence)
{
    float h = 1.0f / m_Width;

    // Divergence
    for (int j = 1; j < m_Height - 1; j++) {
        for (int i = 1; i < m_Width - 1; i++) {

            if (m_SolidMask[GetIndex(i, j)] > 0.0f) {
                divergence[GetIndex(i, j)] = 0.0f;
                pressure[GetIndex(i, j)] = 0.0f;
                continue;
            }

            divergence[GetIndex(i, j)] = -0.5f * h * (velocX[GetIndex(i + 1, j)] - velocX[GetIndex(i - 1, j)] +
                                                velocY[GetIndex(i, j + 1)] - velocY[GetIndex(i, j - 1)]);
            pressure[GetIndex(i, j)] = 0;
        }
    }

    SetBoundaries(0, divergence);
    SetBoundaries(3, pressure); // 3 = Pressure specific boundary

    // Solve Pressure (Poisson equation)
    for (int k = 0; k < m_Iterations; k++) {
        for (int j = 1; j < m_Height - 1; j++) {
            for (int i = 1; i < m_Width - 1; i++) {

                if (m_SolidMask[GetIndex(i, j)] > 0.0f) continue;

                // Neumann boundary condition at obstacles
                float pLeft   = (m_SolidMask[GetIndex(i - 1, j)] > 0.0f) ? pressure[GetIndex(i, j)] : pressure[GetIndex(i - 1, j)];
                float pRight  = (m_SolidMask[GetIndex(i + 1, j)] > 0.0f) ? pressure[GetIndex(i, j)] : pressure[GetIndex(i + 1, j)];
                float pBottom = (m_SolidMask[GetIndex(i, j - 1)] > 0.0f) ? pressure[GetIndex(i, j)] : pressure[GetIndex(i, j - 1)];
                float pTop    = (m_SolidMask[GetIndex(i, j + 1)] > 0.0f) ? pressure[GetIndex(i, j)] : pressure[GetIndex(i, j + 1)];

                pressure[GetIndex(i, j)] = (divergence[GetIndex(i, j)] + pLeft + pRight + pBottom + pTop) / 4.0f;
            }
        }
        SetBoundaries(3, pressure);
    }

    // Subtract Gradient from Velocity
    for (int j = 1; j < m_Height - 1; j++) {
        for (int i = 1; i < m_Width - 1; i++) {

            if (m_SolidMask[GetIndex(i, j)] > 0.0f) {
                velocX[GetIndex(i, j)] = 0.0f;
                velocY[GetIndex(i, j)] = 0.0f;
                continue;
            }

            float pLeft   = (m_SolidMask[GetIndex(i - 1, j)] > 0.0f) ? pressure[GetIndex(i, j)] : pressure[GetIndex(i - 1, j)];
            float pRight  = (m_SolidMask[GetIndex(i + 1, j)] > 0.0f) ? pressure[GetIndex(i, j)] : pressure[GetIndex(i + 1, j)];
            float pBottom = (m_SolidMask[GetIndex(i, j - 1)] > 0.0f) ? pressure[GetIndex(i, j)] : pressure[GetIndex(i, j - 1)];
            float pTop    = (m_SolidMask[GetIndex(i, j + 1)] > 0.0f) ? pressure[GetIndex(i, j)] : pressure[GetIndex(i, j + 1)];

            velocX[GetIndex(i, j)] -= 0.5f * (pRight - pLeft) / h;
            velocY[GetIndex(i, j)] -= 0.5f * (pTop - pBottom) / h;
        }
    }

    SetBoundaries(1, velocX);
    SetBoundaries(2, velocY);
}

void FluidSolver::SetBoundaries(int boundaryType, std::vector<float>& field)
{
    for (int i = 1; i < m_Width - 1; i++) {
        // Top and Bottom walls
        field[GetIndex(i, 0)]             = (boundaryType == 2) ? -field[GetIndex(i, 1)] : field[GetIndex(i, 1)];
        field[GetIndex(i, m_Height - 1)]  = (boundaryType == 2) ? -field[GetIndex(i, m_Height - 2)] : field[GetIndex(i, m_Height - 2)];
    }

    for (int j = 1; j < m_Height - 1; j++) {
        // Left and Right walls
        if (boundaryType == 3) {
             // Special case for Pressure: Fixed pressure at outflow (Right wall)
            field[GetIndex(0, j)] = field[GetIndex(1, j)];
            field[GetIndex(m_Width - 1, j)] = 0.0f;
        } else {
            field[GetIndex(0, j)] = field[GetIndex(1, j)];
            field[GetIndex(m_Width - 1, j)] = field[GetIndex(m_Width - 2, j)];
        }
    }

    // Corners (average of neighbors)
    field[GetIndex(0, 0)]                      = 0.5f * (field[GetIndex(1, 0)] + field[GetIndex(0, 1)]);
    field[GetIndex(0, m_Height - 1)]           = 0.5f * (field[GetIndex(1, m_Height - 1)] + field[GetIndex(0, m_Height - 2)]);
    field[GetIndex(m_Width - 1, 0)]            = 0.5f * (field[GetIndex(m_Width - 2, 0)] + field[GetIndex(m_Width - 1, 1)]);
    field[GetIndex(m_Width - 1, m_Height - 1)] = 0.5f * (field[GetIndex(m_Width - 2, m_Height - 1)] + field[GetIndex(m_Width - 1, m_Height - 2)]);
}

void FluidSolver::ApplyInflow()
{
    if (m_FrontalSource) {
        // Displacement Flow: Emit fluid from the surface of the object outwards
        for (int j = 1; j < m_Height - 1; j++) {
            for (int i = 1; i < m_Width - 1; i++) {
                if (m_SolidMask[GetIndex(i, j)] == 0.0f) {
                    float normalX = 0.0f;
                    float normalY = 0.0f;
                    bool isBoundary = false;

                    // Check neighbors to determine normal direction away from solid
                    if (m_SolidMask[GetIndex(i - 1, j)] > 0.0f) { normalX += 1.0f; isBoundary = true; }
                    if (m_SolidMask[GetIndex(i + 1, j)] > 0.0f) { normalX -= 1.0f; isBoundary = true; }
                    if (m_SolidMask[GetIndex(i, j - 1)] > 0.0f) { normalY += 1.0f; isBoundary = true; }
                    if (m_SolidMask[GetIndex(i, j + 1)] > 0.0f) { normalY -= 1.0f; isBoundary = true; }

                    if (isBoundary) {
                        float length = std::sqrt(normalX * normalX + normalY * normalY);
                        if (length > 0.0f) {
                            normalX /= length;
                            normalY /= length;

                            float speed = 2.0f;
                            m_VelocityX[GetIndex(i, j)] = normalX * speed;
                            m_VelocityY[GetIndex(i, j)] = normalY * speed;
                            m_DyeDensity[GetIndex(i, j)] = 1.0f;
                        }
                    }
                }
            }
        }
        return;
    }

    // Override left boundary for wind tunnel effect
    for (int j = 1; j < m_Height - 1; j++) {
        m_VelocityX[GetIndex(0, j)] = m_InflowVelocity;
        m_VelocityX[GetIndex(1, j)] = m_InflowVelocity;
        m_VelocityY[GetIndex(0, j)] = 0.0f;
        m_VelocityY[GetIndex(1, j)] = 0.0f;

        // Emitter
        if (j > m_Height * 0.45f && j < m_Height * 0.55f) {
             m_DyeDensity[GetIndex(0, j)] = 1.0f;
             m_DyeDensity[GetIndex(1, j)] = 1.0f;
        } else {
             m_DyeDensity[GetIndex(0, j)] = 0.0f;
        }
    }
}

void FluidSolver::InitObstacle()
{
    std::fill(m_SolidMask.begin(), m_SolidMask.end(), 0.0f);

    int centerX = m_Width / 3;
    int centerY = m_Height / 2;
    int chordLength = m_Width / 4;
    float thickness = 0.15f;

    for (int j = 0; j < m_Height; j++) {
        for (int i = 0; i < m_Width; i++) {
            float localX = (float)(i - centerX) / chordLength;
            float localY = (float)(j - centerY) / chordLength;

            // Simple NACA 4-digit symmetrical airfoil approximation (NACA 00xx)
            // yt = 5 * t * (0.2969*sqrt(x) - 0.1260*x - 0.3516*x^2 + 0.2843*x^3 - 0.1015*x^4)
            if (localX >= 0.0f && localX <= 1.0f) {
                float yt = 5.0f * thickness * (0.2969f * std::sqrt(localX) - 0.1260f * localX - 0.3516f * localX * localX + 0.2843f * localX * localX * localX - 0.1015f * localX * localX * localX * localX);
                if (std::abs(localY) <= yt) {
                    m_SolidMask[GetIndex(i, j)] = 1.0f;

                    // Clear velocity inside obstacle
                    m_VelocityX[GetIndex(i, j)] = 0.0f;
                    m_VelocityY[GetIndex(i, j)] = 0.0f;
                }
            }
        }
    }
}

void FluidSolver::SetObstacleMask(const std::vector<float>& mask)
{
    if (mask.size() != m_Size) return;
    m_SolidMask = mask;

    // Clear velocity inside obstacle
    for (int i = 0; i < m_Size; i++) {
        if (m_SolidMask[i] > 0.0f) {
            m_VelocityX[i] = 0.0f;
            m_VelocityY[i] = 0.0f;
        }
    }
}
