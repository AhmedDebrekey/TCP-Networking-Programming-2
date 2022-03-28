#include "ServerClass.h"
#include <LOG.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27010"

Server::Server()
	:	/*Set both Listen and Client sockets to INVALID for error checking purposes*/
	m_ListenSocket(INVALID_SOCKET),		
	m_ClientSocket(INVALID_SOCKET)
{
	Init();
	RunServer();
}

Server::~Server()
{
	// Shutdown the send half of the connection since no more data will be sent
	iResult = shutdown(m_ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		Debreky::LOG("Shuting down of the send half, failed: " + std::to_string(WSAGetLastError()), Debreky::Error);
		closesocket(m_ClientSocket);
		WSACleanup();
	}

	// Shutdown the receive half of the connection since no more data will be received
	iResult = shutdown(m_ClientSocket, SD_RECEIVE);
	if (iResult == SOCKET_ERROR) {
		Debreky::LOG("Shuting down of the receive half, failed: " + std::to_string(WSAGetLastError()), Debreky::Error);
		closesocket(m_ClientSocket);
		WSACleanup();
	}

	// cleanup
	closesocket(m_ClientSocket);
	WSACleanup();
}

void Server::Init()
{
	/*
	* The WSADATA struct contains information about the Windows Sockets implementation.
	*/
	WSADATA wsaData;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		Debreky::LOG("WSAStartup failed: " + std::to_string(iResult), Debreky::Error);
		return;
	}

	/*
	* The addrinfo structure is used by the getaddrinfo function to hold host address information.
	*/
	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		Debreky::LOG("getaddrinfo failed: " + std::to_string(iResult), Debreky::Error);
		WSACleanup();
	}

	/*
	* The socket function creates a socket that is bound to a specific transport service provider.
	* Create a SOCKET for the server to listen for client connections.
	*/
	m_ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (m_ListenSocket == INVALID_SOCKET) {
		Debreky::LOG("Error at socket(): " + std::to_string(WSAGetLastError()), Debreky::Error);
		freeaddrinfo(result);
		WSACleanup();
	}

	/*
	* Setup the TCP listening socket
	* The bind function associates a local address with a socket.
	*/
	iResult = bind(m_ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		Debreky::LOG("bind failed with error: " + std::to_string(WSAGetLastError()), Debreky::Error);
		freeaddrinfo(result);
		closesocket(m_ListenSocket);
		WSACleanup();
	}

	/*
	*Once the bind function is called, the address information returned by the getaddrinfo function is no longer needed.
	*The freeaddrinfo function is called to free the memory allocated by the getaddrinfo function for this address information.
	*/
	freeaddrinfo(result);

	/*
	*The listen function places a socket in a state in which it is listening for an incoming connection.
	*/
	if (listen(m_ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		Debreky::LOG("Listen failed with error: " + std::to_string(WSAGetLastError()), Debreky::Error);
		closesocket(m_ListenSocket);
		WSACleanup();
	}
}

void Server::RunServer()
{
	Debreky::LOG("Server is up and running \n\n", Debreky::Info);
	while (true)
	{
		Debreky::LOG("Listening for a Client ...\n\n", Debreky::Info);
		/*
		* Accept a client socket
		*The accept function permits an incoming connection attempt on a socket.
		*/
		m_ClientSocket = accept(m_ListenSocket, NULL, NULL);
		if (m_ClientSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(m_ListenSocket);
			WSACleanup();
		}

		//Create an instance of a struct that hold the contents of the client socket
		Client client = { m_ClientSocket, uint8_t(m_ClientSocket) };
		
		Debreky::LOG("Client connected [" + std::to_string(uint32_t(client.clientId)) + "]\n", Debreky::Warning);

		//Adding the client and thread to the vectors to be accessed later
		m_VClients.push_back(client);

		m_VThreads.push_back(std::thread([this](Client client) { RecvMessages(client); }, client));

		std::cout << std::endl;
	}
}

int Server::RecvMessages(Client client)
{
	char recvbuf[DEFAULT_BUFLEN]; //Initalizing a char buffer to write to
	int recvbuflen = DEFAULT_BUFLEN;

	auto clientSocket = client.clientSocket;
	auto clientId = client.clientId;

	do
	{
		/*
		* The recv function receives data from a connected socket or a bound connectionless socket.
		* recvbuf is written to by this function the data recevied.
		* iResult is the length of the data that was recevied.
		*/
		iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			Debreky::LOG("Bytes sent: ", Debreky::Info); std::cout << iResult << std::endl;
			Debreky::LOG("Message from [", Debreky::Info); std::cout << uint32_t(clientId) << "]: ";
			for (int i = 0; i < iResult; i++) //Can't print the data normally because it's 512 bytes long, otherwise garabage will be printed
			{
				std::cout << recvbuf[i];
			}
			std::cout << std::endl;

			std::string msg;
			for (int i = 0; i < iResult; i++)
				msg.insert(msg.end(), recvbuf[i]);

			for (uint32_t i = 0; i < m_VClients.size(); i++)
			{
				if (m_VClients[i].clientId != client.clientId)
				{
					SendMessages(m_VClients[i], client, msg); //Send messages to a client.
				}
			}
		}
		else if (iResult == 0)
			Debreky::LOG("Connection closing ...", Debreky::Info);
		else if (WSAGetLastError() == WSAECONNRESET)
		{
			closesocket(clientSocket);
			Debreky::LOG("Connection closed! ["  + std::to_string(uint32_t(clientId))  +"]\n\n", Debreky::Warning);
			try
			{
				for (size_t i = 0; i < m_VClients.size(); i++)
				{
					if (m_VClients[i].clientId == clientId)
					{
						m_VClients.erase(m_VClients.begin() + i);
						//Update client ID . . .
						return 1;
					}
				}
			}
			catch (const std::exception&)
			{
				Debreky::LOG("Exception thrown!", Debreky::Error);
			}
		}
		else
		{
			Debreky::LOG("recv failed: " + std::to_string(WSAGetLastError()), Debreky::Error);
			closesocket(clientSocket);
			ExitThread(0);
			return 1;
		}
	} while (iResult > 0);

	return 1;
}

int Server::SendMessages(Client toClient, Client fromClient, std::string& msg)
{
	auto clientSocket = toClient.clientSocket;
	auto fromClientId = fromClient.clientId;

	if (msg[0] != '[')
	{
		std::string id = "[" + std::to_string(fromClientId) + "]";
		msg.insert(0, id);
	}

	/*
	* The send function sends data on a connected socket.
	* Message (msg) should be handled as a char buffer and not a string.
	*/
	iResult = send(clientSocket, msg.c_str(), (int)strlen(msg.c_str()), 0);
	if (iResult == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAENOTSOCK) //Checking if the connection is still avaliable
		{
			Debreky::LOG("Message attempted on sokcet [" + std::to_string(uint32_t(toClient.clientId)) + "]\n", Debreky::Warning);
			return 0;
		}
		if (WSAGetLastError() == WSAECONNRESET)
			return 1;
		else
		{
			Debreky::LOG("send failed: " + std::to_string(WSAGetLastError()), Debreky::Error);
			closesocket(clientSocket);
			WSACleanup();
			return 1;
		}
	}
	Debreky::LOG("Sending message to [" + std::to_string(uint32_t(toClient.clientId)) + "]\n", Debreky::Info);
	Debreky::LOG("Bytes sent: ", Debreky::Info); std::cout << iResult << std::endl;
	Debreky::LOG("Message sent: " + msg + "\n\n", Debreky::Info);
	return 1;
}
