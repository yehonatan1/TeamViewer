#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")
using namespace std;


bool mouseInput = true;
bool keyboardInput = true;


void getKeyboardInput(SOCKET sock) {


    //if buff[0] = K it's keyboard inputs and buff[1] = the character if buff[0] = M it's mouse inputs and buff[1] = R or L, R means right click and L means left click
    vector<char> buff(3, 0);
    INPUT inputs[2];
    ZeroMemory(inputs, sizeof(inputs));
    while (keyboardInput) {
        recv(sock, buff.data(), 2, 0);
        string data = buff.data();
        cout << "buff is " << buff.data() << endl;
        if (buff[0] == 'K') {
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = data[1];

            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = data[1];
            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

            UINT uSent = SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
            ::SendInput(ARRAYSIZE(inputs), inputs, sizeof(inputs));
            continue;
        }
        if (buff[1] == 'R') {
            inputs[0].type = INPUT_MOUSE;
            inputs[0].mi.mouseData = 0;
            inputs[0].mi.dwFlags = (MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP);
        } else {
            inputs[0].type = INPUT_MOUSE;
            inputs[0].mi.mouseData = 0;
            inputs[0].mi.dwFlags = (MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP);
        }
        SendInput(1, &inputs[0], sizeof(inputs[0]));
    }
}


void getMouseInput(SOCKET sock) {
    vector<char> buff(10, 0);

    string resolution = to_string(GetSystemMetrics(SM_CXSCREEN)) + "$" + to_string(GetSystemMetrics(SM_CYSCREEN));
    send(sock, resolution.c_str(), resolution.size(), 0);


    while (mouseInput) {
        ZeroMemory(buff.data(), 10);
        recv(sock, buff.data(), 9, 0);
        string data = buff.data();
        cout << "buff is" << data << endl;

        string strX, strY;

        for (int i = 0; i < 4; i++) {
            if (data[i] == '$') {
                break;
            }
            strX += data[i];
        }
        for (int i = 4; i < data.size(); i++) {
            if (data[i] == '$') {
                continue;
            }
            strY += data[i];
        }
        int x = atoi(strX.c_str());
        int y = atoi(strY.c_str());
        cout << "(" << x << "," << y << ")" << endl;
        ::SetCursorPos(x, y);
        send(sock, " ", 1, 0);
    }
}


void client() {
    //string ipAdress = "141.226.121.68"; //IP Address of the server
    string ipAdress = "192.168.1.210"; //IP Address of the server
    int port = 9087; //Listtening port # on server


    // Initialize Winsock

    WSADATA data;
    WORD ver = MAKEWORD(2, 2);
    int wsResualt = WSAStartup(ver, &data);

    if (wsResualt != 0) {
        cerr << "Cant start Winsock";
        return;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET) {
        cerr << "Cant create socket" << WSAGetLastError << endl;
        WSACleanup();
        return;
    }
    // Fill in a hint structure
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, ipAdress.c_str(), &hint.sin_addr);

    // Connect to serverT
    int connResult = connect(sock, (sockaddr *) &hint, sizeof(hint));
    if (connResult == SOCKET_ERROR) {
        cerr << "Cant connect to server" << WSAGetLastError << endl;
        closesocket(sock);
        WSACleanup();
        return;
    }

    //Do-while loop to send and receive data
    vector<char> buf(1024, 0);
    string userInput;

    while (true) {
        cout << "hey2" << endl;
        ZeroMemory(buf.data(), 1024);
        int bytesRecived = recv(sock, buf.data(), 1024, 0);
        send(sock, "Got the message", 15, 0);
        string command = buf.data();
        if (!command.rfind("mouse")) {
            HANDLE hThread = ::CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(getMouseInput),
                                            reinterpret_cast<LPVOID>(sock),
                                            0,
                                            nullptr);
            WaitForSingleObject(hThread, INFINITE);
//            getMouseInput(sock);
            continue;
        } else if (!command.rfind("keyboard")) {
            HANDLE hThread = ::CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(getKeyboardInput),
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(sock), 0, nullptr);
            WaitForSingleObject(hThread, INFINITE);
            //getKeyboardInput(sock);
        }
    }



    // Gracefully close down everything
    closesocket(sock);
    WSACleanup();
}


int main() {
    client();
    return 0;
}
