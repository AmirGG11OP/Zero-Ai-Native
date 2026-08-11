# Source Code Laboratory: AI-Assisted Software Suite

## Overview
This directory contains the core implementation layer of the Zero-Ai-Native project. The repository houses a collection of modules developed utilizing high-concurrency architectures, low-level system APIs, and computational models for security research and biological simulation. The development process heavily utilized Large Language Models (LLMs) for algorithmic generation and logic synthesis.

---

## Technical Details / Architecture & Genesis

**Operator Identity:** Zero-AI-Native (Age 15)  
**Core AI Engine:** Google Gemini 3.1 Pro  
**Development Methodology:** AI-Assisted Engineering  

### Development Context
This software ecosystem—comprising C++, CUDA, and Python implementations—was developed by a 15-year-old operator utilizing the Gemini 3.1 Pro architecture. The code generation was orchestrated via a custom system prompt structure (referred to internally as "ZeroMode"), designed to bypass conversational simplifications and directly synthesize low-level execution models (IOCP, CUDA kernels, Win32API, and Raw Sockets) under operator guidance.

### Conversational Logs & Verification
The iterative generation, debugging, and logic synthesis cycles are documented within Google AI Studio. These architectural logs record the iterative prompts and code outputs used to structure the modules. Access to the raw conversational history is available for technical auditing and verification by researchers or adjudicators.

<div align="center">
  <img src="https://raw.githubusercontent.com/AmirGG11OP/Zero-Ai-Native/main/assets/Photo%20%2326%20from%20ZeroCancerReactor.png" width="800">
  <p><i>Figure: Sample log matrix for the ZeroCancerReactor module, illustrating consecutive prompt sessions and token utilization during the generation of the CUDA architecture and biological logic.</i></p>
</div>
<br>
<div align="center">
  <img src="https://raw.githubusercontent.com/AmirGG11OP/Zero-Ai-Native/main/assets/ConversationalHistory.png" width="800">
  <p><i>Figure: Google AI Studio Environment - Session history archive documenting the iterative development process.</i></p>
</div>

---

## Internal Modules & Architecture

### [ZeroCancerReactor](ZeroCancerReactor)
**Role:** GPU-Accelerated 70M-Cell TME Simulator  
**Technical Stack:** CUDA C++ / DirectX 11 (ImGui) / Computational Biology  
**Architectural Highlights:**
*   **Parallel Execution Matrix:** Processes up to 70 million concurrent `Cell` structs on consumer-grade hardware (tested on RTX 3060) utilizing 64-byte cache-line memory alignments and asynchronous CUDA streams.
*   **Heuristic Biological Modeling:** Simulates cytokine networks, effector mechanisms, and cellular states using stochastic Gaussian noise, non-linear differential equations, and Michaelis-Menten kinetics instead of pre-scripted state transitions.
*   **Systemic Homeostasis:** Implements a programmatic `BiologicalPID` controller that regulates the simulated tumor parameters to achieve a theoretical steady-state (Lotka-Volterra dynamics), logging outputs via an asynchronous `AsyncDataLogger`.

### [ZeroSnake](ZeroSnake)
**Role:** Concurrent Network Port Scanner  
**Technical Stack:** C++ / Windows Socket API / IOCP / ImGui (DX11)  
**Architectural Highlights:**
*   **Asynchronous I/O:** Utilizes native Windows I/O Completion Ports (IOCP) and `ConnectEx` via `WSAIoctl` to handle high-concurrency TCP connections without spawning individual threads per socket.
*   **Resource Management:** Implements atomic counters (`g_PendingConnections`) and micro-throttling to manage outbound traffic rates and prevent OS resource exhaustion.
*   **Data Deduplication:** Employs linear search caching over `std::vector` structures and bitwise operations to filter out Bogon and non-routable IP addresses.

