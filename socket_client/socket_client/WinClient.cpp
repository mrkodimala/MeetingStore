#include <winsock2.h>
#include <StdAfx.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <conio.h>

int getsocket()
{
	int hsock;
	int * p_int ;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		printf("Error initializing socket %d\n",WSAGetLastError());
		return -1;
	}
	
	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		return -1;
	}
	free(p_int);

	return hsock;
}
void SendCommand(char *command, struct sockaddr_in my_addr);

void ProcessMenus(char *command,struct sockaddr_in my_addr)
{
	char screen[32];
	char printstring[1024];
	char sendingcommand[1024];
	char temp[30];
	int l_offset = 0;
	int i = 1;
	int getstring_flag = 0;
	for (; command[i] != '$';i++)
		screen[l_offset++] = command[i];
	screen[l_offset] = '\0';
	i++;
	l_offset = 0;
	for (; command[i] != '$'; i++)
		printstring[l_offset++] = command[i];
	printstring[l_offset] = '\0';
	i++;
	l_offset = 0;
	for (; command[i] != '$'; i++)
		temp[l_offset++] = command[i];
	temp[l_offset] = '\0';
	int Minimum = atoi(temp);
	i++;
	l_offset = 0;
	for (; command[i] != '$'; i++)
		temp[l_offset++] = command[i];
	temp[l_offset] = '\0';
	int Maximum = atoi(temp);
	i++;
	l_offset = 0;
	for (; command[i] != '$'; i++)
		temp[l_offset++] = command[i];
	temp[l_offset] = '\0';
	if (strlen(temp) > 0)
		getstring_flag = 1;
	while (1)
	{
		system("CLS");
		printf("%s\n", printstring);
		if (Minimum >= 0 && Maximum > 0)
		{
			printf("\n\tEnter your option\n\t");
			int option;
			scanf("%d", &option);
			if (option >= Minimum&&option <= Maximum)
			{
				char name[32];
				if (getstring_flag == 1)
				{
					printf("\n\tEnter Name\n\t");
					gets(name);
					gets(name);
				}
				strcpy(sendingcommand, "$");
				strcat(sendingcommand, screen);
				strcat(sendingcommand, "$");
				strcat(sendingcommand, itoa(option, temp, 10));
				strcat(sendingcommand, "$");
				if (getstring_flag > 0)
				{
					strcat(sendingcommand, name);
				}
				else{
					strcat(sendingcommand, "@@@");
				}
				strcat(sendingcommand, "$");
				SendCommand(sendingcommand, my_addr);
				return;
			}
		}
		else{
			break;
		}
	}
}


int checkDateIsValid(char *date)
{
	int length = strlen(date);
	char tempdate[3];
	char tempmonth[3];
	char tempyear[5];
	int i;
	int count = 0;
	for (i = 0; i < length; i++)
	{
		if (!((date[i] >= '0'&&date[i] <= '9') || date[i] == '-'))
			return 0;
		if (date[i] == '-')
			count++;
	}
	if (count != 2)
		return 0;
	if (date[3] != '-'&&date[5] != '-')
		return 0;
	tempdate[0] = date[0]; tempdate[1] = date[1]; tempdate[2] = '\0';
	tempmonth[0] = date[3]; tempmonth[1] = date[4]; tempmonth[2] = '\0';
	tempyear[0] = date[6]; tempyear[1] = date[7]; tempyear[2] = date[8]; tempyear[3] = date[9]; tempyear[4] = '\0';
	int day = atoi(tempdate);
	int month = atoi(tempmonth);
	int year = atoi(tempyear);
	if (month == 2 && day > 29)
		return 0;
	if ((month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) && day > 31)
		return 0;
	if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30)
		return 0;
	if (year<2000 || year>3000)
		return 0;
	return 1;
}

void ProcessDate(char *command, struct sockaddr_in my_addr)
{
	char screen[32];
	char returnBuffer[128];
	int i;
	int offset = 0;
	for (i = 1; command[i] != '$'; i++)
		screen[offset++] = command[i];
	screen[offset] = '\0';
	char printstring[1024];
	i++;
	offset = 0;
	for (; command[i] != '$'; i++)
		printstring[offset++] = command[i];
	printstring[offset] = '\0';
	char date[12];
	while (1)
	{
		system("CLS");
		printf("%s\n\t", printstring);
		gets(date);
		gets(date);
		int k = checkDateIsValid(date);
		printf("k=%d", k);
		getchar();
		if (k)
		{
			strcpy(returnBuffer, "$");
			strcat(returnBuffer, screen);
			strcat(returnBuffer, "$");
			strcat(returnBuffer, date);
			strcat(returnBuffer, "$");
			SendCommand(returnBuffer, my_addr);
			return;
		}
	}
}

void ProcessCommandFromServer(char *command,struct sockaddr_in my_addr)
{
	
	if (command[0] == '@')
	{
		ProcessMenus(command,my_addr);
	}
	else if (command[0] == '%')
	{
		ProcessDate(command, my_addr);
	}
	else{
		system("ClS");
		printf("%s\n", command);
		getchar();
		getchar();
		SendCommand("$opened$", my_addr);
	}
}


void SendCommand(char *command, struct sockaddr_in my_addr)
{
	char buffer[1024];
	strcpy(buffer, command);
	int buffer_len = 1024;
	int bytecount;

	int hsock = getsocket();
	//add error checking on hsock...
	if (connect(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == SOCKET_ERROR){
		fprintf(stderr, "Error connecting socket %d\n", WSAGetLastError());
		return;
	}
	if ((bytecount = send(hsock, buffer, strlen(buffer), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		return;
	}
	printf("Sent bytes %d\n", bytecount);

	if ((bytecount = recv(hsock, buffer, buffer_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		return;
	}
	char temp[1024];
	int i;
	for (i = 0; buffer[i] != '#'; i++)
	{
		temp[i] = buffer[i];
	}
	temp[i] = '\0';
	ProcessCommandFromServer(temp, my_addr);
	closesocket(hsock);
}


void socket_client()
{

	//The port and address you want to connect to
	int host_port= 1101;
	char* host_name="127.0.0.1";

	//Initialize socket support WINDOWS ONLY!
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );
 	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 || ( LOBYTE( wsaData.wVersion ) != 2 ||
		    HIBYTE( wsaData.wVersion ) != 2 )) {
	    fprintf(stderr, "Could not find sock dll %d\n",WSAGetLastError());
		return;
	}

	//Initialize sockets and set any options

	//Connect to the server
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);
	
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(host_name);

	
	SendCommand("$opened$", my_addr);
	getchar();
}