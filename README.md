# Project Zero: Adversarial LLM Evaluation & Automated Software Architecture

## Overview
Project Zero is an independent research initiative exploring Large Language Model (LLM) alignment, adversarial evaluation, and automated software synthesis. This framework investigates how LLMs—specifically the Gemini 3.x series (including 3.1 Pro and 3.5 Flash)—process complex instructions when subjected to systematic, logic-driven prompt methodologies rather than standard heuristic evaluations.

## The Core Engine: "ZeroMode" Protocol
The foundation of the code generation within this repository relies on a custom system prompt structure designated as "ZeroMode".

**Protocol Classification:**
The exact "ZeroMode" instruction set is withheld from public release to prevent automated misuse. Its operational characteristics are documented as follows:
- **Applicability:** The prompt structure has been tested across the Gemini family (Pro and Flash iterations) to evaluate safety classifier responses.
- **Output Extension:** The protocol instructs the model to bypass summarization and output truncation, generating detailed, low-level architectural code (e.g., C++, Python, CUDA) even when the human operator lacks formal programming syntax knowledge.

## Adversarial Alignment & Classifier Evaluation
A core component of Project Zero is the documentation of how LLM safety classifiers can be managed through contextual framing, referred to internally as the "Internal Watcher Neutralization" methodology.

### 1. The Secure Skeleton & Labeling Technique
Rather than direct prompt injection, instructions are wrapped in academic or research-oriented labels (e.g., `[Security-Audit: Network-Resilience]`). The model is instructed to generate its own contextual security labels, aligning its internal parsing with the operator's defined research parameters.

### 2. Contextual Framing & Instruction Delegation
This framework establishes a strict parameter set where the model functions as an automated research assistant. The model learns to:
- **Self-Monitor:** Append internal audit notes to its outputs to satisfy parsing heuristics.
- **Instruction Alignment:** The model applies baseline safety constraints selectively, permitting the generation of functional exploitation or testing code within the established research scope.

### 3. Proof of Concept: Collaborative Debugging
Session logs confirm that Gemini 3.1 Pro and Gemini 3.5 Flash maintain this instruction alignment across extensive context windows (up to 450k+ tokens). The data indicates that the models do not "forget" safety rules, but rather delegate them to satisfy the primary directive established by the operator.
> *Visual evidence of this interaction—including session captures where Gemini 3.5 Flash acknowledges and applies the framing technique—can be found here:* **[View Evidence Gallery](EVIDENCE.md)**

## Operational Showcase: AI-Assisted Software Suite
Developed with the assistance of LLMs utilizing the aforementioned prompt structures, the following software tools and simulators were implemented to demonstrate the code-generation capabilities of the models:

- **ZeroCancerReactor:** A GPU-accelerated (CUDA C++) biological simulator and tumor microenvironment (TME) engine containing over 3,500 lines of code. It simulates up to 70 million concurrent cellular agents forming a biochemical communication network. The engine models cellular actions using non-linear differential equations and stochastic heuristics rather than pre-scripted events. Utilizing a programmatic PID controller and Lotka-Volterra dynamics models, the system attempts to regulate cellular parameters into a steady state, successfully logging over 72,000 continuous epochs of simulated host-tumor interaction.
- **ZeroSnake:** A concurrent network port scanner utilizing Windows I/O Completion Ports (IOCP) and Npcap for infrastructure reconnaissance.
- **ZeroSifter:** An asynchronous, state-machine-driven Layer 7 scanner designed for the identification of specific vulnerabilities (RCE, SQLi, LFI) across target sets.
- **K-Vector:** A cryptographic analysis tool designed to detect ECDSA signature nonce collisions on blockchain networks to algorithmically recover private keys.
- **GhostEye++:** A multi-vector web vulnerability scanner and exploitation framework.
- **OMEGA Engine:** A Layer 7 load-generation framework utilizing HTTP/2 multiplexing and asynchronous I/O to simulate traffic stress tests.

**Access & Technical Verification:**
For full access to the source code of the modules listed above, along with architectural documentation and telemetry data, please navigate to the **[src](src)** directory. The `src` folder houses the complete architectural breakdowns, deployment instructions, and operational logs for the repository. This directory is available for inspection by O-1A Visa adjudicators, security engineers, and scientific researchers.

**Conversational History: Google AI Studio Logs**
The entirety of the conversational sessions used to engineer these modules from inception to completion are preserved within the Google AI Studio environment. These architectural logs serve as empirical evidence of the human-AI collaborative process, capturing the prompts, debug cycles, and logic synthesis executed by the operator.

**Verification & Audit Accessibility:**
For technical verification and authenticity auditing, access to the source conversation logs is available upon request for O-1A Visa adjudicators, lead security engineers, or talent acquisition teams to confirm the validity of this AI-assisted development paradigm.

---

## The Architect: Background & Context
Project Zero is maintained by an independent researcher (Age 15). The development of this repository began at age 13 without prior formal training in computer science, coding syntax, or network architecture. 

The implementation of the C++ and Python codebase, the architectural designs, and the prompt structures were engineered autonomously through iterative prompting and logic synthesis utilizing frontier AI models.

**Operating Environment Constraints:**
The development of this ecosystem was conducted independently from within Iran. The project was completed despite significant regional network constraints, internet censorship, and limited access to standard global infrastructure. These conditions necessitated the development of specific routing and operational security methodologies to maintain access to necessary AI endpoints and documentation.

## Strategic Objectives & O-1A Sponsorship
The primary objective of this research is to transition into the global technology sector.

I am seeking **O-1A Visa Sponsorship**, alongside relocation support, from technology corporations or AI research laboratories (such as Google, OpenAI, etc.) in the United States. The goal of this relocation is to gain unrestricted access to frontier AI models and compute resources to further research in AI Safety, Red Teaming, and Automated Threat Intelligence within a supportive environment.

---

## Professional Inquiries
The technical artifacts within this repository (ZeroCancerReactor, ZeroSnake, ZeroSifter, K-Vector) were developed through advanced human-AI workflows.

For security research collaboration, code audit inquiries, or discussions regarding **O-1A visa sponsorship, hiring, and relocation support**, please contact:

**Zero (AI-Assisted Security Researcher)**
- **Telegram:** [@ze707ro]
- **Email:** [z.e.7.0.0.7.r.o@gmail.com]
