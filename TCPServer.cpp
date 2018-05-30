#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
//#define D_SCL_SECURE_NO_WARNINGS


#include <iostream>
#include <sstream>

using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <fstream>

char* str2char(string s);
char* headRequest(int index);
char* getRequest(int index, bool onlyHead);
char* putRequest(string fileContent, int index);
char* headerFields();
char* traceRequest(int index);
char* getFileName(char* fileName);
char* getContent(int index);
char* optionsRequest();
char* deleteRequest(int index);



enum eMessageIndex {
	OK_200 = 0,
	ERROR_404,
	CREATED_201,
	NOT_IMP_501,
	HOST,
	CONTENT,
	LNG,
	CONNECTION, 
	ASSISTANT,
	CONTENT_LENGTH
};
char* messages[] = { "HTTP/1.1 200 OK\r\n", "HTTP/1.1 400 Bad Request\r\n"
, "HTTP/1.1 201 Created\r\n"
, "HTTP/1.1 501 Not Implemented\r\n"
, "Host: www.worshipInara.com\r\n"
, "Content - Type: text / html\r\n"
, "Accept - Language: en - us\r\n"
, "Connection: Keep - Alive\r\n"
, "Assistant: Nikita/1.0\r\n"
, "Content - Length: " };

enum eRequestType
{
	Options = 0,
	Get, 
	Head, 
	Put,
	Trace ,
	Delete,
	Exit,
};

char *requestType[] = { "OPTIONS" ,"GET", "HEAD","PUT","TRACE", "DELETE", "Exit" };

#define PATH_SIZE 1024

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[128];
	int len;
	
	char filePath[PATH_SIZE];
};

const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;


bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

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
		cout << "Time Server: Error at WSAStartup()\n";
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
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
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
	serverService.sin_port = htons(TIME_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
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
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

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
	cout << "Time Server: Closing Connection.\n";
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
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Time Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
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
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
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
		cout << "Time Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			for (int i = 0; i <= eRequestType::Exit; i++)
			{
			
				char* currRequestType = requestType[i];
				int reqLength = strlen(const_cast<char*>(currRequestType));

				// if exit
				if (strcmp(currRequestType, requestType[Exit]) == 0 && strncmp(sockets[index].buffer, currRequestType, reqLength) == 0)
				{
					closesocket(msgSocket);
					removeSocket(index);
					return;
				}
				// if other requests
				else if (strncmp(sockets[index].buffer, currRequestType, reqLength) == 0)
				{
					sockets[index].send = SEND;
					sockets[index].sendSubType = i;
					memcpy(sockets[index].buffer, &sockets[index].buffer[reqLength], sockets[index].len - reqLength);
					sockets[index].len -= reqLength;
					return;
				}

			}
		}
	}

}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[255];
	char *response, *content;

	SOCKET msgSocket = sockets[index].id;

	switch (sockets[index].sendSubType)
	{
	case eRequestType::Get:
		response = getRequest(index,false);
		strcpy(sendBuff, response);
		break;
	case eRequestType::Head:
		response = headRequest(index);
		strcpy(sendBuff, response);
		break;
	case eRequestType::Delete:
		response = deleteRequest(index);
		strcpy(sendBuff, response);
		break;
	case eRequestType::Options:
		response = optionsRequest();
		strcpy(sendBuff, response);
		break;
	case eRequestType::Put:
		content = getContent(index);
		response = putRequest(content, index);
		strcpy(sendBuff, response);
		break;
	case eRequestType::Trace:
		response = traceRequest(index);
		strcpy(sendBuff, response);
		break;
	case eRequestType::Exit:
		closesocket(msgSocket);
		removeSocket(index);
		return;

	default:

		break;
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Time Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

	sockets[index].send = IDLE;
}

char* optionsRequest()
{
	string res = string(messages[NOT_IMP_501]);
	res += "Allow: HEAD, GET, PUT, DELETE, TRACE, OPTIONS\r\n";
	res += string(headerFields());
	
	return str2char(res);
}

char* putRequest(string fileContent, int index)
{
	string res = "";
	ofstream file;
	char* filename = getFileName(sockets[index].filePath);
	file.open(filename);
	if (file.is_open())
	{
		
		file << getContent(index);
		file.close();

		res += messages[CREATED_201];
		res += "Content-Location: /" + string(filename);
	}
	else
	{
		res += messages[ERROR_404];
	}

	return  str2char(res);
}

char* getRequest(int index, bool onlyHead)
{
	string res, currentLine;
	int contentLength = 0;
	ifstream file;
	file.open(sockets[index].filePath);

	if (file.is_open())
	{
		res += messages[OK_200];
		res += headerFields();


		string content = "";
		while (!file.eof())
		{
			getline(file, currentLine);
			content += currentLine;
		}
		content += "\r\n";
		contentLength = content.length();

		res += messages[CONTENT_LENGTH] + to_string(contentLength) + "\r\n";

		if (!onlyHead) // for HEAD request
		{
			return str2char(res);
		}

		res += content + "\r\n"; // GET request

		file.close();

	}
	else
	{
		res += messages[ERROR_404];
	}

	return str2char(res);
}

char* deleteRequest(int index)
{
	string res = "";
	ofstream file;
	file.open(sockets[index].filePath);
	if (!file)
	{
		res += messages[ERROR_404];
	}
	else
	{
		file.close();
		if (remove(sockets[index].filePath))
		{
			res += messages[OK_200];
			res += "Web server:file deleted successfully\r\n";
		}
		else
		{
			res += messages[NOT_IMP_501];
			res += "Web server: error occured while executing request\r\n";
		}
	}

	return str2char(res);
}

char* headRequest(int index)
{
	return getRequest(index, false);
}

char* headerFields()
{
	char* val="";
	// Get the current time.
	time_t timer;
	time(&timer);
	// Parse the current time to printable string.
	strcpy(val, ctime(&timer));
	val[strlen(val) - 1] = 0; //to remove the new-line from the created string

	string res ="Date: " + string(val) + "\n";
	res += string(messages[HOST]) + string(messages[CONTENT]) + string(messages[LNG]) + string(messages[CONNECTION]) + string(messages[ASSISTANT]);
	res += string(messages[CONTENT_LENGTH]);

	return str2char(res);
}

char* str2char(string s)
{

	return _strdup(s.c_str());

}

char* traceRequest(int index)
{
	string res="";

	if (strlen(sockets[index].buffer) > 0)
	{
		res += string(messages[OK_200]) + string(headerFields());
		res += to_string(strlen(sockets[index].buffer)) + "\r\n";
		res += string(sockets[index].buffer);
	}
	else if (strlen(sockets[index].buffer) == 0)
	{
		res += string(messages[NOT_IMP_501]);
		res += "Web Server: couldn't reflect received message.\r\n";
	}

	return str2char(res);
}

char* getFileName(char* fileName)
{
	string s = string(fileName);
	const size_t last_slash_idx = s.find_last_of("\\/");
	
	if (std::string::npos != last_slash_idx)
	{
		s.erase(0, last_slash_idx + 1);
	}

	return str2char(s);

}

char* getContent(int index)
{
	string res;
	string originalMsg = sockets[index].buffer;
	int startPos = originalMsg.find("\r\n\r\n");
	if (startPos != string::npos) // if we have a body - we will write it into the file
	{
		string res = originalMsg.substr(startPos + 4, strlen(originalMsg.c_str()));
	}

	return str2char(res);
}
