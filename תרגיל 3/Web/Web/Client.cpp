#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h> 
#include <string.h>
#include <iomanip>

const int PORT = 8080;
const int MESSAGE_SIZE = 1024;
const char* s_OPTIONS = "OPTIONS";
const char* s_GET = "GET";
const char* s_HEAD = "HEAD";
const char* s_PUT = "PUT";
const char* s_TRACE = "TRACE";
const char* s_DELETE = "DELETE";
const char* s_EXIT = "Exit";
/*
PAY ATTENTION: you have to change the following paths according to your folder and files.
*/
const char* s_MESSAGE_HEADER = " /MyFolder/page.html HTTP/1.1\nHost: 127.0.0.1\nAccept: text/html\r\n";
const char* s_PUT_HEADER = " /PUT.html HTTP/1.1\nHost: 127.0.0.1\nAccept: text/html\r\n";
const char* s_PUT_BODY = "<html>\n<head>\n<title>This is the PUT method</title>\n</head>\n<body>\n<h1>nice to meet you</h1>\n</body>\n</html>\r\n";

void createSendMessage(char* i_SendBuff, const char* i_TypeOfRequest);

enum eTypeOfRequest
{
	Options = 1,
	Get = 2,
	Head = 3,
	Put = 4,
	Trace = 5,
	Delete = 6,
	Exit = 7
};

void main()
{

	// Initialize Winsock (Windows Sockets).

	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Web Client: Error at WSAStartup()\n";
		return;
	}

	// Client side:
	// Create a socket and connect to an internet address.

	SOCKET connSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == connSocket)
	{
		cout << "Web Client: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	//
	// Binding the created socket to the IP address and port representing "us".
	//
	//Note: for clients, if the socket is left unbound, the first call for connect() will 
	//bind the socket with arbitrary properties (IP and Port)

	//If specific values are required, you can use the following code:

	//struct sockaddr_in me;
	//me.sin_family = AF_INET;
	//me.sin_addr.s_addr = INADDR_ANY;
	//me.sin_port = 0;
	//
	// Binding the created socket to the IP address and port representing "us".
	//
	//if (bind(connSocket, (struct sockaddr*)&me, sizeof(me)) == SOCKET_ERROR)
	//{
	//	cout<<"Web Server: Error at bind(): "<<WSAGetLastError()<<endl;
	//  closesocket(connSocket);
	//	WSACleanup();
	//  return;
	//}

	// For a client to communicate on a network, it must connect to a server.    
	// Need to assemble the required data for connection in sockaddr structure.
	// Create a sockaddr_in object called server. 
	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(PORT);

	// Connect to server.

	// The connect function establishes a connection to a specified network 
	// address. The function uses the socket handler, the sockaddr structure 
	// (which defines properties of the desired connection) and the length of 
	// the sockaddr structure (in bytes).
	if (SOCKET_ERROR == connect(connSocket, (SOCKADDR *)&server, sizeof(server)))
	{
		cout << "Web Client: Error at connect(): " << WSAGetLastError() << endl;
		closesocket(connSocket);
		WSACleanup();
		return;
	}

	// Send and receive data.
	int bytesSent = 0;
	int bytesRecv = 0;
	char sendBuff[MESSAGE_SIZE];
	char recvBuff[MESSAGE_SIZE];
	int option = 0;
	eTypeOfRequest type;

	while (option != 7)
	{
		cout << "\nPlease choose an option:\n";
		cout << "1 - OPTIONS request.\n";
		cout << "2 - GET request.\n";
		cout << "3 - HEAD request.\n";
		cout << "4 - PUT request.\n";
		cout << "5 - TRACE request.\n";
		cout << "6 - DELETE request.\n";
		cout << "7 - Exit.\n";
		cout << ">> ";

		cin >> option;
		type = (eTypeOfRequest)option;

		switch (type)
		{
		case eTypeOfRequest::Options:
			createSendMessage(sendBuff, s_OPTIONS);
			break;
		case eTypeOfRequest::Get:
			createSendMessage(sendBuff, s_GET);
			break;
		case eTypeOfRequest::Head:
			createSendMessage(sendBuff, s_HEAD);
			break;
		case eTypeOfRequest::Put:
			createSendMessage(sendBuff, s_PUT);
			break;
		case eTypeOfRequest::Trace:
			createSendMessage(sendBuff, s_TRACE);
			break;
		case eTypeOfRequest::Delete:
			createSendMessage(sendBuff, s_DELETE);
			break;
		case eTypeOfRequest::Exit:
			strcpy(sendBuff, "Exit");
			break;
		}

		// The send function sends data on a connected socket.
		// The buffer to be sent and its size are needed.
		// The last argument is an idicator specifying the way 
		// in which the call is made (0 for default).
		bytesSent = send(connSocket, sendBuff, (int)strlen(sendBuff), 0);
		if (SOCKET_ERROR == bytesSent)
		{
			cout << "Web Client: Error at send(): " << WSAGetLastError() << endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}
		cout << "Web Client: Sent: " << bytesSent << "/" << strlen(sendBuff) << " bytes of \n\"" << sendBuff << "\" message.\n\n";

		// Gets the server's answer for options 1 and 2.
		if (option >= eTypeOfRequest::Options && option <= eTypeOfRequest::Exit)
		{
			bytesRecv = recv(connSocket, recvBuff, 255, 0);
			if (SOCKET_ERROR == bytesRecv)
			{
				cout << "Web Client: Error at recv(): " << WSAGetLastError() << endl;
				closesocket(connSocket);
				WSACleanup();
				return;
			}
			if (bytesRecv == 0)
			{
				cout << "Server closed the connection\n";
				return;
			}

			recvBuff[bytesRecv] = '\0'; //add the null-terminating to make it a string
			cout << "Web Client: Recieved: " << bytesRecv << " bytes of \n\"" << recvBuff << "\" message.\n\n";
		}
		else if (option == eTypeOfRequest::Exit)
		{
			// Closing connections and Winsock.
			cout << "Web Client: Closing Connection.\n";
			closesocket(connSocket);
			WSACleanup();
		}
	}
}

void createSendMessage(char* i_SendBuff, const char* i_TypeOfRequest) 
{
	char fullRequest[MESSAGE_SIZE];

	strcpy(fullRequest, i_TypeOfRequest);
	if (i_TypeOfRequest == s_PUT || i_TypeOfRequest == s_DELETE)
	{
		strcat(fullRequest, s_PUT_HEADER);
		if (i_TypeOfRequest == s_PUT)
		{
			strcat(fullRequest, "\n");
			strcat(fullRequest, s_PUT_BODY);
		}
	}
	else
	{
		strcat(fullRequest, s_MESSAGE_HEADER);
	}
	strcpy(i_SendBuff, fullRequest);
}

