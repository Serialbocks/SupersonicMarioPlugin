// TcpClient.cpp
// Author: Serialbocks

#include "Networking.h"

TcpClient* instance = nullptr;

TcpClient::TcpClient()
{
	instance = this;
}


void clientThread()
{
	if (instance == nullptr)
	{
		return;
	}

	// Initialize Winsock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		std::stringstream errMsg;
		errMsg << "Can't start Winsock, Err#" << wsResult;
		BM_LOG(errMsg.str());
		return;
	}

	// Create socket
	instance->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (instance->sock == INVALID_SOCKET)
	{
		std::stringstream errMsg;
		errMsg << "Can't create socket, Err#" << WSAGetLastError();
		BM_LOG(errMsg.str());
		instance->sock = INVALID_SOCKET;
		WSACleanup();
		return;
	}

	int flags = 1;
	setsockopt(instance->sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&flags, sizeof(flags));

	// Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(instance->serverPort);
	inet_pton(AF_INET, instance->serverIp.c_str(), &hint.sin_addr);

	// Connect to server
	int connResult = connect(instance->sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		std::stringstream errMsg;
		errMsg << "Can't connect to server, Err#" << WSAGetLastError();
		BM_LOG(errMsg.str());
		closesocket(instance->sock);
		instance->sock = INVALID_SOCKET;
		WSACleanup();
		return;
	}

	BM_LOG("Connected to server");
	char buf[TCP_BUF_SIZE];

	while (true)
	{
		ZeroMemory(buf, TCP_BUF_SIZE);
		int bytesReceived = recv(instance->sock, buf, TCP_BUF_SIZE, 0);
		if (bytesReceived <= sizeof(int))
		{
			break;
		}
		if (instance != nullptr && instance->msgReceivedClbk != nullptr)
		{
			instance->msgReceivedClbk(buf, bytesReceived);
		}
	}

	closesocket(instance->sock);
	instance->sock = INVALID_SOCKET;
	WSACleanup();
	BM_LOG("Disconnected from server");
}

void TcpClient::ConnectToServer(std::string inIpAddress, int inPort)
{
	if (sock != INVALID_SOCKET)
	{
		DisconnectFromServer();
	}
	serverIp = inIpAddress;
	serverPort = inPort;

	std::thread clientThrd(clientThread);
	clientThrd.detach();
}

void TcpClient::DisconnectFromServer()
{
	if (sock == INVALID_SOCKET)
	{
		BM_LOG("Tried to disconnect from server, but we're not connected");
		return;
	}
	closesocket(sock);
	sock = INVALID_SOCKET;
	WSACleanup();
}

void TcpClient::RegisterMessageCallback(void (*clbk)(char* buf, int len))
{
	msgReceivedClbk = clbk;
}

void TcpClient::SendBytes(char* buf, int len)
{
	if (sock == INVALID_SOCKET)
	{
		return;
	}
	send(sock, buf, len, 0);
}