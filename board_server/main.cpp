#define  _WINSOCK_DEPRECATED_NO_WARNINGS
//
#include <WinSock2.h>
#include <iostream>
#include <string>
//
#include "jdbc/mysql_connection.h"
#include "jdbc/cppconn/driver.h"
#include "jdbc/cppconn/exception.h"
#include "jdbc/cppconn/prepared_statement.h"
//
#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "debug/mysqlcppconn.lib")
//
#define SERVER_IPV4 "0.0.0.0"
#define SERVER_PORT 4949
#define PACKET_SIZE 100
//
using namespace std;
//
const string server = "tcp://127.0.0.1:3306";
const string username = "root";
const string password = "1234";


// ?ѱ?.
std::string MultiByteToUtf8(std::string multibyte_str)
{
    char* pszIn = new char[multibyte_str.length() + 1];
    strncpy_s(pszIn, multibyte_str.length() + 1, multibyte_str.c_str(), multibyte_str.length());

    std::string resultString;

    int nLenOfUni = 0, nLenOfUTF = 0;
    wchar_t* uni_wchar = NULL;
    char* pszOut = NULL;

    // 1. ANSI(multibyte) Length
    if ((nLenOfUni = MultiByteToWideChar(CP_ACP, 0, pszIn, (int)strlen(pszIn), NULL, 0)) <= 0)
        return 0;

    uni_wchar = new wchar_t[nLenOfUni + 1];
    memset(uni_wchar, 0x00, sizeof(wchar_t) * (nLenOfUni + 1));

    // 2. ANSI(multibyte) ---> unicode
    nLenOfUni = MultiByteToWideChar(CP_ACP, 0, pszIn, (int)strlen(pszIn), uni_wchar, nLenOfUni);

    // 3. utf8 Length
    if ((nLenOfUTF = WideCharToMultiByte(CP_UTF8, 0, uni_wchar, nLenOfUni, NULL, 0, NULL, NULL)) <= 0)
    {
        delete[] uni_wchar;
        return 0;
    }

    pszOut = new char[nLenOfUTF + 1];
    memset(pszOut, 0, sizeof(char) * (nLenOfUTF + 1));

    // 4. unicode ---> utf8
    nLenOfUTF = WideCharToMultiByte(CP_UTF8, 0, uni_wchar, nLenOfUni, pszOut, nLenOfUTF, NULL, NULL);
    pszOut[nLenOfUTF] = 0;
    resultString = pszOut;

    delete[] uni_wchar;
    delete[] pszOut;

    return resultString;
}

int main(int argc, char* argv[])
{
    // sql ????
    sql::Driver* driver = nullptr;
    sql::Connection* con = nullptr;
    sql::Statement* stmt = nullptr;
    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* rs = nullptr;
    // sql ????
    driver = get_driver_instance();
    con = driver->connect(server, username, password);
    con->setSchema("test");
    cout << "?????ͺ??̽? ???Ӽ???!" << endl;

    bool bProgramRunning = true;
    bool bNetworkConnected = false;
    // winsock ????
    WSAData WSAdata = { 0, };
    if (WSAStartup(MAKEWORD(2, 2), &WSAdata) != 0) { exit(-1); }
    SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == INVALID_SOCKET) { exit(-2); }
    SOCKADDR_IN ServerSockAddr = { 0, };
    ServerSockAddr.sin_family = AF_INET; // IPV4
    ServerSockAddr.sin_addr.S_un.S_addr = inet_addr(SERVER_IPV4);
    ServerSockAddr.sin_port = htons(SERVER_PORT);
    if (::bind(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr)) != 0) { exit(-3); };
    if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) { exit(-4); };
    
    SOCKET ClientSocket = NULL;

    while (bProgramRunning)
    {
        if (!bNetworkConnected)
        {   
            // Ŭ?? ???? ????, ????
            SOCKADDR_IN ClientAddrIn = { 0, };
            int ClientLength = sizeof(ClientAddrIn);
            cout << "Ŭ???̾?Ʈ ???Ӵ???!" << endl;
            ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddrIn, &ClientLength);
            if (ClientSocket == INVALID_SOCKET)
            {
                closesocket(ClientSocket);
                continue;
            }
            else
            {
                // Ŭ?? ???? ?Ϸ?
                cout << "Ŭ???̾?Ʈ ???ӿϷ?!!!" << endl;
                bNetworkConnected = true;
            }
        }
        // ?޽??? ???? ????
        char Buffer[PACKET_SIZE] = { 0, };
        int RecvBytes = recv(ClientSocket, Buffer, sizeof(Buffer), 0);

        if (RecvBytes <= 0)
        {
            // ???? ????
            bNetworkConnected = false;
            continue;
        }
        Buffer[PACKET_SIZE - 1] = '\0';
        // ???? ?޽??? string ??ȯ
        string strCONTENTS = Buffer;
        if (strCONTENTS == "EXITSERVER")
        {
            // ???? ?޽??? EXITSERVER ?϶? ????
            bProgramRunning = false;
            break;
        }

        // ???? ?޽??? DB ????
        pstmt = con->prepareStatement("INSERT INTO BOARD(`CONTENTS`)VALUES(?)");
        pstmt->setString(1, MultiByteToUtf8(strCONTENTS));
        pstmt->execute();
        cout << "???? ?޽??? : " << strCONTENTS << endl;

        int SendBytes = send(ClientSocket, Buffer, sizeof(Buffer), 0);
        cout << "SendBytes :" << SendBytes << endl;

    }
    closesocket(ClientSocket);
    closesocket(ServerSocket);

    WSACleanup();
    return 0;
}