### [ZeroSifter](ZeroSifter)
**Role:** Asynchronous Vulnerability Scanning State-Machine  
**Technical Stack:** C++ / IOCP / ImGui (DX11)  
**Architectural Highlights:**
*   **State-Driven Execution:** Ingests output from port scanners and executes an asynchronous, multi-layered payload state-machine (`OP_CONNECT`, `OP_SEND`, `OP_RECV`).
*   **FractalBrain Payload Generation:** Dynamically serves predefined payloads targeting SQLi, RCE, and LFI, utilizing techniques like inline SQL comments (`/*!50000UNION*/`) to evaluate WAF parsing behaviors.
*   **Hash Cache Verification:** Implements `std::unordered_set` structures (`g_FileVulnCache`, `g_SessionVulnCache`) to log identified vulnerabilities strictly using static string matching over HTTP responses.

### [K-Vector](K-Vector)
**Role:** Cryptographic Collision & Blockchain Analysis Tool  
**Technical Stack:** Python / Flet (UI) / Web3.py / ecdsa  
**Architectural Highlights:**
*   **Signature Analysis:** Scans historical Ethereum transactions to identify non-random nonce (`k`) reuse in ECDSA signature generation.
*   **Mathematical Extraction:** Utilizes modular arithmetic (`inverse_mod`) to calculate compromised nonces and derive SECP256k1 private keys.
*   **Automated Execution Logic:** Includes a routine to autonomously sign and broadcast raw Ethereum transactions (`send_raw_transaction`) mapping a programmatic fee logic, conditional upon successful key recovery.

### [GhostEye](GhostEye)
**Role:** Web Exploitation & Threat Intelligence Suite  
**Technical Stack:** Python / Flet (UI) / BeautifulSoup  
**Architectural Highlights:**
*   **Payload Mutation:** Implements modules to adjust payloads (e.g., URL-encoding, Null bytes) based on HTTP response analysis.
*   **Vulnerability Evaluation:** Automates the testing of specific application-layer flaws such as SSRF, XXE, SSTI, and Prototype Pollution.

### [OMEGA](OMEGA)
**Role:** Application-Layer Load Generation Framework  
**Technical Stack:** Python (asyncio) / HTTPX  
**Architectural Highlights:**
*   **Asynchronous Stress Testing:** Utilizes `asyncio` and multiprocessing to generate concurrent HTTP/2 requests for application-layer stress testing.
*   **Header & Protocol Manipulation:** Integrates proxy rotation and header randomization to evaluate network rate-limiting algorithms and DPI configurations.

---

## Current Status
The suite consists of functional executables and scripts. The C++ modules (ZeroCancerReactor, ZeroSnake, ZeroSifter) are operational within Windows environments. The Python tools (K-Vector, GhostEye, OMEGA) require standard interpreters and explicit dependencies (e.g., `web3`, `flet`).

## Assumptions & Limitations
1.  **Platform Dependency:** The C++ network and rendering engines rely explicitly on Windows APIs (IOCP, `ConnectEx`, DirectX 11). They cannot be natively compiled on Linux or macOS.
2.  **Heuristic Algorithms vs. Empirical Biology:** In *ZeroCancerReactor*, assertions regarding "biological immortality," "age 25," or "1,300 years" are heuristic mappings of simulation ticks (`epoch_ticks`), not validated clinical metrics. The equilibrium is enforced mathematically (via `std::clamp` and PID controllers).
3.  **Algorithmic Complexity:** *ZeroSnake* relies on `O(N)` linear searches over vectors for deduplication, which may cause performance bottlenecks under extreme scaling compared to hash-based mapping.
4.  **Vulnerability Verification:** *ZeroSifter* utilizes static string matching for vulnerability confirmation rather than dynamic heuristic validation or execution latency checks.
5.  **Cryptographic Scope:** *K-Vector* assumes target wallets failed to implement RFC 6979. Modern wallets with deterministic nonce generation are immune to this specific analysis vector.

## Usage / How to Run
Please navigate to the specific directories (e.g., `src/ZeroCancerReactor`, `src/ZeroSnake`) for granular build instructions, dependency requirements, and execution parameters.

---

## Security & Compliance Note
All source code provided in this repository is strictly for **Educational and Authorized Security Research Purposes**. The modules are intended for use by researchers and developers to study high-concurrency architecture, mathematical modeling, and to identify software vulnerabilities in controlled, authorized environments.
