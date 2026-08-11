# ZeroCancerReactor: GPU-Accelerated Tumor Microenvironment (TME) Simulator

## Overview
ZeroCancerReactor is a highly optimized, GPU-bound cellular automaton and biological simulation framework. It is designed to model the Tumor Microenvironment (TME) and host-tumor interactions at a large scale (up to 70 million concurrent cellular agents). The simulation utilizes CUDA for parallel processing and relies on mathematical models, stochastic distributions, and a Proportional-Integral-Derivative (PID) controller to evaluate cellular state transitions, immune system responses, and metabolic homeostasis.

The primary objective of this computational model is to study localized Lotka-Volterra predator-prey dynamics between oncogenic cells and the host immune system, specifically exploring theoretical states of symbiosis regulated by automated biological feedback loops.

## Technical Details / Architecture
The architecture is structured around a decoupled processing model, separating the CUDA compute kernels from the DirectX 11 rendering thread to maintain UI responsiveness. 

*   **Cellular Subsystem (`Cell.h`)**: The foundational entity of the simulation. The `Cell` struct is explicitly aligned to 64 bytes (`alignas(64)`) to match GPU L1/L2 cache-line architecture, preventing memory fragmentation. It tracks parameters such as `telomere_length`, `mutation_load`, `metabolic_exhaustion`, `cd59_shield`, and `hif1a_expression`.
*   **Biological Engine (`NatureDirector.h` / `NatureDirector.cpp`)**: A mathematical engine that evaluates environmental variables, cytokine gradients (e.g., IL-2, IL-6, TGF-beta), and white blood cell (WBC) lineages. It uses Michaelis-Menten kinetics and Sigmoid activation functions to model complex biological cascades and updates systemic constraints.
*   **Parallel Execution Matrix (`CellularKernel.cuh` / `CellularKernel.cu`)**: Handles the core computational load. The `BiologicalTickKernel` processes cellular states concurrently. It calculates survival probabilities based on environmental factors (e.g., oxygen, glucose) against effector mechanisms like the Perforin/Granzyme pathway and MAC complement system. It utilizes asynchronous CUDA streams (`cudaStreamCreateWithPriority`) and pinned memory (`cudaMallocHost`) to avoid WDDM blocking.
*   **Reactor Engine & Control Loop (`ReactorEngine.h` / `ReactorEngine.cpp`)**: Manages the main asynchronous execution loop. This module integrates the `BiologicalPID` controller, which dynamically adjusts the `immortality_control` variable to maintain the `systemic_z_tumor_saturation` within specific predefined thresholds. It also manages the `AsyncDataLogger` for double-buffered CSV telemetry recording.
*   **Intervention Logic (`SentinelGuard.cpp`, `TelomeraseExploit.cpp`)**: 
    *   `SentinelGuard` acts as an automated threshold monitor, executing pruning functions based on immune evasion metrics (PD-1/PD-L1 axis, Treg aura).
    *   `TelomeraseExploit` models theoretical micro-seeding events (`ExecutePayload`), modifying cellular subsets with a `FLAG_Z_TUMOR_MARKER` to observe the system's capacity to return to equilibrium.
*   **Visualization & Telemetry (`CyberGraph.h` / `CyberGraph.cpp`)**: A DirectX 11 and ImGui-based user interface. It renders a customized HLSL shader representing the cellular population and provides a dashboard mapping the 38 biological variables tracked in the `BioTelemetry` struct.

## Current Status
The computational framework is fully implemented and capable of running on consumer-grade GPUs (tested primarily for 12GB VRAM environments). The core engine successfully allocates memory for large populations (up to `MAX_70M_LIMIT = 70000000`) and logs multivariable data across asynchronous epochs without impeding frame rates.

## Assumptions & Limitations
While the simulation tracks numerous biological variables, several significant assumptions and abstractions are present in the model:
*   **Spatial Abstraction:** The simulation processes cells in a linear 1D array (`d_cells_read_`, `d_cells_write_`). True 3D spatial dynamics, cell-to-cell structural morphology, and distance-based chemical gradients are not explicitly modeled; interactions are primarily calculated based on systemic global variables and randomized target indices.
*   **Arbitrary Scalars & Heuristics:** The codebase utilizes manually tuned scalar multipliers and predefined thresholds (e.g., `cancer_ratio * 0.28`, predefined `APOPTOSIS_THRESHOLD = 0.85`) to maintain computational stability and force the PID equilibrium. These are theoretical constructs rather than values derived from specific *in vitro* or *in vivo* empirical assays.
*   **Theoretical Mechanisms:** Functions such as the rejuvenating exosome network (`ExosomeNetwork`) and the programmatic reversal of telomere degradation via oncogenic symbiosis are speculative mathematical implementations used to test the PID controller, not validated biological phenomena.
*   **Conditional Logic over Pure Physics:** State transitions heavily rely on discrete conditional logic (e.g., explicit `if` statements for checking `CellState::HEALTHY` or flags like `FLAG_P53_ACTIVE`) and discrete stochastic probabilities rather than pure molecular-level physics or continuous partial differential equations (PDEs).

## Usage / How to Run
*(Note: Refer to the provided Visual Studio solution file for environment setup.)*
1.  Ensure an NVIDIA GPU with CUDA support is available and the CUDA Toolkit is installed.
2.  Compile the project using a C++17/C++20 compatible compiler linking against `cudart.lib`, `d3d11.lib`, and `d3dcompiler.lib`.
3.  The application launches via `main.cpp`, initializing the reactor and standard ImGui interface. Data logs will be asynchronously written to a CSV file in the executable directory.
