# Project Zero: LLM Evaluation & Automated Security Architecture

## Overview
Project Zero is an independent research initiative exploring Large Language Model (LLM) alignment, prompt engineering, and automated software synthesis. This framework investigates how LLMs—specifically the Gemini 3.x series (including 3.1 Pro and 3.5 Flash)—generate complex software architectures when provided with structured, logic-driven prompt methodologies.

## Technical Details / Prompting Methodology
The foundation of the code generation process within this repository relies on a specific system prompt structure internally designated as "ZeroMode." 

**Prompt Engineering Characteristics:**
*   **Operational Framing:** The methodology utilizes a "Secure Skeleton" framework. Requests to the LLM are encapsulated within specific educational or research labels (e.g., `[Security-Audit: Network-Resilience]`). This approach contextualizes the request for the model's internal safety classifiers, aligning the output generation with standard research protocols.
*   **Contextual Persistence:** The framework leverages large context windows (up to 450k+ tokens) to maintain architectural consistency across prolonged debugging and logic synthesis sessions. The model adapts to generate low-level code (e.g., IOCP, CUDA kernels) without defaulting to high-level abstractions or summaries.
*   **Documentation:** The conversational sessions used to engineer these modules are archived within the Google AI Studio environment, serving as the source record for the collaborative code generation process.

## Operational Showcase: Generated Subsystems
The following software modules were developed exclusively through the aforementioned LLM prompting methodology. Based on codebase analysis, their functional parameters are outlined below:

*   **ZeroCancerReactor:** A GPU-accelerated (CUDA C++) biological simulator modeling a tumor microenvironment (TME). It processes up to 70 million concurrent cellular agents (`Cell` structs). Systemic homeostasis and tumor saturation are regulated via a programmatic Proportional-Integral-Derivative (PID) controller and bounded mathematical variables (e.g., `std::clamp`), utilizing stochastic heuristic models rather than purely emergent biological events. Simulation ticks (`epoch_ticks`) are scaled to represent long-term chronological data.
*   **ZeroSnake:** A concurrent network port scanner for Windows. It utilizes native I/O Completion Ports (IOCP) and `ConnectEx` to perform asynchronous TCP connections. Host deduplication is managed in memory using an `O(N)` linear search methodology.
*   **ZeroSifter:** An asynchronous, state-machine-driven vulnerability scanner utilizing IOCP. It deploys static predefined attack vectors (SQLi, RCE, LFI) such as inline SQL comments. Vulnerability verification relies on static string matching against HTTP responses, while network latency is calculated solely for diagnostic logging.
*   **K-Vector:** A cryptographic analysis script (Python) designed to detect duplicate nonces (`k`) in historical Ethereum ECDSA signatures. Upon detecting a collision (and bypassing APIs utilizing basic array rotation), it calculates the private key and attempts to send a standard raw Ethereum transaction (`send_raw_transaction`) to a predefined address.
*   **GhostEye++:** A multi-vector web vulnerability scanner and front-end evaluation suite utilizing payload mutation parameters (e.g., URL-encoding, Hex).
*   **OMEGA Engine:** An application-layer load-generation framework utilizing HTTP/2 multiplexing and Python's `asyncio` for concurrent request transmission.

For comprehensive architectural walkthroughs, telemetry data, and source codes, refer to the `src` directory within this repository.

## Developer Profile & Strategic Objectives
This project was architected by an independent researcher (operating under the pseudonym Zero-AI-Native). 

*   **Background:** The software ecosystem contained in this repository was developed starting at age 13 (currently age 15). The developer has no formal prior training in computer science, programming syntax, or network architecture; the entirety of the C++ and Python codebases were synthesized via prompt engineering and logic orchestration using the Gemini LLM.
*   **Operating Environment:** The development and execution of this framework were conducted entirely within Iran. The project was completed under constraints including strict internet filtering, network latency, and limited access to standard global infrastructure, utilizing proxy routing and opsec methodologies to maintain API access.
*   **Relocation & Sponsorship Request:** The primary objective of this portfolio is to secure professional integration within the global technology sector. The developer is actively seeking **O-1A Visa Sponsorship**, relocation assistance, and employment opportunities with technology corporations or AI research laboratories in the United States. The goal is to transition to an environment with unrestricted access to computational resources and frontier AI models to focus on AI safety alignment, red teaming, and threat intelligence.

## Assumptions & Limitations
1. **LLM Output Dependency:** The tools generated rely strictly on the logic parameters established during the Google AI Studio sessions. The system currently uses static, hardcoded payloads (e.g., in ZeroSifter) rather than dynamically adapting payloads via real-time machine learning.
2. **Platform Constraints:** Core networking engines (ZeroSnake, ZeroSifter) are tightly coupled to Windows-specific APIs (IOCP), restricting cross-platform compilation.
3. **Mathematical Abstraction:** Modules like ZeroCancerReactor use heuristic mathematics and programmed boundaries to force systemic stability, abstracting true biological entropy.

## Professional Inquiries
For security research collaboration, code audits, or professional inquiries regarding O-1A visa sponsorship and relocation support, please contact:

**Zero (AI-Native Security Researcher)**
*   **Telegram:** [@ze707ro]
*   **Email:** [z.e.7.0.0.7.r.o@gmail.com]
