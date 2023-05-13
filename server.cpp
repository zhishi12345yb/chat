/*
Here is a README file inside the folder, you can refer to it to get
some ideas about how to use this program. Before starting, please
make sure you have installed MySQL and the sql files have been put
into your database.
*/

#include <iostream>
#include<WinSock2.h>
#include<Windows.h>
#include<process.h>
#include<time.h>
#include<mysql.h>
#pragma comment(lib,"ws2_32.lib")
#define MAXSIZE 1024
SOCKET clientSocks[MAXSIZE];
HANDLE handle;// use a mutex to lock the critical section

// before getting started, you need to chage the following information to yours
#define host "localhost"
#define user "root"
#define pwd "030609"
#define database "groupchat"
#define port 3306

int clientCount = 0;//record how many clients are connected

// Send message to all clients (implement group chat later)
void SendMsg(char* szMsg, int iLen) {
	int i = 0;
	WaitForSingleObject(handle, INFINITE);
	for (i = 0; i < clientCount; i++) {
		//send message to all clients
		send(clientSocks[i], szMsg, iLen, 0);
	}
	ReleaseMutex(handle);
}

unsigned WINAPI HandleClient(void* arg) {
	// get the socket handle
	SOCKET HandleClientSock = *(SOCKET*)arg;
	int infoLen = 0, i;
	char szMsg[MAXSIZE] = { 0 };
	// use a loop to receive message from client
	while (true) {
		infoLen = recv(HandleClientSock, szMsg, sizeof(szMsg), 0);
		if (infoLen != -1) {
			// send message to all clients
			SendMsg(szMsg, infoLen);
		}
		else {
			break;// break the loop when the client disconnects
		}

	}
	// disconnect
	WaitForSingleObject(handle, INFINITE);
	for (i = 0; i < clientCount; i++) {
		// check all the clients
		if (HandleClientSock == clientSocks[i]) {
			//
			while (i < clientCount) {
				clientSocks[i] = clientSocks[i + 1];
				i++;
			}
			break;
		}
	}
	//connected user minus 1
	clientCount--;
	printf("Current Connect£º%d\n", clientCount);
	ReleaseMutex(handle);
	//close the socket
	closesocket(HandleClientSock);
	return 0;
}

int main() {
	WORD wVersion;// to check the version of socket
	WSADATA wsaData;
	int err;// to check the error code
	HANDLE handleThread;

	wVersion = MAKEWORD(1, 1);  //MAKEWORD(a,b) b|a<<8
	err = WSAStartup(wVersion, &wsaData);

	// check the error code
	if (err != 0) {
		return err;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();// clean up the socket if necessary
		return -1;
	}

	//create a mutex for the critical section
	handle = CreateMutex(NULL, FALSE, NULL);
	// create a socket
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
	// AF_INET: IPv4, SOCK_STREAM: TCP, 0: default protocol
	SOCKADDR_IN addrServer;
	addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	// INADDR_ANY: any address
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(6000);
	// htons: host to network short, define the port number
	if (bind(sockSrv, (SOCKADDR*)&addrServer, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		printf("Bind error: %d\n", GetLastError()); // get the error code and print it
	}
	// listen to the port
	if (listen(sockSrv, 10) == SOCKET_ERROR) {
		printf("Listen error:%d\n", GetLastError());
	}
	std::cout << "Server start!" << std::endl;

	// connect to the MySQL database
	MYSQL mysql;
	mysql_init(&mysql);
	mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "gbk");
	MYSQL* res = mysql_real_connect(&mysql, host, user, pwd, database, port, NULL, 0);
	if (res == NULL) {
		printf("Error connecting to database:%s\n", mysql_error(&mysql));
	}
	else {
		printf("Connected!\n");
	}
	int ret = mysql_select_db(&mysql, database);
	if (ret) {
		printf("Failed to connect! Error: %s\n", mysql_error(&mysql));
		return false;
	}
	printf("Database Selection Success£¡\n");

	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);
	char now[64];
	while (true) {
		// receive the socket
		SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		WaitForSingleObject(handle, INFINITE);
		// when a new user join in, the size of array should plus 1
		clientSocks[clientCount++] = sockConn;
		ReleaseMutex(handle);
		// start a new thread to handle the client
		handleThread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, (void*)&sockConn, 0, NULL);
		time_t nowTime = time(NULL);
		strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", localtime(&nowTime));
		printf("Connect client IP :%s (%s)\n", inet_ntoa(addrClient.sin_addr), now);
		WaitForSingleObject(handle, INFINITE);
		printf("Connect client num :%d\n", clientCount);
		ReleaseMutex(handle);
	}
	// close and clean the socket
	closesocket(sockSrv);
	WSACleanup();
	// shut
	system("pause");
}