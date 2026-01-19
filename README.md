# 2D F1 Aerodynamic Flow Simulation - Design Document

## 1. Architecture Overview

The application follows a standard simulation-visualization loop structure, separated into three main modules.

### Modules

1.  **`Application` (Main Loop)**
    *   Initializes the window (GLFW) and OpenGL context.
    *   Manages the simulation time step ($\Delta t$).
    *   Handles user input (ImGui integration for UI).
    *   Orchestrates the `FluidSolver` updates and `Renderer` draw calls.

2.  **`FluidSolver` (Physics Engine)**
    *   Stores the simulation state (grids for Velocity, Pressure, Density, Obstacles).
    *   Implements the Stable Fluids algorithms (Advection, Diffusion, Projection).
    *   Handles boundary conditions and object interaction.

3.  **`Renderer` (Visualization)**
    *   Manages OpenGL resources (Textures, VAOs, Shaders).
    *   Maps solver state to visual representations (Heatmaps, Streamlines).

---

## 2. Grid Layout & Data Structures

We will use a **Colocated Grid** (Cell-Centered) for simplicity, where all variables ($u, v, p$, solid_mask) are defined at the center of each cell $(i, j)$.

### Grid Resolution
*   A fixed grid size, e.g., $128 \times 128$ or $256 \times 128$ depending on aspect ratio.
*   Physical domain size: $L_x \times L_y$.

### Data Arrays (Flat `std::vector<float>`)
The solver requires two buffers for most fields (current state and previous state) to handle implicit integration steps.

*   `u[size]`, `u_prev[size]`: Horizontal velocity component.
*   `v[size]`, `v_prev[size]`: Vertical velocity component.
*   `p[size]`, `div[size]`: Pressure and Divergence.
*   `s[size]`: Solid mask (0.0 = fluid, 1.0 = solid).
*   `dye[size]`: Passive scalar (smoke/dye) for basic visualization.

### Indexing
`index(i, j) = i + (N + 2) * j`
*(Includes a 1-cell boundary padding layer for simpler boundary logic)*

---

## 3. Boundary Handling

### Domain Boundaries (Walls)
*   **Inflow (Left)**: Constant velocity injection ($u = U_{in}, v = 0$).
*   **Outflow (Right)**: Zero-gradient (let flow leave freely) or Fixed Pressure ($p=0$).
*   **Top/Bottom**: Free-slip (velocity parallel to wall) or No-slip (velocity = 0).

### Internal Obstacles (F1 Profile)
The F1 profile is rasterized into the `s[]` array.
*   **Advection Step**: If a backtrace lands in a solid cell, clamp to boundary or use fluid velocity (0).
*   **Projection Step (Pressure)**:
    *   For the divergence calculation: Treat solid cell velocity as 0 (No-slip).
    *   For the pressure solve: Enforce $\frac{\partial p}{\partial n} = 0$ (Neumann BC) at solid fluid interfaces. This ensures no flux enters the solid.

---

## 4. Solver Pseudocode (Stable Fluids)

```cpp
void FluidSolver::Step(float dt) {
    // 1. Advection (Semi-Lagrangian)
    // Move velocity field along itself
    Advect(u, u_prev, u_prev, v_prev, dt);
    Advect(v, v_prev, u_prev, v_prev, dt);

    // 2. Diffusion (Implicit Jacobi)
    // Viscosity step (optional for low viscosity air, but stabilizes simulation)
    Diffuse(u, u_prev, viscosity, dt);
    Diffuse(v, v_prev, viscosity, dt);

    // 3. Force Application
    // Add external forces or reinforce inflow boundary
    ApplyInflow();

    // 4. Projection (Enforce Incompressibility)
    Project(u, v, p, div);

    // 5. Advect Dye (for visualization)
    Advect(dye, dye_prev, u, v, dt);
}
```

### The Projection Step
1.  **Compute Divergence**: $\nabla \cdot \mathbf{u}$
2.  **Solve Poisson Equation**: $\nabla^2 p = \nabla \cdot \mathbf{u}$ using Jacobi iteration.
    *   *Iterate 20-50 times.*
    *   Check `s[]` mask to handle obstacles.
3.  **Subtract Gradient**: $\mathbf{u}_{new} = \mathbf{u}_{old} - \nabla p$

---

## 5. Visualization & Shader Logic

The renderer uses OpenGL textures to upload grid data and display it on a full-screen quad.

### Shaders

1.  **`FluidVisualization` Shader**
    *   **Input**: Textures (`u_texture`, `v_texture`, `p_texture`, `mask_texture`), Uniform (`display_mode`).
    *   **Modes**:
        *   *Speed Heatmap*: `color = colormap(length(vec2(u, v)))`
        *   *Pressure*: `color = colormap(p)`
        *   *Curl/Vorticity*: `color = blue_red_map( d(v)/dx - d(u)/dy )`
        *   *Mask Overlay*: If `mask > 0.5`, output gray color (wing).

2.  **`Particles` (Streamlines)**
    *   **CPU**: Update a list of `Particle` positions using the current velocity grid (RK2 integration).
    *   **GPU**: Render as `GL_POINTS` or `GL_LINES` with fading trails.

---

## 6. Incremental Implementation Plan (2 Weeks)

### Week 1: Core Solver & Basic Rendering
*   **Day 1-2**: Project setup, Window, OpenGL boilerplate. Implement basic "Density Advection" to test the grid and Semi-Lagrangian backtrace.
*   **Day 3-4**: Implement the full Velocity Solver (Advection + Project). Get a box of fluid swirling.
*   **Day 5**: Add "Solid Mask" support. Implement the Jacobi solver adjustments for internal boundaries.
*   **Day 6-7**: Debugging. Visualize Velocity and Pressure.

### Week 2: Simulation Features & Polish
*   **Day 8**: Implement the "Inflow" boundary condition to simulate a wind tunnel.
*   **Day 9**: Load/Rasterize the F1 geometry (simple polygon or distance check) into the grid. Support Angle of Attack rotation.
*   **Day 10**: Implement Particle Streamlines (Lagrangian particles flowing through the Eulerian grid).
*   **Day 11**: UI (ImGui) to tweak parameters (Viscosity, Speed, Angle).
*   **Day 12-13**: Optimization and visual polish (Color palettes, nice rendering of the wing).
*   **Day 14**: Final cleanup and documentation.
```
