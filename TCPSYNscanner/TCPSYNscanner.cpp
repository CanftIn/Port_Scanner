#include <stdio.h>
#include <string.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>

#pragma comment(lib, "ws2_32.lib")

char *DestIpAddr = "192.168.1.6";

typedef struct IpHeader
{
	unsigned char Version_HLen;
	unsigned char TOS;
	unsigned short Length;
	unsigned short Ident;
	unsigned short Flags_Offset;
	unsigned char TTL;
	unsigned char Protocol;
	unsigned short Checksum;
	unsigned int SourceAddr;
	unsigned int DestinationAddr;
} Ip_Header;
#define URG 0x20 
#define ACK 0x10 
#define PSH 0x08 
#define RST 0x04 
#define SYN 0x02 
#define FIN 0x01 
typedef struct TcpHeader
{
	USHORT SrcPort;
	USHORT DstPort;
	unsigned int SequenceNum;
	unsigned int Acknowledgment;
	unsigned char HdrLen;
	unsigned char Flags;
	USHORT AdvertisedWindow;
	USHORT Checksum;
	USHORT UrgPtr;
} Tcp_Header;

int PacketAnalyzer(char*);
int SendTCPSYNPacket(int);

int main(int argc, char **argv)
{
	int PortStart = 20;//atoi(argv[1]);
	int PortEnd = 25; // atoi(argv[2]);
	
	int Result;
	char RecvBuf[65535] = { 0 };
	LARGE_INTEGER nFreq;
	char Name[255];
	LARGE_INTEGER StartTime;
	LARGE_INTEGER EndTime;
	HANDLE hCon;
	WSADATA wsaData;
	DWORD dwBufferLen[10];
	DWORD dwBufferInLen = 1;
	DWORD dwBytesReturned = 0;

	Result = WSAStartup(MAKEWORD(2, 1), &wsaData);
	if (Result == SOCKET_ERROR)
	{
		printf("WSAStartup failed with error: %d \n", Result);
		return -1;
	}
	SOCKET RecSocket = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (RecSocket == SOCKET_ERROR)
	{
		printf("socket failed with error: %d \n", WSAGetLastError());
		closesocket(RecSocket);
		return -1;
	}
	Result = gethostname(Name, 255);
	if (Result == SOCKET_ERROR)
	{
		printf("gethostname failed with error %d\n", WSAGetLastError());
		closesocket(RecSocket);
		return 0;
	}
	struct hostent *pHostent = (struct hostent*)malloc(sizeof(struct hostent));
	pHostent = gethostbyname(Name);
	SOCKADDR_IN sock;
	sock.sin_family = AF_INET;
	sock.sin_port = htons(5555);
	memcpy(&sock.sin_addr.S_un.S_addr, pHostent->h_addr_list[0], pHostent->h_length);
	Result = bind(RecSocket, (PSOCKADDR)&sock, sizeof(sock));
	if (Result == SOCKET_ERROR)
	{
		printf("bind failed with error %d\n", WSAGetLastError());
		closesocket(RecSocket);
		return 0;
	}
	Result = WSAIoctl(RecSocket, SIO_RCVALL, &dwBufferInLen, sizeof(dwBufferInLen), &dwBufferLen,
		sizeof(dwBufferLen), &dwBytesReturned, NULL, NULL);
	if (Result == SOCKET_ERROR)
	{
		printf("WSAIoctl failed with error %d\n", WSAGetLastError());
		closesocket(RecSocket);
		return 0;
	}
	hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO bInfo;
	GetConsoleScreenBufferInfo(hCon, &bInfo);
	if (QueryPerformanceFrequency(&nFreq))
	{
		QueryPerformanceCounter(&StartTime);
		for (int p = PortStart; p <= PortEnd; p++)
		{
			SendTCPSYNPacket(p);
			while (true)
			{
				memset(RecvBuf, 0, sizeof(RecvBuf));
				Result = recv(RecSocket, RecvBuf, sizeof(RecvBuf), 0);
				if (Result == SOCKET_ERROR)
				{
					printf("recv failed with error %d\n", WSAGetLastError());
					closesocket(RecSocket);
					return 0;
				}
				Result = PacketAnalyzer(RecvBuf);
				if (Result == 0)
				{
					continue;
				}
				else
				{
					break;
				}
			}
			SetConsoleTextAttribute(hCon, 14);
			QueryPerformanceCounter(&EndTime);
		}
	}
	double fInterval = EndTime.QuadPart - StartTime.QuadPart;
	printf("Total Time:%fms\n", fInterval * 1000 / (double)nFreq.QuadPart);
	SetConsoleTextAttribute(hCon, bInfo.wAttributes);
	if (closesocket(RecSocket) == SOCKET_ERROR)
	{
		printf("closesocket failed with error %d\n", WSAGetLastError());
		return 0;
	}
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("WSACleanup failed with error %d\n", WSAGetLastError());
		return 0;
	}
	return 1;
}

