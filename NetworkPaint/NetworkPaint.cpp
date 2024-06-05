﻿#include "framework.h"
#include "NetworkPaint.h"
#include "Network.h"
#include "RingBuffer.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <locale.h>
#include <WS2tcpip.h>
#include <windowsx.h>
#pragma comment(lib,"ws2_32")
#pragma warning(disable : 4996)

constexpr int SERVER_PORT = 25000;
constexpr int HEADER_SIZE = sizeof(stHEADER);
constexpr int DRAW_PACKET_SIZE = sizeof(st_DRAW_PACKET);

RingBuffer g_sendRB;
RingBuffer g_recvRB;

// 전역 변수:
HINSTANCE hInst;                               
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HPEN g_hGridPen;
HBRUSH g_hTileBrush;
RECT g_MemDCRect;
SOCKET g_clientSock;
bool g_MouseOnClick = false;
int g_oldX;
int g_oldY;
#define UM_NETWORK ((WM_USER) + (1))
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
// 전역 변수 선언
void PrintErrorMessage(int netWorkEvent, int errCode)
{
    switch (netWorkEvent)
    {
    case FD_CLOSE:
        printf("closesocket() error code : %d\n", errCode);
        __debugbreak();
        break;
    case FD_CONNECT:
        printf("connect() error code : %d\n", errCode);
        __debugbreak();
        break;
    }
}
void ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int netWorkEvent = WSAGETSELECTEVENT(lParam);
    int errCode = WSAGETSELECTERROR(lParam);
    if (errCode != 0)
    {
        PrintErrorMessage(netWorkEvent, errCode);
        PostMessage(hWnd, WM_DESTROY, 0, 0);
        return;
    }

    switch (netWorkEvent)
    {
    case FD_CLOSE:
        break;
    case FD_CONNECT:
        break;
    case FD_WRITE:
        SendEvent();
        break;
    case FD_READ:
        RecvEvent();
        break;
    }
}

