#include "ClientClass.h"
#include <LOG.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27010"

Client::Client()
	:ConnectSocket(INVALID_SOCKET) /*Set to invalid for error checking*/
{
	if (Init() != 1) 
	{
		if (ConnectToServer() != 1) 
		{
			ThreadSendAndRecv(); 
		}
		else
			std::cout << "Connecting to server failed... " << std::endl;
	}
	else
		std::cout << "Initilization of client failed..." << std::endl;
}

Client::~Client()
{
	// shutdown the connection for sending since no more data will be sent
	// the client can still use the ConnectSocket for receiving data
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		Debreky::LOG("Shutdown failed: " + std::to_string(WSAGetLastError()), Debreky::Error);
		closesocket(ConnectSocket);
		WSACleanup();
	}

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();
}

int Client::Init()
{
	/*
	* The WSADATA struct contains information about the Windows Sockets implementation.
	*/
	WSADATA wsaData;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) 
	{
		Debreky::LOG("WSAStartup failed: " + std::to_string(iResult), Debreky::Error);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo("??.???.?.??", DEFAULT_PORT, &hints, &result); //<- ADD YOUR IP ADRESS HERE
	if (iResult != 0) 
	{
		Debreky::LOG("getaddrinfo failed: " + std::to_string(iResult), Debreky::Error);
		WSACleanup();
		return 1;
	}
}

int Client::ConnectToServer()
{
	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr = result;

	/*
	* The socket function creates a socket that is bound to a specific transport service provider.
	* Create a SOCKET for the server to listen for client connections.
	*/
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (ConnectSocket == INVALID_SOCKET) 
	{
		Debreky::LOG("Error at socket(): " + std::to_string(WSAGetLastError()), Debreky::Error);
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	/*
	* The connect function establishes a connection to a specified socket.
	* Connect to server.
	*/
	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		Debreky::LOG("Connection to server failed ...\n", Debreky::Error);
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	/* Should really try the next address returned by getaddrinfo
	* if the connect call failed
	* But for this simple example we just free the resources
	* returned by getaddrinfo and print an error message
	*/

	freeaddrinfo(result);
	return 0;
}

void Client::ThreadSendAndRecv()
{
	/*
	* Creating two threads using lamdas because really this is the only way I got it to work.
	*/
	std::thread outputThread = std::thread([this](SOCKET clientSocket) { RecvMessages(ConnectSocket); }, ConnectSocket);
	std::thread inputThread = std::thread([this](SOCKET clientSocket) { SendMessages(ConnectSocket); }, ConnectSocket);
	outputThread.join(); /*Wait for output thread to finish*/
	inputThread.join();  /*Wait for input thread to finish*/
}

int Client::RecvMessages(SOCKET clientSocket)
{
	int recvbuflen = DEFAULT_BUFLEN; //Initalizing a char buffer to write to
	char recvbuf[DEFAULT_BUFLEN];

	do
	{
		/*
		* The recv function receives data from a connected socket or a bound connectionless socket.
		* recvbuf is written to by this function the data recevied.
		* iResult is the length of the data that was recevied.
		*/
		memset(recvbuf, 0, recvbuflen);
		int	iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			std::string msg = recvbuf;

			int pos = msg.find_first_of(']');
			std::string encryptedMessage = msg.substr(pos + 1), id = msg.substr(0, pos);

			decrypt_message(encryptedMessage);

			std::string decryptedMessage = id + "]" + encryptedMessage;
			std::cout << decryptedMessage << std::endl << std::endl;
		}
		else if (iResult == 0)
			Debreky::LOG("Connection closed\n", Debreky::Warning);
		else if (WSAGetLastError() == 10054) //Checking if server has disconnected
		{
			Debreky::LOG("Server disconnected!", Debreky::Error);
			return 1;
		}
		else
			Debreky::LOG("recv failed: " + std::to_string(WSAGetLastError()) + "\n", Debreky::Error);
	} while (true);
	return 0;
}

int Client::SendMessages(SOCKET clientSocket)
{
	std::string sendbuf;
	Debreky::LOG("Connected to server, You can start chatting!\n", Debreky::Info);
	do
	{
		std::getline(std::cin, sendbuf); std::cout << std::endl;

		encrypt_message(sendbuf);

		/*
		* The send function sends data on a connected socket.
		* Message (sendbuf) should be handled as a char buffer and not a string.
		*/
		int iResult = send(clientSocket, sendbuf.c_str(), (int)strlen(sendbuf.c_str()), 0);
		if (iResult == SOCKET_ERROR) {
			Debreky::LOG("recv failed: " + std::to_string(WSAGetLastError()) + "\n", Debreky::Error);
			closesocket(clientSocket);
			WSACleanup();
			if (WSAGetLastError() == WSAECONNRESET)
				Debreky::LOG("Error has occured!", Debreky::Error);
			return 1;
		}
	} while (sendbuf != "0");
	return 0;
}

void Client::encrypt_message(std::string& str)
{
	std::random_device r;
	std::default_random_engine e1(r());
	std::uniform_int_distribution<int> gen(0, 255);

	char k1 = static_cast<char>(gen(e1));
	char k2 = static_cast<char>(gen(e1));
	for (int i = 0; i < str.size(); i++)
	{
		char k = (i % 2) ? k1 : k2;
		str[i] ^= k;
	}
	str.insert(str.begin(), k1);
	str.insert(str.end(), k2);
}

void Client::decrypt_message(std::string& str)
{
	char k1 = str[0];
	char k2 = str[str.size() - 1];

	str.erase(str.begin());
	str.erase(str.end() - 1);

	for (int i = 0; i < str.size(); i++)
	{
		char k = (i % 2) ? k1 : k2;
		str[i] ^= k;
	}
}
