/*
Idan Goor I.D. 307948836
Dan Nechushtan I.D 304990583
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <time.h>

const int PORT = 8080;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;
const int SEND_BAD_REQUEST = 8;
const int MESSAGE_SIZE = 1024;
const int PATH_SIZE = 30;
const int TIMEOUT = 120;

const char* HTTP_CODE_200_OK = "HTTP/1.1 200 OK";
const char* HTTP_CODE_201_CREATED = "HTTP / 1.1 201 Created";
const char* HTTP_CODE_204_NO_CONTENT = "HTTP/1.1 204 No Content";
const char* HTTP_CODE_404_NOT_FOUND = "HTTP/1.1 404 Not Found";
const char* HTTP_CODE_500_INTERNAL_SERVER_ERROR = "HTTP/1.1 500 Internal Server Error";
const char* HTTP_CODE_501_NOT_IMPLEMENTED = "HTTP/1.1 501 Not Implemented";

const char* s_OPTIONS = "OPTIONS";
const char* s_GET = "GET";
const char* s_HEAD = "HEAD";
const char* s_PUT = "PUT";
const char* s_TRACE = "TRACE";
const char* s_DELETE = "DELETE";
const char* s_EXIT = "Exit";

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

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[MESSAGE_SIZE];
	int len;
	time_t timeCounter;
};


bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

void calculateMinTime(timeval *i_Time);
void ifTimeOutExpiredThenCloseSocket();
void handleReceivedMessage(int index, eTypeOfRequest i_Type, const char* i_RequestName);
void executeOptions(int index, char* i_SendBuff);
void executeGET(int index, char* i_SendBuff);
void executeHEAD(int index, char* i_SendBuff);
void executePUT(int index, char* i_SendBuff);
void executeTRACE(int index, char* i_SendBuff);
void executeDELETE(int index, char* i_SendBuff);

void getFilePath(int index, char* i_Path);
void getFileContent(int index, char* i_Content);
void makePathLegal(char* i_Path);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;


void main()
{
	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Web Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Web Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "Web Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Web Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//

		timeval timeout;
		calculateMinTime(&timeout);

		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeout);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Web Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		ifTimeOutExpiredThenCloseSocket();

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Web Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			time(&sockets[i].timeCounter);
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Web Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Web Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Web Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Web Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Web Server: Recieved: " << bytesRecv << " bytes of the request \n\"" << &sockets[index].buffer[0] << "\".\n\n";

		sockets[index].len += bytesRecv;
		time(&sockets[index].timeCounter);

		if (strncmp(sockets[index].buffer, s_OPTIONS, strlen(s_OPTIONS)) == 0)
		{
			handleReceivedMessage(index, eTypeOfRequest::Options, s_OPTIONS);
		}
		else if (strncmp(sockets[index].buffer, s_GET, strlen(s_GET)) == 0)
		{
			handleReceivedMessage(index, eTypeOfRequest::Get, s_GET);
		}
		else if (strncmp(sockets[index].buffer, s_HEAD, strlen(s_HEAD)) == 0)
		{
			handleReceivedMessage(index, eTypeOfRequest::Head, s_HEAD);
		}
		else if (strncmp(sockets[index].buffer, s_PUT, strlen(s_PUT)) == 0)
		{
			handleReceivedMessage(index, eTypeOfRequest::Put, s_PUT);
		}
		else if (strncmp(sockets[index].buffer, s_TRACE, strlen(s_TRACE)) == 0)
		{
			handleReceivedMessage(index, eTypeOfRequest::Trace, s_TRACE);
		}
		else if (strncmp(sockets[index].buffer, s_DELETE, strlen(s_DELETE)) == 0)
		{
			handleReceivedMessage(index, eTypeOfRequest::Delete, s_DELETE);
		}
		else if (strncmp(sockets[index].buffer, s_EXIT, strlen(s_EXIT)) == 0)
		{
			closesocket(msgSocket);
			removeSocket(index);
			return;
		}
		else
		{
			sockets[index].send = SEND;
			sockets[index].sendSubType = SEND_BAD_REQUEST;
			return;
		}
	}

}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[MESSAGE_SIZE];

	eTypeOfRequest type = (eTypeOfRequest)sockets[index].sendSubType;
	SOCKET msgSocket = sockets[index].id;

	if (sockets[index].sendSubType == eTypeOfRequest::Options)
	{
		executeOptions(index, sendBuff);
	}
	else if (sockets[index].sendSubType == eTypeOfRequest::Get)
	{
		executeGET(index, sendBuff);
	}
	else if (sockets[index].sendSubType == eTypeOfRequest::Head)
	{
		executeHEAD(index, sendBuff);
	}
	else if (sockets[index].sendSubType == eTypeOfRequest::Put)
	{
		executePUT(index, sendBuff);
	}
	else if (sockets[index].sendSubType == eTypeOfRequest::Trace)
	{
		executeTRACE(index, sendBuff);
	}
	else if (sockets[index].sendSubType == eTypeOfRequest::Delete)
	{
		executeDELETE(index, sendBuff);
	}
	else //SEND_BAD_REQUEST
	{

	}



	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Web Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \n\"" << sendBuff << "\" message.\n\n";

	sockets[index].len = 0;
	sockets[index].send = IDLE;
}

void getFilePath(int index, char* i_Path)
{
	char temp[MESSAGE_SIZE];

	strcpy(temp, sockets[index].buffer);
	makePathLegal(temp);
	strcpy(i_Path, strtok(temp, " "));
	strcpy(i_Path, i_Path + 1); // because we want to get ride of the first character '\'
}

void makePathLegal(char* i_Path)
{
	for (int i = 0; i < strlen(i_Path); i++) 
	{
		if (i_Path[i] == '/')
		{
			i_Path[i] = '\\';
		}
	}
}

void getFileContent(int index, char* i_Content)
{
	char temp[MESSAGE_SIZE];
	char* pointer;

	sockets[index].buffer[sockets[index].len] = '\0';
	strcpy(temp, sockets[index].buffer);
	pointer = strchr(sockets[index].buffer, '\r');
	pointer++;
	strcpy(i_Content, pointer);
}

void executeOptions(int index, char* i_SendBuff) 
{
	char* response = new char[MESSAGE_SIZE];
	char* contentLength = new char[3];

	strcpy(response, HTTP_CODE_200_OK);
	strcat(response, "\r\nContent-Type: text/html");
	strcat(response, "\r\nContent-Length: ");
	_itoa(strlen(s_TRACE) + strlen(sockets[index].buffer), contentLength, 10);
	strcat(response, contentLength);
	strcat(response, "\r\n\r\n");
	strcat(response, "Allow: GET, HEAD, PUT, TRACE, DELETE, OPTIONS");
	strcpy(i_SendBuff, response);
}

void executeGET(int index, char* i_SendBuff)
{
	ifstream requestedFile;
	ostringstream out;
	string str;
	char* response = new char[MESSAGE_SIZE];
	char path[PATH_SIZE];
	char* contentLength = new char[3];

	getFilePath(index, path);
	requestedFile.open(path, ios::in);
	if (!requestedFile) // if file cant be opened
	{
		strcpy(response, HTTP_CODE_404_NOT_FOUND);
	}
	else 
	{
		strcpy(response, HTTP_CODE_200_OK);
		strcat(response, "\r\nContent-Type: text/html");
		strcat(response, "\r\nContent-Length: ");
		out << requestedFile.rdbuf(); // returns a pointer to the stream buffer object currently associated with the stream.
		str = out.str();
		_itoa(str.length(), contentLength, 10);
		strcat(response, contentLength);
		strcat(response, "\r\n\r\n");
		strcat(response, str.c_str());
		requestedFile.close();
	}
	strcpy(i_SendBuff, response);
}

void executeHEAD(int index, char* i_SendBuff)
{
	ifstream requestedFile;
	ostringstream out;
	string str;
	char* response = new char[MESSAGE_SIZE];
	char path[PATH_SIZE];
	char* contentLength = new char[3];

	getFilePath(index, path);
	requestedFile.open(path, ios::in);
	if (!requestedFile) // if file cant be opened
	{
		strcpy(response, HTTP_CODE_404_NOT_FOUND);
	}
	else 
	{
		strcpy(response, HTTP_CODE_200_OK);
		strcat(response, "\r\nContent-Type: text/html");
		strcat(response, "\r\nContent-Length: ");
		out << requestedFile.rdbuf();
		str = out.str();
		_itoa(str.length(), contentLength, 10);
		strcat(response, contentLength);
		strcat(response, "\r\n\r\n");
		requestedFile.close();
	}
	strcpy(i_SendBuff, response);
}

void executePUT(int index, char* i_SendBuff)
{
	ifstream requestedFile;
	ofstream outputFile;
	ostringstream out;
	char* response = new char[MESSAGE_SIZE];
	char path[PATH_SIZE];
	char content[MESSAGE_SIZE];
	char* contentLength = new char[3];

	getFilePath(index, path);
	getFileContent(index, content);
	requestedFile.open(path, ios::in);

	if (!requestedFile) // file is not exists, so we need to create it
	{
		strcpy(response, HTTP_CODE_201_CREATED);
	}
	else // file exists
	{
		strcpy(response, HTTP_CODE_200_OK);
	}

	requestedFile.close();
	outputFile.open(path, ios::trunc);
	if (!outputFile)
	{
		strcpy(response, HTTP_CODE_501_NOT_IMPLEMENTED);
	}
	else
	{
		outputFile << content;
		strcat(response, "\r\nContent-Type: text/html");
		strcat(response, "\r\nContent-Length: ");
		_itoa(strlen(content), contentLength, 10);
		strcat(response, contentLength);
		strcat(response, "\r\n\r\n");
		strcat(response, content);
	}
	strcpy(i_SendBuff, response);
	outputFile.close();
}


void executeTRACE(int index, char* i_SendBuff)
{
	ifstream requestedFile;
	char* response = new char[MESSAGE_SIZE];
	char path[PATH_SIZE];
	char* contentLength = new char[3];

	getFilePath(index, path);
	requestedFile.open(path, ios::in);
	if (!requestedFile)
	{
		strcpy(response, HTTP_CODE_404_NOT_FOUND);
	}
	else
	{
		sockets[index].buffer[sockets[index].len] = '\0';
		strcpy(response, HTTP_CODE_200_OK);
		strcat(response, "\r\nContent-Type: text/html");
		strcat(response, "\r\nContent-Length: ");
		_itoa(strlen(s_TRACE) + strlen(sockets[index].buffer), contentLength, 10);
		strcat(response, contentLength);
		strcat(response, "\r\n\r\n");
		strcat(response, s_TRACE);
		strcat(response, sockets[index].buffer);
		requestedFile.close();
	}
	strcpy(i_SendBuff, response);
}

void executeDELETE(int index, char* i_SendBuff)
{
	ifstream requestedFile;
	char* response = new char[MESSAGE_SIZE];
	char path[PATH_SIZE];

	getFilePath(index, path);
	requestedFile.open(path, ios::in);
	if (!requestedFile)
	{
		strcpy(response, HTTP_CODE_404_NOT_FOUND);
	}
	else
	{
		requestedFile.close();
		if (remove(path) != 0)
		{
			cout << "Web Server: Error. Cant delete file.\n";
			strcpy(response, HTTP_CODE_501_NOT_IMPLEMENTED);
		}
		else
		{
			cout << "Web Server: File was deleted.\n";
			strcpy(response, HTTP_CODE_204_NO_CONTENT);
		}
	}
	strcpy(i_SendBuff, response);
}


void calculateMinTime(timeval *i_Time) 
{
	time_t currentTime = time(NULL);
	long minimumTime = TIMEOUT;

	for (int i = 1; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv != EMPTY)
		{
			if (difftime(currentTime, sockets[i].timeCounter) < minimumTime)
			{
				minimumTime = sockets[i].timeCounter;
			}
		}
	}

	i_Time->tv_sec = minimumTime;
}


void ifTimeOutExpiredThenCloseSocket()
{
	time_t currentTime = time(NULL);
	for (int i = 1; i < MAX_SOCKETS; i++) //runs from index 1 because we dont want to close the LISTEN SOCKET
	{
		if (sockets[i].recv != EMPTY)
		{
			if (difftime(currentTime, sockets[i].timeCounter) >= TIMEOUT)
			{
				removeSocket(i);
				closesocket(sockets[i].id);
				cout << "Web Server: Socket was closed.\n";
			}
		}
	}
}

void handleReceivedMessage(int index, eTypeOfRequest i_Type, const char* i_RequestName)
{
	int nameLength = strlen(i_RequestName);

	sockets[index].send = SEND;
	sockets[index].sendSubType = i_Type;
	sockets[index].len -= nameLength;
	memcpy(sockets[index].buffer, &sockets[index].buffer[nameLength], sockets[index].len);
}