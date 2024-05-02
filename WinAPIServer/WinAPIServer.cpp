#include <exception>
#include <iostream>
#include <list>
#include <WinSock2.h>
#include <process.h>

#include "Packet.h"

#pragma comment(lib, "ws2_32")

constexpr int MaximumPlayerCount = 2;
const wchar_t* CharacterNames[2] = {L"Woodcutter", L"GraveRobber"};
bool PlayersExist[2] = {false, false};
Location PlayersLocation[2] = {};
SOCKET clientSockets[2] = {INVALID_SOCKET, INVALID_SOCKET};

unsigned WINAPI WorkerThread(void* argument);

int main()
{
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listenSocket = {};
    fd_set clientSocketList;
    try
    {
        listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) throw std::exception("Listen socket is invalid.");
        SOCKADDR_IN listenAddress = {};
        listenAddress.sin_family = AF_INET;
        listenAddress.sin_addr.s_addr = INADDR_ANY;
        listenAddress.sin_port = htons(18080);
        int result = bind(listenSocket, reinterpret_cast<SOCKADDR*>(&listenAddress), sizeof(listenAddress));
        if (result < 0) throw std::exception("Bind fail");
        result = listen(listenSocket, 2);
        if (result < 0) throw std::exception("Listen fail");

        FD_ZERO(&clientSocketList);

        HANDLE threadHandle = (HANDLE)_beginthreadex(nullptr, NULL, WorkerThread, &clientSocketList,
                                                     NULL, nullptr);

        while (true)
        {
            SOCKADDR_IN clientAddress = {};
            int clientAddressSize = sizeof(clientAddress);
            SOCKET clientSocket = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddress), &clientAddressSize);
            if (clientSocket == INVALID_SOCKET) throw std::exception("Accept fail.");

            // Connect Sequence
            Header header = {};
            header.length = sizeof(Connection);
            header.type = Connect;
            result = send(clientSocket, reinterpret_cast<char*>(&header), sizeof(header), 0);
            if (result < 0) throw std::exception("Connection header send fail.");

            Connection connection = {};
            for (int i = 0; i < MaximumPlayerCount; ++i)
            {
                if (!connection.otherPlayerExist && PlayersExist[i])
                {
                    connection.otherPlayerExist = true;
                    connection.otherPlayerLocation = PlayersLocation[i];
                    wcscpy_s(connection.otherCharacterName, CharacterNames[i]);
                }
                if (!connection.canConnect && !PlayersExist[i])
                {
                    connection.canConnect = true;
                    wcscpy_s(connection.characterName, CharacterNames[i]);
                    PlayersExist[i] = true;
                    clientSockets[i] = clientSocket;
                }
            }
            result = send(clientSocket, reinterpret_cast<char*>(&connection), sizeof(connection), 0);
            if (result < 0) throw std::exception("Connection data send fail.");


            // Other player connect sequence
            if (connection.canConnect)
            {
                Header otherHeader = {};
                otherHeader.length = sizeof(OtherConnection);
                otherHeader.type = OtherConnect;
                OtherConnection otherConnection = {};
                wcscpy_s(otherConnection.characterName, connection.characterName);
                for (unsigned i = 0; i < clientSocketList.fd_count; ++i)
                {
                    result = send(clientSocketList.fd_array[i], reinterpret_cast<char*>(&otherHeader),
                                  sizeof(otherHeader), 0);
                    if (result < 0) throw std::exception("Other connection header send fail.");
                    result = send(clientSocketList.fd_array[i], reinterpret_cast<char*>(&otherConnection),
                                  sizeof(otherConnection), 0);
                    if (result < 0) throw std::exception("Other connection data send fail.");
                }
            }
            FD_SET(clientSocket, &clientSocketList);
        }
    }
    catch (const std::exception& exception)
    {
        std::cout << "[" << GetLastError() << "] " << exception.what() << std::endl;
    }
    for (unsigned i = 0; i < clientSocketList.fd_count; ++i)
    {
        closesocket(clientSocketList.fd_array[i]);
    }
    closesocket(listenSocket);

    WSACleanup();
    return 0;
}

unsigned WorkerThread(void* argument)
{
    fd_set* list = static_cast<fd_set*>(argument);
    fd_set copyList = {};
    TIMEVAL timeout = {};
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;
    while (true)
    {
        copyList = *list;
        int eventCount = select(0, &copyList, nullptr, nullptr, &timeout);
        if (eventCount <= 0) continue;
        for (unsigned i = 0; i < list->fd_count; ++i)
        {
            if (!FD_ISSET(list->fd_array[i], &copyList)) continue;
            Header header = {};
            int receiveByte = recv(list->fd_array[i], reinterpret_cast<char*>(&header), sizeof(header), MSG_WAITALL);
            if (receiveByte <= 0)
            {
                // Abnormal sequence (Disconnect)
                const SOCKET deleteSocket = list->fd_array[i];
                for (int j = 0; j < MaximumPlayerCount; ++j)
                {
                    if (clientSockets[j] == deleteSocket)
                    {
                        PlayersExist[j] = false;
                        PlayersLocation[j] = {};
                        clientSockets[j] = INVALID_SOCKET;
                    }
                    else
                    {
                        Header header = {};
                        header.type = Disconnect;
                        send(clientSockets[j], reinterpret_cast<char*>(&header), sizeof(header), 0);
                    }
                }
                FD_CLR(deleteSocket, list);
                closesocket(deleteSocket);
                continue;
            }
            // Normal Sequence
            switch (header.type)
            {
            case PlayerLocation:
                {
                    Location location = {};
                    receiveByte = recv(list->fd_array[i], reinterpret_cast<char*>(&location), header.length,
                                       MSG_WAITALL);
                    if (receiveByte <= 0) throw std::exception("Receive player location fail.");
                    header = {};
                    header.type = OtherLocation;
                    header.length = sizeof(Location);
                    for (const SOCKET clientSocket : clientSockets)
                    {
                        if (clientSocket == INVALID_SOCKET || clientSocket == list->fd_array[i]) continue;
                        int result = send(clientSocket, reinterpret_cast<char*>(&header), sizeof(header), 0);
                        if (result < 0) throw std::exception("Send other location header fail.");
                        result = send(clientSocket, reinterpret_cast<char*>(&location), sizeof(location), 0);
                        if (result < 0) throw std::exception("Send other location data fail.");
                    }
                }
                break;
            default:
                throw std::exception("Receive unknown packet.");
            }
        }
    }
}
