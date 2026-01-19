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

*   `VelocityX`, `VelocityXPrev`: Horizontal velocity component.
*   `VelocityY`, `VelocityYPrev`: Vertical velocity component.
*   `Pressure`, `Divergence`: Pressure field and velocity divergence.
*   `SolidMask`: Solid mask (0.0 = fluid, 1.0 = solid).
*   `DyeDensity`: Passive scalar (smoke/dye) for visualization.

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

## 4. Algorithm Overview (Stable Fluids)

The solver implements Jos Stam's "Stable Fluids" algorithm, which consists of four major steps performed every frame:

1.  **Advection (Transport)**:
    This step moves quantities (velocity, density) through the grid based on the velocity field. We use a **Semi-Lagrangian** approach: for each cell center, we trace the velocity field backwards in time to find where the fluid particle came from, and interpolate the value from that location. This method is unconditionally stable, allowing for larger time steps.

2.  **Diffusion (Viscosity)**:
    Diffusion simulates the fluid's resistance to flow (viscosity). It spreads velocity values to neighboring cells. We solve this using an implicit method (Jacobi iteration) to ensure stability.

3.  **External Forces (Inflow)**:
    We apply external forces, such as the continuous inflow of wind from the left side of the domain, and enforce boundary conditions (walls and obstacles).

4.  **Projection (Incompressibility)**:
    Real fluids (like air at low speeds) are incompressible. The projection step ensures that mass is conserved (what flows in must flow out).
    *   **Compute Divergence**: Calculate how much fluid is expanding or compressing in each cell.
    *   **Solve Pressure**: Solve the Poisson equation ($\nabla^2 p = \nabla \cdot \mathbf{u}$) to find a pressure field that counteracts the divergence.
    *   **Subtract Gradient**: Subtract the pressure gradient from the velocity field to make it divergence-free.

---

## 5. Visualization & Shader Logic

The renderer uses OpenGL textures to upload grid data and display it on a full-screen quad.

### Shaders

1.  **`FluidVisualization` Shader**
    *   **Input**: Textures (`velocityXTexture`, `velocityYTexture`, `pressureTexture`, `solidMaskTexture`), Uniform (`displayMode`).
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
