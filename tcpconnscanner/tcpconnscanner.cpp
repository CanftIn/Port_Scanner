#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>

#pragma comment (lib, "ws2_32.lib")

int main(int argc, char **argv)
{
	char *TargetIpAddr = "192.168.1.106";
	unsigned int StartPort = 1540;
	unsigned int EndPoint = 1560;
	SOCKET ScanSocket;
	struct sockaddr_in TargetAddr_in;
	int Ret;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO bInfo;
	GetConsoleScreenBufferInfo(hCon, &bInfo);
	WSADATA wsaData;
	if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup failed with error: %d \n", Ret);
		return -1;
	}
	DWORD dwStart = GetTickCount();
	printf("Target IP: %s \n", TargetIpAddr);
	for (unsigned int i = StartPort; i <= EndPoint; ++i)
	{
		ScanSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (ScanSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %d \n", WSAGetLastError());
			return -1;
		}
		TargetAddr_in.sin_family = AF_INET;
		TargetAddr_in.sin_addr.s_addr = inet_addr(TargetIpAddr);
		TargetAddr_in.sin_port = htons(i);
		if (connect(ScanSocket, (struct sockaddr*)&TargetAddr_in, sizeof(TargetAddr_in)) == SOCKET_ERROR)
		{
			SetConsoleTextAttribute(hCon, 10);
			printf("Port %5d Close \n", i);
		}
		else
		{
			SetConsoleTextAttribute(hCon, 14);
			printf("Port %5d Open \n", i);
		}
		if (closesocket(ScanSocket) == SOCKET_ERROR)
		{
			printf("closesocket failed with error: %d \n", WSAGetLastError());
			return -1;
		}
	}
	SetConsoleTextAttribute(hCon, 14);
	printf("\ntime: %dms \n", GetTickCount() - dwStart);
	SetConsoleTextAttribute(hCon, bInfo.wAttributes);
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("WSACleanup failed with error: %d \n", WSAGetLastError());
		return -1;
	}
	return 1;
}