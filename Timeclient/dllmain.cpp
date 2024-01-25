// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <winsock2.h>
#include <chrono>
#include <iostream>

#if 0
std::chrono::duration<std::chrono::high_resolution_clock::rep, std::chrono::high_resolution_clock::period> g_DeltaTimestamp;
long long g_TickDelta = 0;
#endif

std::chrono::time_point<std::chrono::high_resolution_clock> g_ClientLocalTick_Start;
std::chrono::time_point<std::chrono::high_resolution_clock> g_ClientLocalTick_End;
std::chrono::nanoseconds g_Delay(0);
std::chrono::nanoseconds g_PCBootDetla(0);
int syncCount = 0;

extern "C" __declspec(dllexport) long long proxy_sendto(SOCKET clientSocket, struct sockaddr_in serverAddr)
{
    // 发送当前时间戳到服务器
    g_ClientLocalTick_Start = std::chrono::high_resolution_clock::now();
    sendto(clientSocket, reinterpret_cast<const char*>(&g_ClientLocalTick_Start), sizeof(g_ClientLocalTick_Start), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    return 0;
}

extern "C" __declspec(dllexport) long long proxy_recvfrom(SOCKET clientSocket, struct sockaddr_in serverAddr, int flag)
{
    std::chrono::time_point<std::chrono::high_resolution_clock> ServerTick2;
    int bytesReceived = recvfrom(clientSocket, reinterpret_cast<char*>(&ServerTick2), sizeof(ServerTick2), 0, NULL, NULL);

    //求开机时间差
    if (flag == 1)
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> ClientLocalTick2 = std::chrono::high_resolution_clock::now();
        g_PCBootDetla = std::chrono::abs(ServerTick2 - ClientLocalTick2) - g_Delay;
        std::cout << "Average Delay(ms) = " << std::chrono::duration<double, std::milli>(g_Delay).count() << "\n";
        std::cout << "PC boot Delta(hours) = " << std::chrono::duration_cast<std::chrono::hours>(g_PCBootDetla).count() << "\n";
        std::cout << "PC boot Delta(minutes) = " << std::chrono::duration_cast<std::chrono::minutes>(g_PCBootDetla).count() << "\n";
    }
    else //求空载耗时
    {
        if (bytesReceived == SOCKET_ERROR)
        {
            std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
        }
        else
        {
            g_ClientLocalTick_End = std::chrono::high_resolution_clock::now();
            auto currentDelay = (g_ClientLocalTick_End - g_ClientLocalTick_Start) / 2;
            g_Delay += currentDelay;
            if (syncCount != 0)
            {
                g_Delay = (g_Delay) / 2;
            }
            syncCount++;
            //std::cout << "Current Delay(ms) = " << std::chrono::duration<double, std::milli>(currentDelay).count();
            //std::cout << "   Average Delay(ms) = " << std::chrono::duration<double, std::milli>(g_Delay).count() << "\n";
        }
    }

    return 0;
}


extern "C" __declspec(dllexport) int CalculateDelay(long long ServerTick)
{    
    auto ServerTick_X = std::chrono::time_point<std::chrono::high_resolution_clock>(std::chrono::nanoseconds(ServerTick));
    auto ClientLocalTick_X = std::chrono::high_resolution_clock::now();
	auto ms = std::chrono::abs(ServerTick_X - ClientLocalTick_X) - g_PCBootDetla;
    std::cout << "server to client delay(ms) is " << std::chrono::duration<double, std::milli>(ms).count() << "\n";
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

