#pragma comment (lib, "Ws2_32.lib")


#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <wchar.h>
#include <stdio.h>
#include <vector>
#include <time.h>
#include <list>
#include <queue>
#include "ChatMessage.h"




#define CMD_START_SERVER  1001
#define CMD_STOP_SERVER   1002
#define MAX_MESSAGES      100

std::list<ChatMessage*> messages;
std::vector<ChatMessage*> usernames;

HINSTANCE hInst;
HWND grpEndpoint, grpLog, serverLog;
HWND btnStart, btnStop;
HWND editIP, editPort;
SOCKET listenSocket;


LRESULT CALLBACK  WinProc(HWND, UINT, WPARAM, LPARAM);
DWORD   CALLBACK  CreateUI(LPVOID);  // User Interface
DWORD   CALLBACK  StartServer(LPVOID);
DWORD   CALLBACK  StopServer(LPVOID);
char*			  SerializeMessages();



int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_     PWSTR cmdLine, _In_     int showMode) {
	hInst = hInstance;

	const WCHAR WIN_CLASS_NAME[] = L"ServerWindow";
	WNDCLASS wc = { };
	wc.lpfnWndProc = WinProc;
	wc.hInstance = hInst;
	wc.lpszClassName = WIN_CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	ATOM mainWin = RegisterClassW(&wc);
	if (mainWin == FALSE) {
		MessageBoxW(NULL, L"error",
			L"error Server", MB_OK | MB_ICONSTOP);
		return -1;
	}

	HWND hwnd = CreateWindowExW(0, WIN_CLASS_NAME,
		L"TCP Chat - Server", WS_OVERLAPPEDWINDOW,
		700, 100, 640, 480, NULL, NULL, hInst, NULL);
	if (hwnd == NULL) {
		MessageBoxW(NULL, L"error",
			L"error Server", MB_OK | MB_ICONSTOP);
		return -2;
	}

	ShowWindow(hwnd, showMode);
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:  CreateUI(&hWnd);    break;
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hWnd, &ps);
		FillRect(dc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_CTLCOLORSTATIC: {
		HDC  dc = (HDC)wParam;
		HWND ctl = (HWND)lParam;
		if (ctl != grpEndpoint
			&& ctl != grpLog) {
			SetBkMode(dc, TRANSPARENT);
			SetTextColor(dc, RGB(20, 20, 50));
		}
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}
	case WM_COMMAND: {
		int cmd = LOWORD(wParam);
		int ntf = HIWORD(wParam);
		switch (cmd) {
		case CMD_START_SERVER: CreateThread(NULL, 0, StartServer, &hWnd, 0, NULL); break;
		case CMD_STOP_SERVER:  StopServer(&hWnd);  break;
		}
		break;
	}
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}


DWORD CALLBACK CreateUI(LPVOID params) {
	HWND hWnd = *((HWND*)params);
	grpEndpoint = CreateWindowExW(0, L"Button", L"EndPoint", BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
		10, 10, 150, 90, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Static", L"IP:", WS_CHILD | WS_VISIBLE,
		20, 35, 20, 15, hWnd, 0, hInst, NULL);
	editIP = CreateWindowExW(0, L"Edit", L"127.0.0.1", WS_CHILD | WS_VISIBLE,
		45, 35, 110, 17, hWnd, 0, hInst, NULL);
	CreateWindowExW(0, L"Static", L"Port:", WS_CHILD | WS_VISIBLE,
		20, 65, 30, 15, hWnd, 0, hInst, NULL);
	editPort = CreateWindowExW(0, L"Edit", L"8888", WS_CHILD | WS_VISIBLE,
		65, 65, 50, 17, hWnd, 0, hInst, NULL);

	grpLog = CreateWindowExW(0, L"Button", L"Server log", BS_GROUPBOX | WS_CHILD | WS_VISIBLE,
		170, 10, 300, 300, hWnd, 0, hInst, NULL);
	serverLog = CreateWindowExW(0, L"Listbox", L"", WS_CHILD | WS_VISIBLE,
		180, 30, 280, 280, hWnd, 0, hInst, NULL);

	btnStart = CreateWindowExW(0, L"Button", L"Start server", WS_CHILD | WS_VISIBLE,
		30, 120, 100, 25, hWnd, (HMENU)CMD_START_SERVER, hInst, NULL);
	btnStop = CreateWindowExW(0, L"Button", L"Stop server", WS_CHILD | WS_VISIBLE,
		30, 160, 100, 25, hWnd, (HMENU)CMD_STOP_SERVER, hInst, NULL);
	EnableWindow(btnStop, FALSE);

	listenSocket = INVALID_SOCKET;

	return 0;
}

