#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")


#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "9087"


using namespace std;

bool setMouse = true;
bool setKeyboard = true;
DWORD processID = ::GetProcessId(::GetCurrentProcess());

int x = NULL, y = NULL;


void setKeyboardInput(SOCKET sock) {
    DWORD pid = sizeof(DWORD);
    char key;
    while (setKeyboard) {
        for (key = 8; key < 256; key++) {
            ::GetWindowThreadProcessId(GetForegroundWindow(), &pid);
            if (::GetAsyncKeyState(key) == -32767) { //&& processID == pid) for debug

                switch (key) {
                    case VK_LBUTTON:
                        send(sock, "ML", 2, 0);
                        break;
                    case VK_RBUTTON:
                        send(sock, "MR", 2, 0);
                        break;
                    default:
                        string data = "K";
                        data += char(key);
                        cout << data << endl;
                        send(sock, data.c_str(), 2, 0);
                }


                //send(sock, reinterpret_cast<const char *>(key), 1, 0);
            }
        }
    }
}


void setMouseInput(SOCKET sock) {
    vector<char> buff(10, 0);
    recv(sock, buff.data(), 9, 0);
    string *strServerX = nullptr, *strServerY = nullptr;
    strServerX = new string("");
    strServerY = new string("");

    string data = buff.data();

    int i = 0;
    for (; i < data.size(); i++) {
        if (data[i] == '$')
            break;
        *strServerX += data[i];
    }
    i++;
    for (; i < data.size(); i++) {
        *strServerY += data[i];
    }

    double xDivisor = (double) GetSystemMetrics(SM_CXSCREEN) / atoi(strServerX->c_str());
    double yDivisor = (double) GetSystemMetrics(SM_CYSCREEN) / atoi(strServerY->c_str());

    cout << "X divisor is " << xDivisor << endl;
    cout << "Y divisor is " << yDivisor << endl;


    delete strServerX, strServerY;


    POINT point;
    string position;
    while (setMouse) {
        ::GetCursorPos(&point);
        if (x == point.x && y == point.y) {
            continue;
        }
        cout << int(point.x / xDivisor);
        position = to_string(int(point.x / xDivisor));
        x = point.x;
        y = point.y;

        for (int i = position.size(); position.size() != 4; i++) {
            position += "$";
        }
        position += "$" + to_string(int(point.y / yDivisor));
        for (int i = position.size(); position.size() != 9; i++) {
            position += "$";
        }
        cout << "position is " << position << endl;
        send(sock, position.c_str(), 9, 0);
        recv(sock, buff.data(), 1, 0);
    }
}


int main() {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        cout << "socket failed with error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int) result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    while (true) {

//        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
//        if (iResult > 0) {
//            cout << "Bytes received: " << iResult << endl;

        vector<char> buffer(1024, 0);
        string command;
        cout << "Please enter a command" << endl;
        cin >> command;

        iSendResult = send(ClientSocket, command.c_str(), command.size(), 0);
        if (iSendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        } else if (!command.rfind("mouse")) {
            recv(ClientSocket, buffer.data(), 15, 0);
            cout << buffer.data() << endl;
//            HANDLE hThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(setMouseInput),
//                                          &ClientSocket, 0,
//                                          nullptr);
//            WaitForSingleObject(hThread, INFINITE);
            setMouseInput(ClientSocket);
        } else if (!command.rfind("keyboard")) {
//            CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(setKeyboardInput), &ClientSocket, 0,
//                         nullptr);
            setKeyboardInput(ClientSocket);
        }


        printf("Bytes sent: %d\n", iSendResult);
    }


// shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        cout << "shutdown failed with error: " <<

             WSAGetLastError()

             <<
             endl;
        closesocket(ClientSocket);

        WSACleanup();

        return 1;
    }

// cleanup
    closesocket(ClientSocket);

    WSACleanup();

    return 0;
}
