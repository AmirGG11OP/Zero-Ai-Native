# Zero Cancer Reactor: GPU-Accelerated 70M-Cell TME Simulator

<div align="center">
  <i>A computational framework for simulating time-dependent cellular dynamics and tumor microenvironment equilibrium.</i>
  <br><br>
  <b>Developer:</b> Zero-AI-Native (Age 15, assisted by Google Gemini 3.1 Pro) <br>
  <b>Architecture:</b> CUDA C++ / DirectX 11 / Computational Biology<br>
  <b>Status:</b> Concept / Simulation Deployed
</div>

---

## Overview
The Zero Cancer Reactor is a GPU-accelerated biological cellular automaton and simulator. It is designed to mathematically model the Tumor Microenvironment (TME) for up to 70 million concurrent cellular agents. The simulation relies on real-time stochastic models, differential equation approximations, and biochemical kinetics rather than pre-scripted state transitions. 

**Project Objective:** The simulation explores a theoretical model based on Lotka-Volterra dynamics. By implementing a programmatic proportional-integral-derivative (PID) controller (`BiologicalPID`), the system attempts to regulate a modified cellular population to achieve targeted homeostasis. This includes simulating telomere maintenance and evaluating whether a steady-state symbiosis can be maintained between simulated host parameters and neoplastic agents over extended chronological mapping.

### Scale & Hardware Specifications
The simulation is engineered to process up to 70 million independent agents (`Cell` structs) per tick. Operations such as chemical gradient evaluation, cellular division, and simulated immune responses are executed on the GPU. The current matrix has been optimized to run within the constraints of consumer-grade hardware, specifically tested on an NVIDIA RTX 3060 (12GB VRAM). Performance stability (60.0 FPS UI rendering) is maintained by aligning memory structures (64-byte cache-line alignment) and decoupling CUDA compute kernels from the DirectX 11 rendering thread using asynchronous streams (`cudaStreamCreateWithPriority`).

---

## Technical Details / Architecture

All core mathematical models, CUDA execution kernels, and biological logic controllers are publicly accessible for peer review and structural analysis.

**Source Code Modules & Engine Components:**

*   [`NatureDirector.h`](NatureDirector.h) | [`NatureDirector.cpp`](NatureDirector.cpp)
    *   Executes the core biological engine approximations, cytokine network variables, and Lotka-Volterra mathematical models via stochastic methods (`FastXoshiro256Nature`).
*   [`ReactorEngine.h`](ReactorEngine.h) | [`ReactorEngine.cpp`](ReactorEngine.cpp)
    *   Manages the asynchronous master loop, the `BiologicalPID` controller, and the `AsyncDataLogger` for continuous telemetry recording.
*   [`CellularKernel.cuh`](CellularKernel.cuh) | [`CellularKernel.cu`](CellularKernel.cu)
    *   Contains the CUDA HPC parallel execution implementations (e.g., `BiologicalTickKernel`). Evaluates cell state changes, resource allocation, and effector mechanisms concurrently.
*   [`TelomeraseExploit.h`](TelomeraseExploit.h) | [`TelomeraseExploit.cpp`](TelomeraseExploit.cpp)
    *   Implements the `ExecutePayload` function, a targeted modification logic (referred to internally as the Phoenix protocol) designed to alter telomere parameters (`FLAG_Z_TUMOR_MARKER`) dynamically.
*   [`SentinelGuard.h`](SentinelGuard.h) | [`SentinelGuard.cpp`](SentinelGuard.cpp)
    *   Handles simulated immune orchestration, calculating pruning thresholds and evasion logic based on effector variables like `perforin_granzyme_pathway`.
*   [`Cell.h`](Cell.h)
    *   Defines the foundational autonomous agent structure. Explicitly aligned (`alignas(64)`) to exactly 64 bytes to ensure GPU cache-line efficiency. Tracks parameters such as `telomere_length`, `mutation_load`, and `epigenetic_shield`.
*   [`main.cpp`](main.cpp)
    *   The entry point. Initializes the execution matrix up to the `MAX_70M_LIMIT` and manages the primary asynchronous event-driven lifecycle.
*   [`CyberGraph.h`](CyberGraph.h) | [`CyberGraph.cpp`](CyberGraph.cpp)
    *   A DirectX 11 / ImGui-based user interface. Provides decoupled telemetry visualization. It also acts as the control module for tracking `integration_phase_active_` states and issuing modification payloads based on user-defined demographic inputs.
*   [`BioTerminal.h`](BioTerminal.h) | [`BioTerminal.cpp`](BioTerminal.cpp)
    *   A thread-safe, double-buffered logging system for real-time reporting of simulated biological events.

**Audit & Collaboration Accessibility:**
The codebase is structured to allow independent compilation and verification of the parallel processing algorithms and mathematical heuristics used to evaluate cellular survival probabilities.

---

## Visual & Empirical Validation Portal

Comprehensive visual data and empirical datasets are available to verify the engine’s real-time variable processing and performance stability.

*   **[TelemetryGallery](TelemetryGallery.md):** A visual repository containing high-resolution captures of the DirectX 11 interface during operation, documenting system state transitions across the 38 tracked biological variables and hardware resource utilization.
*   **[BiologicalTelemetryDataset](BiologicalTelemetryDataset.md):** An analytical dictionary for the telemetry output. It details the 69 parameters generated and recorded asynchronously by the `AsyncDataLogger` during a simulated run of 72,000 ticks.

---

## Current Status
The project is currently capable of running stable simulations with populations scaling up to the 70-million parameter limit. The `AsyncDataLogger` has successfully output datasets (e.g., 72,000 consecutive ticks), which the internal metric system correlates to long-term host-tumor equilibrium. The UI and GPU compute threads run asynchronously without deadlocks.

---

## Assumptions & Limitations
To accurately evaluate this simulator, the following technical and scientific limitations must be acknowledged:

1.  **Heuristic Modeling vs. Empirical Biology:** The variables used for immune system responses, cytokine interactions, and effector mechanisms (e.g., `mac_complement_system`, `cd59_shield`) are based on mathematical heuristics (Sigmoid functions, arbitrary scalar multipliers) rather than being directly derived from measured in-vivo kinetic data.
2.  **Temporal Abstraction:** Output claims relating to "1,300 years of real-time human-tumor symbiosis" or locking biological age to "25 years" are based on an arbitrary programmatic mapping of simulation ticks (`epoch_ticks`) to real-world time. This is a functional assumption of the engine, not a validated biological equivalent.
3.  **Forced Homeostasis:** The Lotka-Volterra equilibrium and systemic stability are heavily regulated by a programmed `BiologicalPID` controller and explicit upper/lower bounds (`std::clamp`), rather than emerging purely from the unconstrained interaction of the cellular agents.
4.  **Pseudo-Random Number Generation:** The simulation utilizes fast PRNGs (`gpu_rand_float`, `FastXoshiro256Nature`) to determine stochastic events like mutation probabilities and cellular apoptosis. While efficient for GPU parallelization, these do not represent true biological entropy.

---

## Usage / How to Run
*(Instructions for compiling via MSVC, linking DirectX 11, setting up the NVIDIA CUDA Toolkit, and running the executable should be placed here).*
