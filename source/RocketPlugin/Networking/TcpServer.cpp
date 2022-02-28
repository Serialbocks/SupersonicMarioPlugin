#include "Networking.h"

#define MSG_TYPE_ID 0x01
#define MSG_TYPE_DATA 0x02

TcpServer* instance = nullptr;

TcpServer::TcpServer()
{
	instance = this;
}

void serverThread()
{
	if (instance == nullptr)
	{
		return;
	}

	WSADATA wsData;
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
	portStr << "" << instance->port;
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
	hint.sin_port = htons(instance->port);
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

	FD_ZERO(&instance->master);

	FD_SET(listening, &instance->master);

	// Create a socket used solely to notify we should close the server
	sockaddr_in closeHint;
	closeHint.sin_family = AF_INET;
	closeHint.sin_port = htons(instance->port);
	std::string localAddress = "127.0.0.1";
	inet_pton(AF_INET, localAddress.c_str(), &closeHint.sin_addr);
	instance->stopServerSocket = socket(AF_INET, SOCK_STREAM, 0);
	int connResult = connect(instance->stopServerSocket, (sockaddr*)&closeHint, sizeof(closeHint));
	if (connResult == SOCKET_ERROR)
	{
		std::stringstream errStrStream;
		errStrStream << "Can't connect close sock to server. Error#" << WSAGetLastError();
		BM_LOG(errStrStream.str());
		closesocket(instance->stopServerSocket);
		instance->stopServerSocket = INVALID_SOCKET;
		WSACleanup();
	}

	// Accept the exit socket
	SOCKET serverExitSocket = accept(listening, nullptr, nullptr);
	if (serverExitSocket == INVALID_SOCKET || instance->stopServerSocket == INVALID_SOCKET)
	{
		std::stringstream errStrStream;
		errStrStream << "Can't accept close sock to server. Error#" << WSAGetLastError();
		BM_LOG(errStrStream.str());
		closesocket(instance->stopServerSocket);
		instance->stopServerSocket = INVALID_SOCKET;
		WSACleanup();
		return;
	}
	FD_SET(serverExitSocket, &instance->master);

	// Main server loop
	char buf[TCP_BUF_SIZE];
	while (true)
	{
		if (instance->stopServerSocket == INVALID_SOCKET)
		{
			break;
		}

		fd_set setCopy = instance->master;
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
					instance->stopServerSocket = INVALID_SOCKET;
					break;
				}

				// Add new connection to list of connected clients
				FD_SET(client, &instance->master);
			}
			else if (sock == serverExitSocket)
			{
				instance->stopServerSocket = INVALID_SOCKET;
				break;
			}
			else
			{
				ZeroMemory(buf, TCP_BUF_SIZE);

				// Receive message
				int bytesIn = recv(sock, buf, TCP_BUF_SIZE, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					closesocket(sock);
					FD_CLR(sock, &instance->master);
				}
				else if(bytesIn > 1) // We use the first byte as message type
				{
					// Send message to other clients, and definitely NOT the listening socket
					for (int k = 0; k < instance->master.fd_count; k++)
					{
						SOCKET outSock = instance->master.fd_array[k];
						if (outSock != listening && outSock != sock)
						{
							send(outSock, buf, bytesIn, 0);
						}
					}

					// Handle the message ourselves too if a callback is set
					if (instance != nullptr && instance->msgReceivedClbk != nullptr)
					{
						instance->msgReceivedClbk(buf, bytesIn);
					}


				}

			}
		}
	}

	// Close all open sockets
	closesocket(listening);
	instance->stopServerSocket = INVALID_SOCKET;

	// Cleanup winsock
	WSACleanup();
}

void TcpServer::StartServer(int inPort)
{
	if (stopServerSocket != INVALID_SOCKET)
	{
		BM_LOG("Tried to create server, but one is already running!");
		return;
	}

	port = inPort;
	std::thread servThread(serverThread);
	servThread.detach();
}

void TcpServer::StopServer()
{
	if (stopServerSocket == INVALID_SOCKET)
	{
		BM_LOG("Tried to close server, but none are running!");
		return;
	}

	std::string emptyStr = "";
	int sendResult = send(stopServerSocket, emptyStr.c_str(), 1, 0);
}

void TcpServer::RegisterMessageCallback(void (*clbk)(char* buf, int len))
{
	msgReceivedClbk = clbk;
}