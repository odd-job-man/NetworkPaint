#include "framework.h"
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
HWND g_hWnd;
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
void RecvEvent(void)
{
    int directEnqueueSize = g_recvRB.DirectEnqueueSize();
    int recvRet;
    if (directEnqueueSize == 0)
    {
        recvRet = recv(g_clientSock, g_recvRB.GetWriteStartPtr(), g_recvRB.GetFreeSize() - directEnqueueSize, 0);

    }
    else
    {
        recvRet = recv(g_clientSock, g_recvRB.GetWriteStartPtr(), directEnqueueSize, 0);
    }
    if (recvRet == 0)
    {
        __debugbreak();
        PostMessage(g_hWnd, WM_DESTROY, 0, 0);
        return;
    }
    else if (recvRet == SOCKET_ERROR)
    {
        int errCode = WSAGetLastError();
        if (errCode != WSAEWOULDBLOCK)
        {
            __debugbreak();
			printf("recv() func error code : %d\n", WSAGetLastError());
            PostMessage(g_hWnd, WM_DESTROY, 0, 0);
            return;
        }
        __debugbreak(); // 언제 우드블럭이 뜨는지, 사실은 안뜰거같은데 뭔가 보험성으로 넣어놓긴햇다, 아몰랑 예외처리
    }
    g_recvRB.MoveRear(recvRet);

    int peekRet;
    int dequeueRet;
    HDC hdc = GetDC(g_hWnd);
    while (true)
    {
        stHEADER header;
        st_DRAW_PACKET drawPacket;
        peekRet = g_recvRB.Peek(HEADER_SIZE,(char*)&header);
        if (peekRet == 0)
        {
            return;
        }

        if (header.Len != DRAW_PACKET_SIZE)
        {
            __debugbreak();
        }
        if (g_recvRB.GetUseSize() < HEADER_SIZE + DRAW_PACKET_SIZE)
        {
            return;
        }
        g_recvRB.MoveFront(HEADER_SIZE);
        dequeueRet = g_recvRB.Dequeue((char*)&drawPacket, DRAW_PACKET_SIZE);


        // 돌다리도 두들겨보는 예외처리
        if (dequeueRet < DRAW_PACKET_SIZE)
        {
            __debugbreak();
            PostMessage(g_hWnd, WM_DESTROY, 0, 0);
        }
        printf("recv()\n");
		printf("StartX : %d, StartY : %d ", drawPacket.iStartX, drawPacket.iStartY);
		printf("EndX: %d, EndY: %d\n", drawPacket.iEndX, drawPacket.iEndY);
        MoveToEx(hdc, drawPacket.iStartX, drawPacket.iStartY, NULL);
        LineTo(hdc, drawPacket.iEndX, drawPacket.iEndY);
    }
    ReleaseDC(g_hWnd, hdc);
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
                __debugbreak();
                PostMessage(g_hWnd, WM_DESTROY, 0, 0);
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


void ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int netWorkEvent = WSAGETSELECTEVENT(lParam);
    int errCode = WSAGETSELECTERROR(lParam);
    if (errCode != 0)
    {
        __debugbreak();
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
    g_hWnd = hWnd;
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

            printf("send()\n");
            printf("StartX : %d, StartY : %d ", drawPacket.iStartX, drawPacket.iStartY);
            printf("EndX: %d, EndY: %d\n", drawPacket.iEndX, drawPacket.iEndY);
            if (g_sendRB.GetFreeSize() < HEADER_SIZE + DRAW_PACKET_SIZE)
                __debugbreak();

            int ret;
            ret = g_sendRB.Enqueue((char*)&header, HEADER_SIZE);
            if (ret == 0)
            {
                __debugbreak();
                PostMessage(g_hWnd, WM_DESTROY, 0, 0);
            }
            ret = g_sendRB.Enqueue((char*)&drawPacket, DRAW_PACKET_SIZE);
            if (ret == 0)
            {
                __debugbreak();
                PostMessage(g_hWnd, WM_DESTROY, 0, 0);
            }
			SendEvent();
		}
        g_oldX = curX;
        g_oldY = curY;
    }
    break;
    case WM_DESTROY:
    {
        DeleteObject(g_hGridPen);
        DeleteObject(g_hTileBrush);
        __debugbreak();
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

