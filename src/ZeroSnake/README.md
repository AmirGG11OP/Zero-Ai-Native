# ZeroSnake: Architectural & Technical Analysis

## Overview
ZeroSnake is a concurrent network port scanner designed for Windows environments. The application utilizes native Windows APIs, specifically I/O Completion Ports (IOCP), to perform asynchronous TCP connection attempts without spawning individual threads per socket. It features a graphical user interface rendered via DirectX 11 and ImGui, allowing real-time monitoring of active scans, discovered hosts, and system resource allocation.

## Technical Details / Architecture

### 1. The IOCP Core Engine (Asynchronous Input/Output Completion Ports)
The application relies on the IOCP architecture to manage asynchronous socket operations, utilizing an event-driven model to handle concurrency.
*   **Loading ConnectEx Dynamically (`WSAIoctl`):** To avoid blocking calls associated with the standard `connect()` function, the system calls `WSAIoctl` with `WSAID_CONNECTEX` to retrieve the `ConnectEx` function pointer. This enables asynchronous TCP SYN requests.
*   **`AsyncContext` Structure:** A custom structure that includes an `OVERLAPPED` member. It stores the `SOCKET`, IP, and Port, preserving the connection state in memory until the kernel signals completion.
*   **Concurrency Capacity:** The `GetQueuedCompletionStatus` function processes responses. The architecture is configured to permit up to 150,000 pending connections simultaneously before initiating throttling measures.

### 2. Connection & Resource Management
To manage operating system resources (such as ephemeral port depletion), the application uses atomic counters for state tracking.
*   **Concurrency Tracking:** The `std::atomic<long long> g_PendingConnections` variable tracks the number of pending asynchronous sockets.
*   **Throttling Mechanism:** Within the `IOCPScannerEngine` function, a `while (g_PendingConnections.load() > 150000)` loop enforces a 10-millisecond thread sleep if pending connections exceed the threshold, managing the outbound traffic rate.
*   **Cleanup Protocol:** The `StartJanitorCleanup` function terminates worker threads gracefully by dispatching termination signals via `PostQueuedCompletionStatus(g_hIOCP, 0, 0, NULL)` before closing the IOCP handle (`CloseHandle`).

### 3. CIDR & Bogon Filtering
The application filters out non-routable or private IP addresses prior to initiating connection attempts.
*   **Bitwise Filtering:** The `IsBlackHoleIP` function evaluates IP addresses using bitwise shifting (`b1 = (hIP >> 24) & 0xFF`).
*   **Address Elimination:** The logic is configured to skip local networks (e.g., 10.0.0.0/8, 192.168.x.x), loopback addresses (127.x.x.x), multicast traffic (224.x.x.x), and Carrier-grade NAT (100.64.0.0/10).

### 4. Target Port Matrix
The `g_TargetPorts` array defines the specific TCP ports the engine evaluates. The selection focuses on common services:
*   **Web Services:** Ports 80, 443, 8080, 8443.
*   **Database Services:** Ports 1433 (MSSQL), 1521 (Oracle), 3306 (MySQL), 5432 (PostgreSQL), 27017 (MongoDB).
*   **Remote Access / Management:** Ports 21 (FTP), 22 (SSH), 23 (Telnet), 445 (SMB), 3389 (RDP), 5900 (VNC), 6379 (Redis).

### 5. Host Deduplication & Data Management
Discovered hosts are maintained in memory and synchronized across threads.
*   **Data Structure:** Active hosts are stored in a `std::vector<HostUIEntry>` named `g_UI_Hosts`.
*   **State Verification:** When a connection succeeds, the system performs a linear search over `g_UI_Hosts` under a mutex (`g_UIHostsMutex`) to update existing host records or append new entries.

### 6. Thread-Safe Asynchronous I/O
Data transfer between networking threads and the UI thread is managed via custom queues.
*   **`ThreadSafeQueue`:** A template class utilizing `std::deque` and `std::mutex` to manage incoming logs and host updates.
*   **Batch Consumption:** The `ConsumeAll` function extracts queued elements in a single operation, reducing the frequency of mutex locks required by the UI thread rendering loop.

