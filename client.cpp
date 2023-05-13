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

#define BUF_SIZE 1024 // the size of the buffer
#define NAME_SIZE 20
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

// before getting started, you need to chage the following information to yours
#define host "localhost"
#define user "root"
#define pwd "030609"
#define database "groupchat"
#define port 3306

char szName[NAME_SIZE] = "[DEFAULT]";
char szMsg[BUF_SIZE];

// Send the message to the server
unsigned WINAPI SendMsg(void* arg) {
	SOCKET ClientSock = *(SOCKET*)arg;
	// the message to be sent whose size is the sum of the username and the message
	char szNameMsg[NAME_SIZE + BUF_SIZE];
	// receive the message from the console
	while (true) {
		// get the message from the console
		fgets(szMsg, BUF_SIZE, stdin);
		// exit if the user enters "EXIT" or "exit"
		if (!strcmp(szMsg, "EXIT\n") || !strcmp(szMsg, "exit\n")) {
			closesocket(ClientSock);
			exit(0);
		}

		// user can check the history of the chat by entering "history"
		if (!strcmp(szMsg, "history\n") || !strcmp(szMsg, "HISTORY\n")) {
			// connect to the database
			MYSQL mysql;
			mysql_init(&mysql);
			mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "gbk");
			mysql_real_connect(&mysql, host, user, pwd, database, port, NULL, 0);
			mysql_select_db(&mysql, database);
			// get the history of the chat
			char sql[1024];
			sprintf_s(sql, "select * from log");
			mysql_query(&mysql, sql);
			MYSQL_RES* result = mysql_store_result(&mysql);
			int num_fields = mysql_num_fields(result);
			MYSQL_ROW row;
			// print the history of the chat
			while (row = mysql_fetch_row(result)) {
				for (int i = 0; i < num_fields; i++) {
					// the '\n' in the content should not be printed
					if (i == 3) {
						// replace the '\n' by ' ' in the content
						for (int j = 0; j < strlen(row[i]); j++) {
							if (row[i][j] == '\n') {
								row[i][j] = ' ';
							}
						}
						printf("%s ", row[i] ? row[i] : "NULL");
					}
					else {
						printf("%s ", row[i] ? row[i] : "NULL");
					}
				}
				printf("\n");
			}
			mysql_free_result(result);
		}

		// Do not allow the user to send an empty message
		if (!strcmp(szMsg, "\n")) {
			continue;
		}

		// add the username to the message
		sprintf_s(szNameMsg, "%s %s", szName, szMsg);

		// add the record to the database
		char sql[1024];
		// get the current time and change its type to string
		time_t nowTime = time(NULL);
		char now[64];
		strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", localtime(&nowTime));
		char myIp[] = "127.0.0.1";
		sprintf_s(sql, "insert into log(nickname, time, content, ip) values('%s', '%s', '%s', '%s')", szName, now, szMsg, myIp);
		// connect to the database
		MYSQL mysql;
		mysql_init(&mysql);
		mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "gbk");
		mysql_real_connect(&mysql, host, user, pwd, database, port, NULL, 0);
		mysql_select_db(&mysql, database);
		mysql_query(&mysql, sql);
		mysql_close(&mysql);
		// send the message to the server
		send(ClientSock, szNameMsg, strlen(szNameMsg), 0);
	}
	return 0;
}

