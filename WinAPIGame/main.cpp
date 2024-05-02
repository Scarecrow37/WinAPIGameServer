#include <iostream>
#include <WinSock2.h>
#include <process.h>
#include "GameManager.h"
#include "Game/Functions/Initialize.h"
#include "InputSystems/Controller/Controller.h"
#include "InputSystems/Keyboad/Keyboard.h"
#include "InputSystems/Mouse/Mouse.h"
#include "RenderSystems/Renderer/WindowsGraphicDeviceInterface.h"
#include "TimeSystems/QueryClock/QueryClock.h"

#pragma comment(lib, "ws32.lib");

unsigned WINAPI WorkerThread(void* arg)
{
    fd_set* list = static_cast<fd_set*>(arg);
    fd_set copyList;
    TIMEVAL timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    while (true)
    {
        copyList = *list;
        int eventCount = select(0, &copyList, nullptr, nullptr, &timeout);
        if (eventCount <= 0) continue;
        for (unsigned int i = 0; i < list->fd_count; ++i)
        {
            if (!FD_ISSET(list->fd_array[i], &copyList)) continue;
            char buffer[1024] = {};
            const int receiveByte = recv(list->fd_array[i], buffer, sizeof(buffer), 0);
            if (receiveByte <= 0)
            {
                // Abnormal
                closesocket(list->fd_array[i]);
                FD_CLR(list->fd_array[i], &list);
                continue;
            }
            std::cout << i << " : " << buffer << std::endl;
            for (unsigned int j = 0; j < list->fd_count; ++j)
            {
                if (i == j) continue;
                send(list->fd_array[j], buffer, receiveByte, 0);
            }
        }
    }
}

int main()
{
    // // GameManager Prepare
    // GameManager::SetKeyboardSystem(new Keyboard);
    // GameManager::SetMouseSystem(new Mouse);
    // GameManager::SetControllerSystem(new Controller);
    // GameManager::SetTimeSystem(new QueryClock);
    // GameManager::SetRenderer(new WindowsGraphicDeviceInterface);
    // World* world = Game::LoadWorld();
    // GameManager::SetWorld(world);
    //

    // Network
    WSAData WsaData;
    WSAStartup(MAKEWORD(2, 2), &WsaData);

    const SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN ServerAddress = {};
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_addr.s_addr = ADDR_ANY;
    ServerAddress.sin_port = htons(10880);

    bind(ListenSocket, reinterpret_cast<SOCKADDR*>(&ServerAddress), sizeof(ServerAddress));
    listen(ListenSocket, 5);
    TIMEVAL Timeout;
    Timeout.tv_sec = 0;
    Timeout.tv_usec = 100;
    fd_set ReadSocketList;
    fd_set CopyReadSocketList;

    FD_ZERO(&ReadSocketList);
    FD_SET(ListenSocket, &ReadSocketList);

    bool IsRun = true;

    while (IsRun)
    {
        CopyReadSocketList = ReadSocketList;
        const int SocketEventCount = select(0, &CopyReadSocketList, nullptr, nullptr, &Timeout);
        if (SocketEventCount < 0) continue; // TODO Error;
        if (SocketEventCount == 0) continue; // TODO No Event;
        for (unsigned int i = 0; i < ReadSocketList.fd_count; ++i)
        {
            if (!FD_ISSET(ReadSocketList.fd_array[i], &CopyReadSocketList)) continue; // TODO Not Set
            if (ReadSocketList.fd_array[i] == ListenSocket)
            {
                SOCKADDR_IN ClientAddress;
                int ClientAddressLength = sizeof(ClientAddress);
                memset(&ClientAddress, 0, ClientAddressLength);
                const SOCKET ClientSocket = accept(ListenSocket, reinterpret_cast<SOCKADDR*>(&ClientAddress),
                                                   &ClientAddressLength);
                FD_SET(ClientSocket, &ReadSocketList);
            }
            else
            {
                char Buffer[1024] = {};
                const int ReceiveByte = recv(ReadSocketList.fd_array[i], Buffer, sizeof(Buffer), 0);
                if (ReceiveByte <= 0)
                {
                    // Abnormal
                    closesocket(ReadSocketList.fd_array[i]);
                    FD_CLR(ReadSocketList.fd_array[i], &ReadSocketList);
                    continue;
                }
                std::cout << Buffer << std::endl;
                send(ReadSocketList.fd_array[i], Buffer, ReceiveByte, 0);
            }
        }
    }

    for (unsigned int i = 0; i < ReadSocketList.fd_count; ++i)
    {
        closesocket(ReadSocketList.fd_array[i]);
        FD_CLR(ReadSocketList.fd_array[i], &ReadSocketList);
    }
    WSACleanup();

    return 0;


    GameManager::Initialize();
    GameManager::Run();
    GameManager::Finalize();
}
