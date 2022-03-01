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

	// Create listening socket
	instance->listening = socket(AF_INET, SOCK_DGRAM, 0);
	if (instance->listening == INVALID_SOCKET)
	{
		BM_LOG("Can't create a socket!");
		WSACleanup();
		return;
	}

	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(instance->port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	int bindResult = bind(instance->listening, (sockaddr*)&hint, sizeof(hint));
	if(bindResult == SOCKET_ERROR)
	{
		std::stringstream errStrStream;
		errStrStream << "After bind() Error#" << WSAGetLastError();
		BM_LOG(errStrStream.str());
	}

	// Tell winsock the socket is for listening
	int listenResult = listen(instance->listening, SOMAXCONN);

	if (listenResult != 0)
	{
		std::stringstream errStrStream;
		errStrStream << "Couldn't Start Listening Thread. Error#" << WSAGetLastError() << " listenResult: " << listenResult;
		BM_LOG(errStrStream.str());
	}

	instance->masterSetSema.acquire();
	FD_ZERO(&instance->master);
	FD_SET(instance->listening, &instance->master);
	instance->masterSetSema.release();

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
	instance->serverExitSocket = accept(instance->listening, nullptr, nullptr);
	if (instance->serverExitSocket == INVALID_SOCKET || instance->stopServerSocket == INVALID_SOCKET)
	{
		std::stringstream errStrStream;
		errStrStream << "Can't accept close sock to server. Error#" << WSAGetLastError();
		BM_LOG(errStrStream.str());
		closesocket(instance->stopServerSocket);
		instance->stopServerSocket = INVALID_SOCKET;
		WSACleanup();
		return;
	}
	instance->masterSetSema.acquire();
	FD_SET(instance->serverExitSocket, &instance->master);
	instance->masterSetSema.release();

	BM_LOG("Server started and listening");

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
			if (sock == instance->listening)
			{
				// Accept a new connection
				SOCKET client = accept(instance->listening, nullptr, nullptr);

				if (client == INVALID_SOCKET)
				{
					instance->stopServerSocket = INVALID_SOCKET;
					break;
				}

				// Add new connection to list of connected clients
				instance->masterSetSema.acquire();
				FD_SET(client, &instance->master);
				instance->masterSetSema.release();
			}
			else if (sock == instance->serverExitSocket)
			{
				instance->stopServerSocket = INVALID_SOCKET;
				break;
			}
			else
			{
				ZeroMemory(buf, TCP_BUF_SIZE);

				// Receive message
				int bytesIn = recv(sock, buf + sizeof(int), TCP_BUF_SIZE - sizeof(int), 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					closesocket(sock);
					instance->masterSetSema.acquire();
					FD_CLR(sock, &instance->master);
					instance->masterSetSema.release();
				}
				else
				{
					if (instance->playerIdMap.count(sock) == 0)
					{
						instance->playerIdMap[sock] = instance->nextPlayerId++;
					}
					int playerId = instance->playerIdMap[sock];

					*((int*)buf) = playerId;
					
					// Send message to other clients, and definitely NOT the listening socket
					for (int k = 0; k < instance->master.fd_count; k++)
					{
						SOCKET outSock = instance->master.fd_array[k];
						if (outSock != instance->listening && outSock != sock && outSock != instance->serverExitSocket)
						{
							send(outSock, buf + sizeof(int), bytesIn, 0);
						}
					}

					// Handle the message ourselves too if a callback is set
					if (instance != nullptr && instance->msgReceivedClbk != nullptr)
					{
						instance->msgReceivedClbk(buf + sizeof(int), bytesIn, playerId);
					}


				}

			}
		}
	}

	// Close all open sockets
	closesocket(instance->listening);
	instance->stopServerSocket = INVALID_SOCKET;

	// Cleanup winsock
	WSACleanup();
	BM_LOG("Server stopped");
}

void TcpServer::StartServer(int inPort)
{
	if (stopServerSocket != INVALID_SOCKET)
	{
		BM_LOG("Tried to create server, but one is already running!");
		return;
	}

	playerIdMap.clear();
	nextPlayerId = 1;
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

	playerIdMap.clear();
	nextPlayerId = 1;
	std::string emptyStr = "";
	int sendResult = send(stopServerSocket, emptyStr.c_str(), 1, 0);
}

void TcpServer::RegisterMessageCallback(void (*clbk)(char* buf, int len, int playerId))
{
	msgReceivedClbk = clbk;
}

void TcpServer::SendBytes(char* buf, int len)
{
	if (stopServerSocket == INVALID_SOCKET)
	{
		return;
	}
	masterSetSema.acquire();
	fd_set setCopy = master;
	masterSetSema.release();

	int outBufSize = len + sizeof(int);
	char* outBuf = (char*)malloc(len + sizeof(int));
	ZeroMemory(outBuf, outBufSize);
	memcpy(outBuf + sizeof(int), buf, len);

	for (int k = 0; k < instance->master.fd_count; k++)
	{
		SOCKET outSock = instance->master.fd_array[k];
		if (outSock != listening && outSock != serverExitSocket)
		{
			send(outSock, outBuf, outBufSize, 0);
		}
	}

	free(outBuf);
}