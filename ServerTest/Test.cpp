#include <iostream>
#include <WinSock2.h>
#include <process.h>"

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

int main(int argc, char* argv[])
{
    WSAData WsaData;
    WSAStartup(MAKEWORD(2, 2), &WsaData);

    const SOCKET ListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN ListenAddress = {};
    ListenAddress.sin_family = AF_INET;
    ListenAddress.sin_addr.s_addr = INADDR_ANY;
    ListenAddress.sin_port = htons(22222);

    bind(ListenSocket, (SOCKADDR*)&ListenAddress, sizeof(ListenAddress));

    listen(ListenSocket, 5);
    fd_set ClientSocketList;
    FD_ZERO(&ClientSocketList);

    HANDLE ThreadHandle = (HANDLE)_beginthreadex(0, 0, WorkerThread, &ClientSocketList, 0, 0);

    while (true)
    {
        SOCKADDR_IN ClientAddress = {};
        int ClientAddressSize = sizeof(ClientAddress);
        SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddress, &ClientAddressSize);
        FD_SET(ClientSocket, &ClientSocketList);
    }

    closesocket(ListenSocket);
    WSACleanup();


    return 0;
}
