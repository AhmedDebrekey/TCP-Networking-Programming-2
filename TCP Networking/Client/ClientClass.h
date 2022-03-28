#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <iostream>
#include <random>

class Client
{
public:
	Client();
	~Client();

private:
	/*
	* Initializing the Client.
	* 
	* Initializing all main variables.
	* It sets everything up before connecting to the server.
	* 
	* returns 1 if there was a problem.
	*/
	int Init();

	/*
	* Attempting a connection on the server.
	* 
	* returns 1 if anything was unsuccessful.
	*/
	int ConnectToServer();

	/*
	* Accepting messages and sending messages from and to the server on two different threads.
	* excluding the main thread
	*/
	void ThreadSendAndRecv();

	/*
	* Runs in a forever loop waiting for the server to send a packet.
	* return values means nothing since it is a different on their own, if the function returns smth then the thread dies.
	*/
	int RecvMessages(SOCKET clientSocket);

	/*
	* Sends messages to the Server
	* same as RecvMessages return values means nothing since it's it's own thread.
	*/
	int SendMessages(SOCKET clientSocket);

	void encrypt_message(std::string& str);

	void decrypt_message(std::string& str);

private:

	/*
	* This varaiable is used for error checking.
	* It is also important for send() and recv() to write their length.
	*/
	int iResult;


	/*
	* The addrinfo structure is used by the getaddrinfo function to hold host address information.
	*/
	struct addrinfo* result = NULL,
				   * ptr	= NULL,
					 hints;

	/*
	* Socket that the client uses to connect to the server.
	*/
	SOCKET ConnectSocket;
};