void RecvEvent(void)
{

}
bool sendSizeRingBufferDirect(int size)
{
    int sendRet = 0;
    while (size > 0)
    {
        sendRet = send(g_clientSock, g_sendRB.GetReadStartPtr(), size, 0);
        if (sendRet == SOCKET_ERROR)
        {
            int errCode = WSAGetLastError();
            if (errCode != WSAEWOULDBLOCK)
            {
                printf("send() func error code : %d\n", WSAGetLastError());
                PostQuitMessage(WM_DESTROY);
            }
            return false;
        }
        size -= sendRet;
        g_sendRB.MoveFront(sendRet);
    }
    return true;
}
void SendEvent(void)
{
    int DirectDequeueSize = g_sendRB.DirectDequeueSize();
    int useSize = g_sendRB.GetUseSize();
	int sendRet;
	if (DirectDequeueSize < useSize)
	{
        int remainSize = useSize - DirectDequeueSize;
        if (!sendSizeRingBufferDirect(DirectDequeueSize)) return;
        if (!sendSizeRingBufferDirect(remainSize)) return;
	}
    else
    {
        if (!sendSizeRingBufferDirect(useSize)) return;
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    hInst = hInstance;
    MSG msg;
    HWND hWnd;
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInst;
    wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NETWORKPAINT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TEST";
    wcex.hIconSm = NULL;
    RegisterClassExW(&wcex);
    hWnd = CreateWindowW(L"TEST", L"MyTest", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    setlocale(LC_ALL, "");
    
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup() func error code : %d\n", WSAGetLastError());
        return 0;
    }

    g_clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_clientSock == INVALID_SOCKET)
    {
        printf("socket() func error code : %d\n", WSAGetLastError());
        __debugbreak();
        return 0;
    }

    WCHAR szIpAddress[MAX_PATH] = { 0 };
    wprintf(L"IP 번호를 입력하세요 : ");
    wscanf_s(L"%s", szIpAddress, MAX_PATH);
    rewind(stdin);

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    InetPton(AF_INET, szIpAddress, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(SERVER_PORT);


    int ASret = WSAAsyncSelect(g_clientSock, hWnd, UM_NETWORK, FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE);
    if (ASret == SOCKET_ERROR)
    {
        printf("WSAAsyncSelect() func error code : %d\n", WSAGetLastError());
        __debugbreak();
        return 0;
    }

    int connectRet = connect(g_clientSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    switch (message)
    {
    case WM_CREATE:
    {
        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
    }
    break;
    case WM_PAINT:
    {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		// 그리기 작업 수행
		HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen);
		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hTileBrush);

		// 예시로 사각형 그리기
		Rectangle(hdc, 600, 10, 100, 100);

		// 이전 GDI 객체 복원
		SelectObject(hdc, hOldPen);
		SelectObject(hdc, hOldBrush);
		EndPaint(hWnd, &ps);
    }
    break;
    case WM_LBUTTONDOWN:
		g_MouseOnClick = true;
		break;
    case WM_LBUTTONUP:
        g_MouseOnClick = false;
        break;
    case WM_MOUSEMOVE:
    {
        int curX = GET_X_LPARAM(lParam);
        int curY = GET_Y_LPARAM(lParam);
        if (g_MouseOnClick)
        {
            stHEADER header;
            st_DRAW_PACKET drawPacket;
            header.Len = DRAW_PACKET_SIZE;
            drawPacket.iEndX = curX;
            drawPacket.iEndY = curY;
            drawPacket.iStartX = g_oldX;
            drawPacket.iStartY = g_oldY;

            if (g_sendRB.GetFreeSize() < HEADER_SIZE + DRAW_PACKET_SIZE)
                __debugbreak();

            int DirectEnqueueSize = g_sendRB.DirectEnqueueSize();
            if (DirectEnqueueSize >= HEADER_SIZE)
            {
                memcpy_s(g_sendRB.GetWriteStartPtr(), HEADER_SIZE, &header, HEADER_SIZE);
                g_sendRB.MoveRear(HEADER_SIZE);
            }
            else
            {
                memcpy_s(g_sendRB.GetWriteStartPtr(), DirectEnqueueSize, &header, DirectEnqueueSize);
                g_sendRB.MoveRear(DirectEnqueueSize);
                int temp = HEADER_SIZE - DirectEnqueueSize;
                memcpy_s(g_sendRB.GetWriteStartPtr(), temp, (char*)&header + DirectEnqueueSize, temp);
                g_sendRB.MoveRear(temp);
            }
            DirectEnqueueSize = g_sendRB.DirectEnqueueSize();
            if (DirectEnqueueSize >= DRAW_PACKET_SIZE)
            {
                memcpy_s(g_sendRB.GetWriteStartPtr(), DRAW_PACKET_SIZE, &drawPacket, DRAW_PACKET_SIZE);
                g_sendRB.MoveRear(DRAW_PACKET_SIZE);
            }
            else
            {
                memcpy_s(g_sendRB.GetWriteStartPtr(), DirectEnqueueSize, (char*)&drawPacket, DirectEnqueueSize);
                g_sendRB.MoveRear(DirectEnqueueSize);
                int temp = DRAW_PACKET_SIZE - DirectEnqueueSize;
                memcpy_s(g_sendRB.GetWriteStartPtr(), temp, (char*)&drawPacket + DirectEnqueueSize, temp);
                g_sendRB.MoveRear(temp);
            }
            writeEvent();
        }
        g_oldX = curX;
        g_oldY = curY;
    }
    case WM_DESTROY:
    {
        DeleteObject(g_hGridPen);
        DeleteObject(g_hTileBrush);
        closesocket(g_clientSock);
        PostQuitMessage(0);
    }
    break;
    case UM_NETWORK:
		ProcessSocketMessage(hWnd, message, wParam, lParam);
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
