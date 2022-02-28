#include "Networking.h"

#define BUF_SIZE 4096

#define MSG_TYPE_ID 0x01
#define MSG_TYPE_DATA 0x02

TcpServer* instance = nullptr;

TcpServer::TcpServer()
{
	instance = this;
}


int port = 7778;
WSADATA wsData;
fd_set master;
SOCKET stopServerSocket = INVALID_SOCKET;
void serverThread()
{
	WORD ver = MAKEWORD(2, 2);

	// Start winsock
	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		BM_LOG("Can't initialize winsock!");
		return;
	}

	// Bind the ip address and port to a socket
	struct addrinfo* result = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	std::stringstream portStr;
	portStr << "" << port;
	int iResult = getaddrinfo(NULL, portStr.str().c_str(), &hints, &result);
	if (iResult != 0) {
		std::stringstream errStrStream;
		errStrStream << "getaddrinfo failed with error: " << iResult;
		BM_LOG(errStrStream.str());
		WSACleanup();
		return;
	}

	// Create listening socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		BM_LOG("Can't create a socket!");
		WSACleanup();
		return;
	}

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	int bindResult = bind(listening, (sockaddr*)&hint, sizeof(hint));
	if(bindResult == SOCKET_ERROR)
	{
		std::stringstream errStrStream;
		errStrStream << "After bind() Error#" << WSAGetLastError();
		BM_LOG(errStrStream.str());
	}

	// Tell winsock the socket is for listening
	int listenResult = listen(listening, SOMAXCONN);

	if (listenResult != 0)
	{
		std::stringstream errStrStream;
		errStrStream << "Couldn't Start Listening Thread. Error#" << WSAGetLastError() << " listenResult: " << listenResult;
		BM_LOG(errStrStream.str());
	}

	FD_ZERO(&master);

	FD_SET(listening, &master);

	// Create a socket used solely to notify we should close the server
	sockaddr_in closeHint;
	closeHint.sin_family = AF_INET;
	closeHint.sin_port = htons(port);
	std::string localAddress = "127.0.0.1";
	inet_pton(AF_INET, localAddress.c_str(), &closeHint.sin_addr);
	stopServerSocket = socket(AF_INET, SOCK_STREAM, 0);
	int connResult = connect(stopServerSocket, (sockaddr*)&closeHint, sizeof(closeHint));
	if (connResult == SOCKET_ERROR)
	{
		std::stringstream errStrStream;
		errStrStream << "Can't connect close sock to server. Error#" << WSAGetLastError();
		BM_LOG(errStrStream.str());
		closesocket(stopServerSocket);
		stopServerSocket = INVALID_SOCKET;
		WSACleanup();
	}

	// Accept the exit socket
	SOCKET serverExitSocket = accept(listening, nullptr, nullptr);
	if (serverExitSocket == INVALID_SOCKET || stopServerSocket == INVALID_SOCKET)
	{
		std::stringstream errStrStream;
		errStrStream << "Can't accept close sock to server. Error#" << WSAGetLastError();
		BM_LOG(errStrStream.str());
		closesocket(stopServerSocket);
		stopServerSocket = INVALID_SOCKET;
		WSACleanup();
		return;
	}
	FD_SET(serverExitSocket, &master);

	// Main server loop
	char buf[BUF_SIZE];
	while (true)
	{
		if (stopServerSocket == INVALID_SOCKET)
		{
			break;
		}

		fd_set setCopy = master;
		int socketCount = select(0, &setCopy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socketCount; i++)
		{
			SOCKET sock = setCopy.fd_array[i];
			if (sock == listening)
			{
				// Accept a new connection
				SOCKET client = accept(listening, nullptr, nullptr);

				if (client == INVALID_SOCKET)
				{
					stopServerSocket = INVALID_SOCKET;
					break;
				}

				// Add new connection to list of connected clients
				FD_SET(client, &master);
			}
			else if (sock == serverExitSocket)
			{
				stopServerSocket = INVALID_SOCKET;
				break;
			}
			else
			{
				ZeroMemory(buf, BUF_SIZE);

				// Receive message
				int bytesIn = recv(sock, buf, BUF_SIZE, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else if(bytesIn > 1) // We use the first byte as message type
				{
					// Send message to other clients, and definitely NOT the listening socket
					for (int k = 0; k < master.fd_count; k++)
					{
						SOCKET outSock = master.fd_array[k];
						if (outSock != listening && outSock != sock)
						{
							send(outSock, buf, bytesIn, 0);
						}
					}
					
					if (buf[0] == MSG_TYPE_ID)
					{
						if (instance->playerSockMap.count(sock) == 0)
						{
							instance->playerSockMap[sock] = std::string(buf + 1, 0, bytesIn - 1);
						}
					}
					else if (buf[0] == MSG_TYPE_DATA)
					{
						// Handle the message ourselves too if a callback is set
						if (instance != nullptr && instance->msgReceivedClbk != nullptr)
						{
							std::string playerName = "";
							if (instance->playerSockMap.count(sock) > 0)
							{
								playerName = instance->playerSockMap[sock];
							}
							instance->msgReceivedClbk(buf + 1, bytesIn - 1, playerName);
						}
					}


				}

			}
		}
	}

	// Close all open sockets
	closesocket(listening);
	stopServerSocket = INVALID_SOCKET;

	// Cleanup winsock
	WSACleanup();
}

static volatile int disableOptimize = 0;
void TcpServer::StartServer(int inPort)
{
	disableOptimize = 1;
	if (stopServerSocket != INVALID_SOCKET)
	{
		BM_LOG("Tried to create server, but one is already running!");
		return;
	}
	playerSockMap.clear();

	port = inPort;
	std::thread servThread(serverThread);
	servThread.detach();
}

void TcpServer::StopServer()
{
	disableOptimize = 2;
	if (stopServerSocket == INVALID_SOCKET)
	{
		BM_LOG("Tried to close server, but none are running!");
		return;
	}
	playerSockMap.clear();

	std::string emptyStr = "";
	int sendResult = send(stopServerSocket, emptyStr.c_str(), 1, 0);
}

void TcpServer::RegisterMessageCallback(void (*clbk)(char* buf, int len, std::string playerName))
{
	msgReceivedClbk = clbk;
}