// Receive the message from the server
unsigned WINAPI RecvMsg(void* arg) {
	SOCKET ClientSock = *(SOCKET*)arg;
	char szNameMsg[NAME_SIZE + BUF_SIZE];
	int infoLen = 0;
	while (true) {
		infoLen = recv(ClientSock, szNameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		if (infoLen == -1) {
			return -1;
		}
		szNameMsg[infoLen] = 0;
		fputs(szNameMsg, stdout);
	}
	return 0;
}

// Function For the users to Log in, return true if the username and the password are correct
bool LogIn(MYSQL mysql) {
	char username[100];
	char password[100];
	// ask the user to input the username
	printf("Please input your username:");
	scanf_s("%s", username, 100);
	// ask the user to input the password
	printf("Please input your password:");
	scanf_s("%s", password, 100);
	// check if the username and the password are correct
	char sql[1024];
	sprintf_s(sql, "select * from user where username='%s' and password='%s'", username, password);
	int t = mysql_query(&mysql, sql);
	if (t) {
		printf("Error making query:%s\n", mysql_error(&mysql));
		return false;
	}
	else {
		MYSQL_RES* res = mysql_store_result(&mysql);
		int num_fields = mysql_num_fields(res);
		MYSQL_ROW row;
		if (row = mysql_fetch_row(res)) {
			return true;
		}
		else {
			printf("Wrong username or password!\n");
			return false;
		}
	}
}

// Function for the users to create an account
bool CreateAccount(MYSQL mysql) {
	char username[100];
	char password[100];
	// ask the user to input the username
	printf("Please input your username:");
	scanf_s("%s", username, 100);
	// ask the user to input the password
	printf("Please input your password:");
	scanf_s("%s", password, 100);
	// check if the username has been used
	char sql[1024];
	sprintf_s(sql, "select * from user where username='%s'", username);
	int t = mysql_query(&mysql, sql);
	if (t) {
		printf("Error making query:%s\n", mysql_error(&mysql));
		return false;
	}
	else {
		MYSQL_RES* res = mysql_store_result(&mysql);
		int num_fields = mysql_num_fields(res);
		MYSQL_ROW row;
		if (row = mysql_fetch_row(res)) {
			printf("The username has been used!\n");
			return false;
		}
		else {
			// insert the username and the password into the database
			sprintf_s(sql, "insert into user(username, password) values('%s', '%s')", username, password);
			int t = mysql_query(&mysql, sql);
			if (t) {
				printf("Error making query:%s\n", mysql_error(&mysql));
				return false;
			}
			else {
				return true;
			}
		}
	}
}

// Function for the users to delete their account
bool DeleteAccount(MYSQL mysql) {
	char username[100];
	char password[100];
	// ask the user to input the username
	printf("Please input your username:");
	scanf_s("%s", username, 100);
	// ask the user to input the password
	printf("Please input your password:");
	scanf_s("%s", password, 100);
	// check if the username has been used
	char sql[1024];
	sprintf_s(sql, "select * from user where username='%s' and password='%s'", username, password);
	int t = mysql_query(&mysql, sql);
	if (t) {
		printf("Error making query:%s\n", mysql_error(&mysql));
		return false;
	}
	else {
		MYSQL_RES* res = mysql_store_result(&mysql);
		int num_fields = mysql_num_fields(res);
		MYSQL_ROW row;
		if (row = mysql_fetch_row(res)) {
			// delete the username and the password from the database
			int user_id = atoi(row[0]);
			// Check if user is an administrator
			if (user_id >= 1 && user_id <= 5) {
				// Print all user information
				printf("Hello Administator!\n");
				sprintf_s(sql, "SELECT * FROM user");
				t = mysql_query(&mysql, sql);
				if (t) {
					printf("Error making query:%s\n", mysql_error(&mysql));
					return false;
				}
				else {
					res = mysql_store_result(&mysql);
					num_fields = mysql_num_fields(res);
					while ((row = mysql_fetch_row(res))) {
						for (int i = 0; i < num_fields; i++) {
							printf("%s\t", row[i]);
						}
						printf("\n");
					}
					mysql_free_result(res);
				}
				// Ask the administrator to input the ID of the user to update
				printf("Please input the ID of the user you want to delete:");
				int id;
				scanf_s("%d", &id);
				// Check if the ID is valid
				if (id <= 0) {
					printf("Invalid ID!\n");
					return false;
				}
				else {
					sprintf(sql, "delete from user where id=%d", id);
					t = mysql_query(&mysql, sql);
					if (t) {
						printf("Error making query:%s\n", mysql_error(&mysql));
						return false;
					}
					else {
						return true;
					}
				}
			}
			else {
				sprintf_s(sql, "delete from user where username='%s' and password='%s'", username, password);
				int t = mysql_query(&mysql, sql);
				if (t) {
					printf("Error making query:%s\n", mysql_error(&mysql));
					return false;
				}
				else {
					return true;
				}
			}
		}
		else {
			printf("Wrong username or password!\n");
			return false;
		}
	}
}

bool UpdateAccount(MYSQL mysql) {
	char username[100];
	char password[100];
	// ask the user to input the username
	printf("Please input your username:");
	scanf_s("%s", username, 100);
	// ask the user to input the password
	printf("Please input your password:");
	scanf_s("%s", password, 100);
	// check if the username has been used
	char sql[1024];
	sprintf_s(sql, "select * from user where username='%s' and password='%s'", username, password);
	int t = mysql_query(&mysql, sql);
	if (t) {
		printf("Error making query:%s\n", mysql_error(&mysql));
		return false;
	}
	else {
		MYSQL_RES* res = mysql_store_result(&mysql);
		int num_fields = mysql_num_fields(res);
		MYSQL_ROW row;
		if (row = mysql_fetch_row(res)) {
			int user_id = atoi(row[0]);
			// Check if user is an administrator
			if (user_id >= 1 && user_id <= 5) {
				// Print all user information
				printf("Hello Administator!\n");
				sprintf_s(sql, "SELECT * FROM user");
				t = mysql_query(&mysql, sql);
				if (t) {
					printf("Error making query:%s\n", mysql_error(&mysql));
					return false;
				}
				else {
					res = mysql_store_result(&mysql);
					num_fields = mysql_num_fields(res);
					while ((row = mysql_fetch_row(res))) {
						for (int i = 0; i < num_fields; i++) {
							printf("%s\t", row[i]);
						}
						printf("\n");
					}
					mysql_free_result(res);
				}
				// Ask the administrator to input the ID of the user to update
				printf("Please input the ID of the user you want to update:");
				int id;
				scanf_s("%d", &id);
				// Check if the ID is valid
				if (id <= 0) {
					printf("Invalid ID!\n");
					return false;
				}
				// Ask the administrator to input the new username
				printf("Please input the new username:");
				scanf_s("%s", username, 100);
				// Ask the administrator to input the new password
				printf("Please input the new password:");
				scanf_s("%s", password, 100);
				// Update the user information in the database
				sprintf_s(sql, "UPDATE user SET username='%s', password='%s' WHERE id=%d", username, password, id);
				t = mysql_query(&mysql, sql);
				if (t) {
					printf("Error making query:%s\n", mysql_error(&mysql));
					return false;
				}
				else {
					return true;
				}
			}
			else {
				sprintf_s(sql, "delete from user where username='%s' and password='%s'", username, password);
				int t = mysql_query(&mysql, sql);
				if (t) {
					printf("Error making query:%s\n", mysql_error(&mysql));
					return false;
				}
				else {
					// ask the user to input the new username
					printf("Please input your new username:");
					scanf_s("%s", username, 100);
					// ask the user to input the new password
					printf("Please input your new password:");
					scanf_s("%s", password, 100);
					// insert the new username and the new password into the database
					sprintf_s(sql, "insert into user(username, password) values('%s', '%s')", username, password);
					int t = mysql_query(&mysql, sql);
					if (t) {
						printf("Error making query:%s\n", mysql_error(&mysql));
						return false;
					}
					else {
						return true;
					}
				}
			}
		}
		else {
			printf("Wrong username or password!\n");
			return false;
		}
	}
}

int main() {
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
	mysql_select_db(&mysql, database);

	int operation;
	while (1) {
		// ask the user to choose the operation
		printf("Please choose the operation\n1. Log in\n2. Create an Account\n3. Delete Account\n4. Update Account\n5. EXIT\nInput number 1 to 5 to select the operation:\n");
		scanf_s("%d", &operation);
		// execute the operation choosen by the user
		if (operation == 1) {
			// Log in
			if (LogIn(mysql)) {
				printf("Log in successfully!\n");
				break;// if the username and the password are correct, the user can log in
			}
			else {
				printf("Log in failed!\n");
				continue;// if the username or the password is wrong, the user can log in again
			}
		}
		else if (operation == 2) {
			// Create an account
			if (CreateAccount(mysql)) {
				printf("Create an account successfully!\n");
				continue;// after creating an account, the user can log in
			}
			else {
				printf("Create an account failed!\n");
				continue;// if the username has been used, the user can create an account again
			}
		}
		else if (operation == 3) {
			// Delete an account
			if (DeleteAccount(mysql)) {
				printf("Delete an account successfully!\n");
				continue;// after deleting an account, the user can log in
			}
			else {
				printf("Delete an account failed!\n");
				continue;// if the username or the password is wrong, the user can delete an account again
			}
		}
		else if (operation == 4) {
			// Update an account
			if (UpdateAccount(mysql)) {
				printf("Update an account successfully!\n");
				continue;// after updating an account, the user can log in
			}
			else {
				printf("Update an account failed!\n");
				continue;// if the username or the password is wrong, the user can update an account again
			}
		}
		else if (operation == 5) {
			// EXIT
			printf("EXIT!\n");
			return 0;
		}
		else {
			printf("Wrong operation!\n");
			continue;// if the user input the wrong operation, the user can input again
		}
	}

	// ask the user to input the username
	char nickName[NAME_SIZE];
	printf("Please input your nickname:");
	scanf_s("%s", nickName, NAME_SIZE);

	// initialize
	WORD wVersion;
	WSADATA wsaData;
	int err;
	HANDLE SendThread, RecvThread;

	wVersion = MAKEWORD(1, 1);
	err = WSAStartup(wVersion, &wsaData);
	if (err != 0) {
		return err;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return -1;
	}

	// show the username who is sending the message
	sprintf_s(szName, "[%s]:", nickName);
	SOCKET sockCli = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6000);

	// connect to the server
	if (connect(sockCli, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		printf("connect error error code£º%d", GetLastError());
		return -1;
	}

	// new thread for sending and receiving messages
	SendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&sockCli, 0, NULL);
	RecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&sockCli, 0, NULL);
	WaitForSingleObject(SendThread, INFINITE);
	WaitForSingleObject(RecvThread, INFINITE);
	WSACleanup();
	mysql_close(&mysql);
	return 0;
}