DWORD CALLBACK StartServer(LPVOID params) {
	HWND hWnd = *((HWND*)params);

	const size_t MAX_LEN = 100;
	WCHAR str[MAX_LEN];

	WSADATA wsaData;
	int err;

	// WinSock API initializing (~ wsaData = new WSA(2.2) )
	err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		_snwprintf_s(str, MAX_LEN,
			L"Startup failed, error %d", err);
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -10;
	}

	// Socket preparing
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		_snwprintf_s(str, MAX_LEN,
			L"Socket failed, error %d", WSAGetLastError());
		WSACleanup();
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -20;
	}

	// -- Socket configuration --
	SOCKADDR_IN addr;   // Config structure

	addr.sin_family = AF_INET;  // 1. Network type (family)

	char ip[20];
	LRESULT ipLen = SendMessageA(editIP, WM_GETTEXT, 19, (LPARAM)ip);
	ip[ipLen] = '\0';
	inet_pton(AF_INET, ip, &addr.sin_addr);  // 2. IP

	char port[8];
	LRESULT portLen = SendMessageA(editPort, WM_GETTEXT, 7, (LPARAM)port);
	port[portLen] = '\0';
	addr.sin_port = htons(atoi(port));  // 3. Port

	
	// -- end configuration of [addr] --

	// Socket binding - config [addr] applying to socket
	err = bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr));
	if (err == SOCKET_ERROR) {
		_snwprintf_s(str, MAX_LEN,
			L"Socket bind, error %d", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		listenSocket = INVALID_SOCKET;
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		return -30;
	}

	// Start of listening - from this point Socket receives data from OS
	err = listen(listenSocket, SOMAXCONN);
	if (err == SOCKET_ERROR) {
		_snwprintf_s(str, MAX_LEN,
			L"Socket listen, error %d", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		listenSocket = INVALID_SOCKET;
		SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
		
		return -40;
	}

	// Log start message
	

	WCHAR* ipW = new WCHAR[20];
	WCHAR* portW = new WCHAR[8];
	
	mbstowcs(ipW, ip, 20);
	mbstowcs(portW, port, 8);
	
	WCHAR strStart[200];
	_snwprintf(strStart, 200, L"Server starts %s::%s", ipW, portW);
	delete[] ipW;
	delete[] portW;
	SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)strStart);
	EnableWindow(btnStop, TRUE);
	EnableWindow(btnStart, FALSE);

	

	SOCKET acceptSocket;	// second Socket - for communication
	const size_t BUFF_LEN = 8;
	const size_t DATA_LEN = 2048;
	char buff[BUFF_LEN + 1];	// small buffer for transfered chunk (+ '\0')
	char data[DATA_LEN];	// big buffer for all transfered chunks
	int receivedCnt;		// chunk size

	messages.clear();

	while (true) {
		// wait for network activity
		acceptSocket = accept(listenSocket, NULL, NULL);
		
		if (acceptSocket == INVALID_SOCKET) {
			_snwprintf_s(str, MAX_LEN,
				L"Accept socket error %d", WSAGetLastError());
			closesocket(acceptSocket);
			SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
			return -60;
		}
		// from this point communication begins
		data[0] = '\0';
		do {
			receivedCnt = recv(acceptSocket, buff, BUFF_LEN, 0);
			if (receivedCnt == 0) {	// 0 - connection closed by client
				closesocket(acceptSocket);
				SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)L"Connection closed");
				break;
			}
			if (receivedCnt < 0) {	// <0 - recieving error
				_snwprintf_s(str, MAX_LEN,
					L"Communication socket error %d", WSAGetLastError());
				closesocket(acceptSocket);
				SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)str);
				break;
			}
			buff[receivedCnt] = '\0';
			strcat(data, buff);	// data += chunk (buff)
		} while (buff[receivedCnt - 1] != '\0');	// '\0' - end of data

		// data is sum of all chunks from socket
		char* mts;
		
		if (strlen(data) == 0) {	// only \0 in request
			mts = SerializeMessages();
			
			send(acceptSocket, mts, strlen(mts) + 1, 0);
			
			delete[] mts;
			
		}
		else {
			// extract message from data
			ChatMessage* message = new ChatMessage();
			ChatMessage* username = new ChatMessage();
			if (message->parseString(data)) {
				//message->setDt(message->getDt() - 260000);
				messages.push_back(message);
				
				if (messages.size() > MAX_MESSAGES) {
					delete messages.front();
					messages.pop_front();
				}
				SerializeMessages();
				mts = message->ToString();
				
				
				SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)mts);
				
				

				mts = SerializeMessages();
			


				// send answer to client - write in socket
				send(acceptSocket, mts, strlen(mts) + 1, 0);
			
				//delete[] mts;

			}
			else {
				delete message;
				SendMessageA(serverLog, LB_ADDSTRING, 0, (LPARAM)data);
				send(acceptSocket, "500", 4, 0);
			}
		}

		// closing socket
		shutdown(acceptSocket, SD_BOTH);
		closesocket(acceptSocket);

	}

	return 0;
}

DWORD CALLBACK StopServer(LPVOID params) {
	HWND hWnd = *((HWND*)params);

	closesocket(listenSocket);
	WSACleanup();
	listenSocket = INVALID_SOCKET;

	SendMessageW(serverLog, LB_ADDSTRING, 0, (LPARAM)L"Server stops");
	EnableWindow(btnStop, FALSE);
	EnableWindow(btnStart, TRUE);

	return 0;
}

char* SerializeMessages() {
	// message->toString() --> char*
	// collect them
	// calc size and build string
	// ==> StringBuilder
	char* ret;
	size_t n = messages.size();

	if (n == 0) {
		ret = new char[1]{ '\0' };
		return ret;
	}
	char** strs = new char* [n];
	size_t total = 0, i = 0;
	for (auto it : messages) {
		strs[i] = it->ToString();
		total += strlen(strs[i]) + 1;	// +1 - delimiter char
		++i;
	}

	ret = new char[total + 1];
	ret[0] = '\0';
	for (size_t i = 0; i < n; i++)
	{
		strcat(ret, strs[i]);
		strcat(ret, "\r");
	}
	ret[total - 1] = '\0';
	return ret;
}