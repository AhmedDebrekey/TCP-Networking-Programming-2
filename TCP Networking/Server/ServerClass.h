#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>


class Server
{
private:
	/*
	*Client struct
	* Contains: Client's Socket, Client's ID
	* Is used to handle client identification paired with it's vector.
	*/
	struct Client
	{
		SOCKET clientSocket;
		uint8_t clientId;
	};

public:
	Server();
	~Server();

	int RecvMessages(Client client);
	int SendMessages(Client toClient, Client fromClient, std::string& msg);

	void Init();
	void RunServer();
	

private:

	/*
	* This varaiable is used for error checking.
	* It is also important for send() and recv() to write their length.
	*/
	int iResult;

	std::vector<std::thread> m_VThreads;
	std::vector<Client> m_VClients;

	/*
	*Listening Socket
	* Is used to listen for client connectoins,
	*/
	SOCKET m_ListenSocket;

	/*
	*Client Socket
	* It is the socket used in the main thread,	that a client can connect to.
	* It is passed over to other thread to handle this specific client.
	* It's contents change every time a new client is accepted.
	*/
	SOCKET m_ClientSocket;
};

