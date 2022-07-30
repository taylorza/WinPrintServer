// WinPrintServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <WinSock2.h>
#include <stdarg.h>

std::wstring printerName = L"EPSON098CEF (WF-3520 Series)";
int printerPort = 9100;

void log(LPCWSTR format, ...);
void logFatal(LPCWSTR msg, int errCode);
void logError(LPCWSTR msg, int errCode);

void showUsage();

int wmain(int argc, wchar_t* argv[])
{
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;

    int status = WSAStartup(wVersionRequested, &wsaData);
    if (status != 0)
    {
        logFatal(L"failed to initialize the socket library", status);
    }

    if (argc == 1) 
    {
        DWORD cb;
        if (!GetDefaultPrinter(NULL, &cb) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
        {
            logFatal(L"no default printer defined", GetLastError());
        }
        LPWSTR szBuffer = new WCHAR[cb];
        GetDefaultPrinter(szBuffer, &cb);
        printerName = szBuffer;
        delete[]szBuffer;
    }
    else if (argc == 2) 
    {
        if (_wcsicmp(argv[1], L"-h") == 0)
        {
            showUsage();
        }
        printerName = argv[1];
    }
    else 
    {
        showUsage();
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        logFatal(L"failed to create server socket", WSAGetLastError());
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(printerPort);
    addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (const sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(serverSocket);
        logFatal(L"failed to bind server socket", WSAGetLastError());
    }

    sockaddr_in client{};
    int size = sizeof(client);
    SOCKET clientSocket = INVALID_SOCKET;
    HANDLE hPrinter = INVALID_HANDLE_VALUE;

    log(L"Print Server Started for '%s'", printerName.c_str());
    while (listen(serverSocket, 5) != SOCKET_ERROR)
    {
        log(L"waiting for connection");
        clientSocket = accept(serverSocket, (sockaddr*)&client, &size);
        if (clientSocket == INVALID_SOCKET) 
        {
            logError(L"failed to accept client connection", WSAGetLastError());
            continue;
        }
        log(L"connection from %d.%d.%d.%d", client.sin_addr.S_un.S_un_b.s_b1, client.sin_addr.S_un.S_un_b.s_b2, client.sin_addr.S_un.S_un_b.s_b3, client.sin_addr.S_un.S_un_b.s_b4);
        
        DOC_INFO_1 di{};
        di.pDocName = const_cast<LPWSTR>(L"RAW Print Job");
        di.pOutputFile = NULL;
        di.pDatatype = const_cast<LPWSTR>(L"XPS_PASS");

        if (!OpenPrinter(const_cast<LPWSTR>(printerName.c_str()), &hPrinter, NULL))
        {
            logError(L"printer not available", GetLastError());
            goto cleanup; // Yes, yes I am sure you can do better.
        }

        if (StartDocPrinter(hPrinter, 1, (LPBYTE)&di) <= 0)
        {
            logError(L"failed to start the print job", GetLastError());
            goto cleanup;
        }

        log(L"print job started");
        char buffer[4096];
        DWORD bytesWritten;
        while (true)
        {
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0)
            {
                break;
            }
            BOOL result = WritePrinter(hPrinter, buffer, bytesRead, &bytesWritten);
            if (!result || bytesWritten != bytesRead)
            {
                logError(L"print failed", GetLastError());
                break;
            }
        }
        EndDocPrinter(hPrinter);
        log(L"print job completed");

    cleanup:
        if (clientSocket != INVALID_SOCKET)
        {
            closesocket(clientSocket);
        }

        if (hPrinter != INVALID_HANDLE_VALUE)
        {
            ClosePrinter(hPrinter);
        }
    }
    closesocket(serverSocket);
    WSACleanup();
}

void logFatal(LPCWSTR msg, int errCode)
{
    std::wcerr << L"fatal: " << msg << L"(" << errCode << L")" << std::endl;
    exit(-1);
}

void logError(LPCWSTR msg, int errCode)
{
    std::wcerr << L"error: " << msg << L"(" << errCode << L")" << std::endl;
}

void log(LPCWSTR format, ...)
{
    va_list args;
    va_start(args, format);
    vwprintf(format, args);
    va_end(args);
    printf("\r\n");
}

void showUsage()
{
    log(L"WinPrintServer v1.0\r\n");
    log(L"WinPrintServer [options] [printername]");
    log(L"  -h           Show this help information");
    log(L"  printername  The name of the printer to print. If not specified the default printer is used");
    exit(-1);
}