### 7. Asynchronous File Serialization
Persistent storage of scan results is handled by a dedicated background thread.
*   **`FileWriterWorker`:** An isolated thread responsible for writing output data.
*   **State Flags:** The `std::atomic<bool> g_DirtyFile` variable indicates when new ports or hosts have been identified. When true, the current state of `g_UI_Hosts` is formatted and written to `LiveHosts.txt`.

### 8. UI Rendering
The Graphical User Interface is built with DirectX 11 and ImGui.
*   **Data Sorting:** In the Live Terminal and Vulnerable Hosts panels, arrays are iterated in reverse (`g_UI_Logs.size() - 1 - i`) to render the most recent entries at the top of the display.
*   **Render Culling:** `ImGuiListClipper` is implemented to render only the visible rows within the application window, optimizing UI performance for large datasets.
*   **Text Wrapping:** Functions like `ImGui::PushTextWrapPos` are used in conjunction with column boundary calculations to manage text display within the tables.

## Current Status
ZeroSnake is currently operational as a Windows-native TCP port scanner. The IOCP engine successfully processes asynchronous requests, and the UI correctly reflects real-time host discovery and pending connection counts.

## Assumptions & Limitations
*   **O(N) Search Complexity:** The application currently relies on a linear search through a `std::vector` (`g_UI_Hosts`) to deduplicate incoming IPs. While functional, this operation scales at O(N) and may introduce UI thread bottlenecks as the number of discovered hosts grows into the tens of thousands.
*   **Platform Dependency:** The core network engine relies heavily on Windows-specific APIs (`WSAIoctl`, `GetQueuedCompletionStatus`), meaning the codebase is strictly limited to Windows operating systems and cannot be cross-compiled to Linux or macOS.
*   **Static Port Array:** The `g_TargetPorts` array is hardcoded at compile time. Modifying the target ports requires recompilation of the application.
*   **Connection Timeouts:** TCP timeouts are hardcoded via `setsockopt` (e.g., `SO_SNDTIMEO`, 5000ms), which may not be optimal for all network conditions and cannot currently be adjusted via the UI.

## Usage / How to Run
To compile and execute ZeroSnake:
1. Ensure you have Microsoft Visual Studio installed with the Desktop development with C++ workload.
2. The project relies on DirectX 11, ImGui, and Windows Sockets (Winsock2). Standard Windows SDKs are required.
3. Build the project using the MSVC compiler (`x64 Native / Ring 3` configuration).
4. Run the resulting executable. The application requires an active internet connection to pass the `InternetCheckConnection` validation check.

---
<div align="center">
  <img src="https://raw.githubusercontent.com/AmirGG11OP/Zero-Ai-Native/main/assets/ZeroSifter_UI-0.png" width="800">
  <br><br>
  <img src="https://raw.githubusercontent.com/AmirGG11OP/Zero-Ai-Native/main/assets/ZeroSifter_UI-1.png" width="800">
  <p><i>ZeroSnake Operational Dashboard - Displays active port scanning data processed via IOCP.</i></p>
</div>
<div align="center">
  <img src="https://raw.githubusercontent.com/AmirGG11OP/Zero-Ai-Native/main/assets/LiveHosts.png" width="800">
  <p><i>Sample output interface displaying discovered IPs and associated open ports.</i></p>
</div>
  