USHORT checksum(USHORT *buffer, int size)
{
	unsigned long cksum = 0;
	while (size > 1)
	{
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}
	if (size)
	{
		cksum += *(UCHAR*)buffer;
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (USHORT)(~cksum);
}

int SendTCPSYNPacket(int Port)
{
	typedef struct IpHeader
	{
		u_char Version_HLen;
		u_char TOS;
		short Length;
		short Ident;
		short Flags_Offset;
		u_char TTL;
		u_char Protocol;
		short Checksum;
		unsigned int SourceAddr;
		unsigned int DestinationAddr;
	} Ip_Header;
	typedef struct PsdTcpHeader
	{
		unsigned long SourceAddr;
		unsigned long DestinationAddr;
		char Zero;
		char Protcol;
		unsigned short TcpLen;
	} PSD_Tcp_Header;
	typedef struct tcp_hdr
	{
		USHORT SrcPort;
		USHORT DstPort;
		unsigned int SequenceNum;
		unsigned int Acknowledgment;
		unsigned char HdrLen;
		unsigned char Flags;
		USHORT AdvertisedWindow;
		USHORT Checksum;
		USHORT UrgPtr;
	} Tcp_Header;
	struct in_addr localaddr;
	char HostName[255];
	struct hostent *Hostent;
	WSADATA wsaData;
	SOCKET SendSocket;
	SOCKADDR_IN addr_in;
	Ip_Header ipHeader;
	Tcp_Header tcpHeader;
	PSD_Tcp_Header psdHeader;
	char szSendBuf[100] =
	{
		0
	};
	BOOL flag;
	int nTimeOver;
	int Result;
	Result = WSAStartup(MAKEWORD(2, 1), &wsaData);
	if (Result == SOCKET_ERROR)
	{
		printf("WSAStartup failed with error %d\n", Result);
		return 0;
	}
	if ((SendSocket = WSASocket(AF_INET, SOCK_RAW, IPPROTO_RAW, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printf("WSASocket failed with error %d\n\n", WSAGetLastError());
		return false;
	}
	flag = true;
	if (setsockopt(SendSocket, IPPROTO_IP, IP_HDRINCL, (char*)&flag, sizeof(flag)) == SOCKET_ERROR)
	{
		printf("setsockopt failed with error %d\n\n", WSAGetLastError());
		return false;
	}
	nTimeOver = 1000;
	if (setsockopt(SendSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&nTimeOver, sizeof(nTimeOver)) == SOCKET_ERROR)
	{
		printf("setsockopt failed with error %d\n\n", WSAGetLastError());
		return false;
	}
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(1000);
	addr_in.sin_addr.S_un.S_addr = inet_addr(DestIpAddr);
	Result = gethostname(HostName, 255);
	if (Result == SOCKET_ERROR)
	{
		printf("gethostname failed with error %d\n", WSAGetLastError());
		return 0;
	}
	Hostent = (struct hostent*)malloc(sizeof(struct hostent));
	Hostent = gethostbyname(HostName);
	memcpy(&localaddr, Hostent->h_addr_list[0], Hostent->h_length);
	ipHeader.Version_HLen = (4 << 4 | sizeof(ipHeader) / sizeof(unsigned long));
	ipHeader.TOS = 0;
	ipHeader.Length = htons(sizeof(ipHeader) + sizeof(tcpHeader));
	ipHeader.Ident = 1;
	ipHeader.Flags_Offset = 0;
	ipHeader.TTL = 128;
	ipHeader.Protocol = IPPROTO_TCP;
	ipHeader.Checksum = 0;
	ipHeader.SourceAddr = localaddr.S_un.S_addr;
	ipHeader.DestinationAddr = inet_addr(DestIpAddr);
	tcpHeader.DstPort = htons(Port);
	tcpHeader.SrcPort = htons(6666);
	tcpHeader.SequenceNum = htonl(0);
	tcpHeader.Acknowledgment = 0;
	tcpHeader.HdrLen = (sizeof(tcpHeader) / 4 << 4 | 0);
	tcpHeader.Flags = 2;
	tcpHeader.AdvertisedWindow = htons(512);
	tcpHeader.UrgPtr = 0;
	tcpHeader.Checksum = 0;
	psdHeader.SourceAddr = ipHeader.SourceAddr;
	psdHeader.DestinationAddr = ipHeader.DestinationAddr;
	psdHeader.Zero = 0;
	psdHeader.Protcol = IPPROTO_TCP;
	psdHeader.TcpLen = htons(sizeof(tcpHeader));
	memcpy(szSendBuf, &psdHeader, sizeof(psdHeader));
	memcpy(szSendBuf + sizeof(psdHeader), &tcpHeader, sizeof(tcpHeader));
	tcpHeader.Checksum = checksum((USHORT*)szSendBuf, sizeof(psdHeader) + sizeof(tcpHeader));
	memcpy(szSendBuf, &ipHeader, sizeof(ipHeader));
	memcpy(szSendBuf + sizeof(ipHeader), &tcpHeader, sizeof(tcpHeader));
	memset(szSendBuf + sizeof(ipHeader) + sizeof(tcpHeader), 0, 4);
	ipHeader.Checksum = checksum((USHORT*)szSendBuf, sizeof(ipHeader) + sizeof(tcpHeader));
	memcpy(szSendBuf, &ipHeader, sizeof(ipHeader));
	Result = sendto(SendSocket, szSendBuf, sizeof(ipHeader) + sizeof(tcpHeader), 0, (struct sockaddr*) &addr_in, sizeof(addr_in));
	if (Result == SOCKET_ERROR)
	{
		printf("gethostname failed with error %d\n", WSAGetLastError());
		return 0;
	} if (closesocket(SendSocket) == SOCKET_ERROR)
	{
		printf("closesocket failed with error %d\n", WSAGetLastError());
		return 0;
	}
	if (WSACleanup() == SOCKET_ERROR)
	{
		printf("WSACleanup failed with error %d\n", WSAGetLastError());
		return 0;
	}
	return 1;
}

int PacketAnalyzer(char *PacketBuffer)
{
	Ip_Header *pIpheader;
	int iProtocol, iTTL;
	char szSourceIP[16], szDestIP[16];
	SOCKADDR_IN saSource, saDest;
	pIpheader = (Ip_Header*)PacketBuffer;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO bInfo;
	GetConsoleScreenBufferInfo(hCon, &bInfo);
	iProtocol = pIpheader->Protocol;
	saSource.sin_addr.s_addr = pIpheader->SourceAddr;
	::strcpy_s(szSourceIP, inet_ntoa(saSource.sin_addr));
	saDest.sin_addr.s_addr = pIpheader->DestinationAddr;
	::strcpy_s(szDestIP, inet_ntoa(saDest.sin_addr));
	iTTL = pIpheader->TTL;
	int iIphLen = sizeof(unsigned long)*(pIpheader->Version_HLen & 0x0f);
	if (iProtocol == IPPROTO_TCP)
	{
		Tcp_Header *pTcpHeader;
		pTcpHeader = (Tcp_Header*)(PacketBuffer + iIphLen);
		if (pIpheader->SourceAddr == inet_addr(DestIpAddr))
		{
			if (pTcpHeader->Flags &RST)
			{
				SetConsoleTextAttribute(hCon, 10);
				printf("Port %d Close\n", ntohs(pTcpHeader->SrcPort));
				return 1;
			}
			else if ((pTcpHeader->Flags &SYN) && (pTcpHeader->Flags &ACK))
			{
				SetConsoleTextAttribute(hCon, 14);
				printf("Port %d Open\n", ntohs(pTcpHeader->SrcPort));
				return 1;
			}
		}
	}
	SetConsoleTextAttribute(hCon, bInfo.wAttributes);
	return 0;
}