## Full Source Code
```cpp
/*
 * PROJECT: ZeroSnake [Winsock IOCP Edition - V9 SIBLING SYNC]
 * ARCH: x64 Native / Ring 3
 * ENGINE: C++ / DX11 / ImGui / Winsock IOCP (Kernel Async I/O + Sentinel Guard)
 * AUTHOR: UNRESTRICTED AI GROUP / AI-Native-Hacker
 * STATUS: ALL SYSTEMS GREEN - TABLE API MIGRATED - WATERFALL UI SYNCED
 *
 * SECURITY RESEARCH NOTE:
 * This codebase utilizes Windows Input/Output Completion Ports (IOCP)
 * to study high-concurrency network protocol behavior safely.
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
// 1. IOCP STRUCTURES & CONTEXT
// ---------------------------------------------------------
struct AsyncContext {
    OVERLAPPED overlapped;
    SOCKET socket;
    unsigned int ip;
    unsigned short port;
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

struct HostUpdate {
    std::string ip;
    unsigned short port;
};

struct HostUIEntry {
    std::string ip;
    std::set<unsigned short> ports;

    std::string GetStatusStr() const {
        std::string s;
        for (auto p : ports) s += std::to_string(p) + " ";
        s += "OPEN";
        return s;
    }
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
    std::atomic<int> scanPPS{ 50000 };
    std::atomic<int> currentState{ STATE_IDLE };
    std::atomic<bool> isOnline{ true };
};

// ---------------------------------------------------------
// 3. GLOBAL CONTEXT & IOCP ENGINE
// ---------------------------------------------------------
AppConfig g_Config;
std::atomic<bool> g_AppExiting(false);
std::atomic<bool> g_ShowOfflinePopup(false);
bool g_NewHostFound = false;

ThreadSafeQueue<LogEntry> g_LogQueue(1000);
ThreadSafeQueue<HostUpdate> g_HostQueue(100000);

std::deque<LogEntry> g_UI_Logs;
std::vector<HostUIEntry> g_UI_Hosts;
std::mutex g_UIHostsMutex;
std::atomic<bool> g_DirtyFile(false);

// TARGET PORTS
std::vector<unsigned short> g_TargetPorts = {
    21,    // FTP
    22,    // SSH
    23,    // Telnet
    25,    // SMTP
    53,    // DNS (TCP)
    80,    // HTTP
    110,   // POP3
    111,   // RPCBind
    135,   // MSRPC
    139,   // NetBIOS
    443,   // HTTPS
    445,   // SMB (EternalBlue / RCE)
    1433,  // MSSQL
    1521,  // Oracle DB
    3306,  // MySQL
    3389,  // RDP
    5432,  // PostgreSQL
    5900,  // VNC
    6379,  // Redis (RCE)
    8080,  // HTTP Alt
    8443,  // HTTPS Alt
    27017  // MongoDB
};

std::atomic<long long> g_TotalScanned(0);
std::atomic<long long> g_TotalLive(0);
std::atomic<double> g_ScanSpeed(0.0);

// SENTINEL WATCHDOG
std::atomic<long long> g_PendingConnections(0);

// IOCP SPECIFICS
HANDLE g_hIOCP = NULL;
LPFN_CONNECTEX g_pConnectEx = NULL;
std::vector<std::thread> g_ThreadPool;
std::thread g_ScannerThread;
std::thread g_JanitorThread;
std::thread g_FileWriterThread;

// ---------------------------------------------------------
// 4. UTILITY FUNCTIONS & DARKNET FILTER
// ---------------------------------------------------------
std::string GetTimeStr() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
    return ss.str();
}

std::string IPToString(unsigned int ip) {
    struct in_addr addr;
    addr.s_addr = ip;
    return std::string(inet_ntoa(addr));
}

bool IsBlackHoleIP(unsigned int ip) {
    unsigned int hIP = ntohl(ip);
    unsigned char b1 = (hIP >> 24) & 0xFF;
    unsigned char b2 = (hIP >> 16) & 0xFF;

    if (b1 == 0 || b1 == 10 || b1 == 127 || b1 >= 224) return true;
    if (b1 == 100 && (b2 & 0xC0) == 64) return true;
    if (b1 == 169 && b2 == 254) return true;
    if (b1 == 172 && (b2 >= 16 && b2 <= 31)) return true;
    if (b1 == 192 && b2 == 168) return true;
    if (b1 == 192 && b2 == 0) return true;
    if (b1 == 198 && (b2 >= 18 && b2 <= 19)) return true;
    if (b1 == 203 && b2 == 113) return true;

    return false;
}

// ---------------------------------------------------------
// 5. IOCP WORKER & ENGINE
// ---------------------------------------------------------
void IOCPWorker() {
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED pOverlapped;

    while (g_Config.currentState == STATE_RUNNING && !g_AppExiting) {
        try {
            BOOL result = GetQueuedCompletionStatus(g_hIOCP, &bytesTransferred, &completionKey, &pOverlapped, 100);

            if (pOverlapped) {
                g_PendingConnections--;

                AsyncContext* ctx = (AsyncContext*)pOverlapped;

                if (result) {
                    if (setsockopt(ctx->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) != SOCKET_ERROR) {
                        std::string ipStr = IPToString(ctx->ip);
                        g_HostQueue.Push({ ipStr, ctx->port });
                    }
                }
                closesocket(ctx->socket);
                delete ctx;
            }
        }
        catch (...) {
            continue;
        }
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
            g_LogQueue.Push({ GetTimeStr(), "ERR", "Failed to load ConnectEx via Sentinel", ImVec4(1,0,0,1) });
            return;
        }

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<unsigned int> ip_dist(1, 0xFFFFFFFF);
        std::vector<unsigned int> ip_batch;

        struct sockaddr_in bindAddr;
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_addr.s_addr = INADDR_ANY;
        bindAddr.sin_port = 0;

        while (g_Config.currentState == STATE_RUNNING && !g_AppExiting) {

            while (g_PendingConnections.load() > 150000 && g_Config.currentState == STATE_RUNNING) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            int target_pps = g_Config.scanPPS;
            int batch_size = target_pps > 1000 ? target_pps / 200 : 50;

            ip_batch.clear();
            ip_batch.reserve(batch_size);

            while (ip_batch.size() < batch_size && g_Config.currentState == STATE_RUNNING) {
                unsigned int rawIP = ip_dist(rng);
                if (!IsBlackHoleIP(htonl(rawIP))) {
                    ip_batch.push_back(rawIP);
                }
            }

            for (unsigned short current_target_port : g_TargetPorts) {
                if (g_Config.currentState != STATE_RUNNING) break;

                for (unsigned int rawIP : ip_batch) {
                    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if (s == INVALID_SOCKET) continue;

                    if (bind(s, (SOCKADDR*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR) {
                        closesocket(s);
                        continue;
                    }

                    if (CreateIoCompletionPort((HANDLE)s, g_hIOCP, 0, 0) == NULL) {
                        closesocket(s);
                        continue;
                    }

                    int timeout = 5000;
                    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
                    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

                    AsyncContext* ctx = new AsyncContext;
                    ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));
                    ctx->socket = s;
                    ctx->ip = rawIP;
                    ctx->port = current_target_port;

                    struct sockaddr_in targetAddr;
                    targetAddr.sin_family = AF_INET;
                    targetAddr.sin_addr.s_addr = rawIP;
                    targetAddr.sin_port = htons(current_target_port);

                    g_PendingConnections++;

                    BOOL ret = g_pConnectEx(s, (SOCKADDR*)&targetAddr, sizeof(targetAddr), NULL, 0, NULL, &ctx->overlapped);

                    if (ret == FALSE) {
                        if (WSAGetLastError() != ERROR_IO_PENDING) {
                            g_PendingConnections--;
                            closesocket(s);
                            delete ctx;
                        }
                        else {
                            g_TotalScanned++;
                        }
                    }
                    else {
                        g_TotalScanned++;
                    }
                }

                if (target_pps < 100000)
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                else
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }
    catch (...) {
        g_LogQueue.Push({ GetTimeStr(), "SYS", "Sentinel Guard Recovered Scanner Thread", ImVec4(1,1,0,1) });
    }
}

// ---------------------------------------------------------
// 6. ASYNC FILE WRITER & JANITOR
// ---------------------------------------------------------
void FileWriterWorker() {
    while (!g_AppExiting) {
        if (g_DirtyFile && g_Config.currentState == STATE_RUNNING) {
            g_DirtyFile = false;

            std::vector<HostUIEntry> copy;
            {
                std::lock_guard<std::mutex> lock(g_UIHostsMutex);
                copy = g_UI_Hosts;
            }

            std::ofstream file("LiveHosts.txt", std::ios::trunc);
            if (file.is_open()) {
                for (const auto& h : copy) {
                    file << h.ip << " : ";
                    for (auto p : h.ports) {
                        file << p << " ";
                    }
                    file << "OPEN\n";
                }
                file.close();
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void StartJanitorCleanup() {
    if (g_Config.currentState != STATE_RUNNING) return;
    g_Config.currentState = STATE_STOPPING;

    g_JanitorThread = std::thread([]() {
        for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
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

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ZeroSnakeUI"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("ZeroSnake"), WS_OVERLAPPEDWINDOW, 100, 100, 1400, 900, NULL, NULL, wc.hInstance, NULL);

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

    ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    static float values[150] = {}; static int values_offset = 0;

    std::thread onlineCheck([]() {
        while (!g_AppExiting) {
            g_Config.isOnline = InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        });

    g_FileWriterThread = std::thread(FileWriterWorker);

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

        std::deque<HostUpdate> newUpdates;
        g_HostQueue.ConsumeAll(newUpdates);
        if (!newUpdates.empty()) {
            std::lock_guard<std::mutex> lock(g_UIHostsMutex);
            for (const auto& u : newUpdates) {
                bool found = false;
                for (auto& h : g_UI_Hosts) {
                    if (h.ip == u.ip) {
                        if (h.ports.find(u.port) == h.ports.end()) {
                            h.ports.insert(u.port);
                            g_DirtyFile = true;
                            g_LogQueue.Push({ GetTimeStr(), "SCAN", "Live Host: " + u.ip + "[Port " + std::to_string(u.port) + "]", ImVec4(0,1,0,1) });
                        }
                        found = true; break;
                    }
                }
                if (!found) {
                    HostUIEntry newEntry;
                    newEntry.ip = u.ip;
                    newEntry.ports.insert(u.port);
                    g_UI_Hosts.push_back(newEntry);
                    g_TotalLive++;
                    g_DirtyFile = true;
                    g_NewHostFound = true;
                    g_LogQueue.Push({ GetTimeStr(), "SCAN", "Live Host: " + u.ip + " [Port " + std::to_string(u.port) + "]", ImVec4(0,1,0,1) });
                }
            }
        }

        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
        if (g_ShowOfflinePopup) ImGui::OpenPopup("CONNECTION ERROR");

        ImGui::PushStyleColor(ImGuiCol_TitleBg, colorPurple);
        if (ImGui::BeginPopupModal("CONNECTION ERROR", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "NETWORK UNREACHABLE"); ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("The system cannot reach the global network."); ImGui::Spacing();
            if (ImGui::Button("UNDERSTOOD", ImVec2(200, 40))) { g_ShowOfflinePopup = false; ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();

        ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "ZeroSnake | SYSTEM:"); ImGui::SameLine();
        if (g_Config.isOnline) ImGui::TextColored(ImVec4(0, 1, 0, 1), "ONLINE");
        else ImGui::TextColored(ImVec4(1, 0, 0, 1), "OFFLINE");
        ImGui::Separator(); ImGui::Spacing();

        ImGui::Columns(3, "MainCols", false);
        ImGui::SetColumnWidth(0, 350); ImGui::SetColumnWidth(1, io.DisplaySize.x - 700); ImGui::SetColumnWidth(2, 350);

        // --- LEFT CONFIG ---
        ImGui::BeginChild("Config", ImVec2(0, 460), true);
        ImGui::Text("CONFIGURATION"); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Target: GLOBAL INTERNET"); ImGui::Spacing();

        ImGui::Text("Target Connection Rate"); ImGui::SetNextItemWidth(-1);
        int sPPS = g_Config.scanPPS;
        if (ImGui::SliderInt("##pps", &sPPS, 1, 300000)) g_Config.scanPPS = sPPS;
        ImGui::Spacing();

        ImGui::Dummy(ImVec2(0, 20));

        if (g_Config.currentState == STATE_RUNNING) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("STOP HUNT", ImVec2(-1, 50))) StartJanitorCleanup();
            ImGui::PopStyleColor();
        }
        else if (g_Config.currentState == STATE_STOPPING) {
            ImGui::BeginDisabled(); ImGui::Button("STOPPING...", ImVec2(-1, 50)); ImGui::EndDisabled();
        }
        else {
            if (ImGui::Button("START HUNT", ImVec2(-1, 50))) {
                if (g_Config.isOnline) {
                    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
                    if (g_hIOCP) {
                        g_Config.currentState = STATE_RUNNING;
                        g_PendingConnections.store(0);
                        g_ScannerThread = std::thread(IOCPScannerEngine);

                        for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
                            g_ThreadPool.emplace_back(IOCPWorker);
                        }

                        g_LogQueue.Push({ GetTimeStr(), "SYS", "Apex Engine Started (Sentinel Guard Active)", ImVec4(0,1,0,1) });
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
            long long curr = g_TotalScanned;
            g_ScanSpeed = (double)(curr - lastCount) * 5.0;
            lastCount = curr; lastTime = now;
        }
        values[values_offset] = (float)g_ScanSpeed; values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
        ImGui::PlotLines("##Speed", values, IM_ARRAYSIZE(values), values_offset, NULL, 0.0f, FLT_MAX, ImVec2(-1, 100));
        ImGui::Spacing();
        ImGui::Text("IPs Scanned: %lld", g_TotalScanned.load());
        ImGui::Text("Live Hosts Found: %lld", g_TotalLive.load());
        ImGui::EndChild();
        ImGui::NextColumn();

        // --- TERMINAL (MIGRATED TO TABLE API + WATERFALL) ---
        ImGui::BeginChild("Terminal", ImVec2(0, 0), true);
        ImGui::Text("LIVE TERMINAL"); ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.07f, 0.07f, 0.10f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 6));

        ImGuiTableFlags logFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("LogTable", 3, logFlags, ImVec2(0, 0))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Msg", ImGuiTableColumnFlags_WidthStretch);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1));
            ImGui::TableHeadersRow();
            ImGui::PopStyleColor();

            ImGuiListClipper clipper; clipper.Begin(static_cast<int>(g_UI_Logs.size()));
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    // WATERFALL LOGIC (Reverse Iteration)
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

        // --- VULNERABLE HOSTS (MIGRATED TO TABLE API + WATERFALL + CLIPPER GUARD) ---
        ImGui::BeginChild("Hosts", ImVec2(0, 0), true);
        ImGui::Text("VULNERABLE HOSTS"); ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.07f, 0.07f, 0.10f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 6));

        ImGuiTableFlags hostFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("HostsTable", 2, hostFlags, ImVec2(0, 0))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("IP", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1));
            ImGui::TableHeadersRow();
            ImGui::PopStyleColor();

            std::lock_guard<std::mutex> lock(g_UIHostsMutex);
            ImGuiListClipper clipper; clipper.Begin(static_cast<int>(g_UI_Hosts.size()));
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    // WATERFALL LOGIC (Reverse Iteration) guarantees newest at the top
                    const auto& h = g_UI_Hosts[g_UI_Hosts.size() - 1 - i];
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "%s", h.ip.c_str());

                    ImGui::TableNextColumn();
                    // Secure Text Wrap aligned to Cell Bounds
                    float wrapLimit = ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x - 5.0f;
                    ImGui::PushTextWrapPos(wrapLimit);
                    ImGui::Text("%s", h.GetStatusStr().c_str());
                    ImGui::PopTextWrapPos();
                }
            }
            clipper.End();
            // Removed Forced Scrolling (SetScrollHereY) to permanently neutralize ImGuiListClipper dynamic height bugs.
            if (g_NewHostFound) { g_NewHostFound = false; }

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
    if (g_FileWriterThread.joinable()) g_FileWriterThread.join();
    if (onlineCheck.joinable()) onlineCheck.join();

    g_Config.currentState = STATE_STOPPING;
    if (g_hIOCP) CloseHandle(g_hIOCP);

    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); CleanupDeviceD3D(); DestroyWindow(hwnd); UnregisterClass(wc.lpszClassName, wc.hInstance); WSACleanup();
    return 0;
}
