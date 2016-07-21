#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
struct node
{
	char msg[128];
	int msg_id;
	node *next;
}*flist,*alist,*printid;

struct bufserv{
	
		int userId;
		int forumId;
		int msgId;
		int commentId;
		int choice;
		char *forumname;
		char msg[128];
}buf1;

bool flag=true;
int mid = 0;
int count1 =0;
char *Data[100];
int count=1;
int values[100];
DWORD WINAPI SocketHandler(void*);
void replyto_client(char *buf, int *csock);

void socket_server() {

	//The port you want the server to listen on
	int host_port= 1101;

	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );
 	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 || ( LOBYTE( wsaData.wVersion ) != 2 ||
		    HIBYTE( wsaData.wVersion ) != 2 )) {
	    fprintf(stderr, "No sock dll %d\n",WSAGetLastError());
		goto FINISH;
	}

	//Initialize sockets and set options
	int hsock;
	int * p_int ;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		printf("Error initializing socket %d\n",WSAGetLastError());
		goto FINISH;
	}
	
	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		goto FINISH;
	}
	free(p_int);

	//Bind and listen
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);
	
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;
	
	/* if you get error in bind 
	make sure nothing else is listening on that port */
	if( bind( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		fprintf(stderr,"Error binding to socket %d\n",WSAGetLastError());
		goto FINISH;
	}
	if(listen( hsock, 10) == -1 ){
		fprintf(stderr, "Error listening %d\n",WSAGetLastError());
		goto FINISH;
	}
	
	//Now lets do the actual server stuff

	int* csock;
	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);
	
	while(true){
		printf("waiting for a connection\n");
		csock = (int*)malloc(sizeof(int));
		
		if((*csock = accept( hsock, (SOCKADDR*)&sadr, &addr_size))!= INVALID_SOCKET ){
			//printf("Received connection from %s",inet_ntoa(sadr.sin_addr));
			CreateThread(0,0,&SocketHandler, (void*)csock , 0,0);
		}
		else{
			fprintf(stderr, "Error accepting %d\n",WSAGetLastError());
		}
	}

FINISH:
;
}

struct BitVector{
	int values[1024];
};
const int USERSOFFSET = 8192;
const int blocksize = 64;
int CURRENT_USER;

struct User{
	char name[28];
	int offset;
};

struct UsersData{
	struct User values[127];
	int userscount;
};

struct Meeting{
	char name[32];
	char Date[16];
	int next;
	char unused[12];
};

int GiveFreeBlock()
{
	int s = 1;
	int flag = 0;
	int count = 0;
	int j = 0;
	struct BitVector b;
	FILE *file = fopen("store.bin", "r+b");
	fread(&b, sizeof(struct BitVector), 1, file);
	for (int i = 0; i < 16384; i++)
	{
		int v = b.values[i];
		j = 1;
		s = 1;
		if (v < 4294967295)
		{
			//printf("%d\t", v);
			do{
				flag = v&s;
				count++;
				if (flag == 0)
				{
					b.values[i] = v^s;
					printf("\nB value=%d\n", b.values[i]);
					fseek(file, 0, SEEK_SET);
					fwrite(&b, sizeof(struct BitVector), 1, file);
					fclose(file);
					return USERSOFFSET + (count * blocksize);
				}
				s = s << 1;
				j++;
			} while (j < 32);
		}
		else{
			count = count + 31;
		}
	}
	fclose(file);
	return 0;
}


void FreeBlock(int blockno)
{
	FILE *file = fopen("store.bin", "rb+");
	struct BitVector b;
	fread(&b, sizeof(struct BitVector), 1, file);
	blockno = blockno - USERSOFFSET;
	blockno = blockno / 64;
	int d = blockno / 32;
	int r = blockno % 32;
	unsigned int s = 1;
	unsigned int v = b.values[d];
	int i;
	if (d == 0)
		i = 2;
	else
		i = 0;
	for (; i <= r; i++)
	{
		s = s << 1;
	}
	b.values[d] = v^s;
	fseek(file, 0, SEEK_SET);
	fread(&b, sizeof(struct BitVector), 1, file);
	fclose(file);
}

void SetMainScreen(char *command)
{
	strcpy(command, "@mainscreen$\n\tMENU\n\n\t1.New User\n\t2.Login\t\n$1$2$getstring$#");
}


void StoreName(char *username)
{
	struct UsersData  u;
	struct User user;
	FILE *file = fopen("store.bin", "rb+");
	fseek(file, 4096, SEEK_SET);
	fread(&u, sizeof(struct UsersData), 1, file);
	strcpy(user.name, username);
	user.offset = 0;
	u.values[u.userscount++] = user;
	CURRENT_USER = u.userscount - 1;
	fseek(file, 4096, SEEK_SET);
	fwrite(&u, sizeof(struct UsersData), 1, file);
	fclose(file);
}


int CheckIfUsersExist(char *username)
{
	struct UsersData u;
	struct User user;
	FILE *file = fopen("store.bin", "rb+");
	fseek(file, 4096, SEEK_SET);
	fread(&u, sizeof(struct UsersData), 1, file);
	for (int i = 0; i < u.userscount; i++)
	{
		user = u.values[i];
		if (!strcmp(user.name, username))
		{
			CURRENT_USER = i;
			fclose(file);
			return 1;
		}
	}
	fclose(file);
	return 0;
}

void ProcessMainScreen(char *command)
{
	int i=1;
	int offset = 0;
	for (; command[i] != '$'; i++);
	char option[10];
	i++;
	for (; command[i] != '$'; i++)
		option[offset++] = command[i];
	option[offset] = '\0';
	i++;
	char name[32];
	offset = 0;
	for (; command[i] != '$'; i++)
		name[offset++] = command[i];
	name[offset] = '\0';
	int choice = atoi(option);
	printf("%d\t%s\n", choice, name);
	if (choice == 1)
	{
		StoreName(name);
	}
	else if (choice == 2)
	{
		int k=CheckIfUsersExist(name);
		if (k == 0)
		{
			strcpy(command, "InValid UserName #");
			return;
		}
	}
	strcpy(command, "@userscreen$\n\n\t1.View Meeting Requests\n\t2.Make Meeting Request\n$1$2$$#");
}

int USERSCOUNT;

void ProcessMakeRequest(char *command)
{
	
	FILE *file = fopen("store.bin", "rb+");
	int i = USERSCOUNT;
	struct UsersData u;
	fseek(file, 4096, SEEK_SET);
	fread(&u, sizeof(struct UsersData), 1, file);
	int count = 0;
	printf("\nuserscount=%d\n", u.userscount);
	if (u.userscount <= 1)
	{
		strcpy(command, "No Users or Only One User#");
		printf("%s", command);
		return;
	}
	strcpy(command, "@makerequest$\n\t");
	for (; i < u.userscount&&count<10; i++)
	{
		if (i != CURRENT_USER)
		{
			count++;
			char temp[5];
			itoa(count, temp, 10);
			strcat(command, temp);
			strcat(command, ".");
			strcat(command, u.values[i].name);
			strcat(command, "\n\t");
		}
	}
	if (i < u.userscount)
	{
		strcat(command, "\n\tEnter 11 to goto next page\n\t");
	}
	strcat(command, "$");
	char temp[5];
	itoa(1, temp, 10);
	strcat(command, temp);
	itoa(11, temp, 10);
	strcat(command, "$");
	strcat(command, temp);
	strcat(command, "$");
	strcat(command, "$#");
	fclose(file);
}

int REQUEST_USER_NUMBER;

void AddRequest(char *command)
{
	int i;
	for (i = 1; command[i] != '$'; i++);
	i++;
	char date[15];
	int offset = 0;
	for (; command[i] != '$'; i++)
		date[offset++] = command[i];
	date[offset] = '\0';
	struct UsersData u;
	FILE *file = fopen("store.bin", "rb+");
	fseek(file, 4096, SEEK_SET);
	fread(&u, sizeof(struct UsersData), 1, file);
	struct User cur_user, req_user;
	cur_user = u.values[CURRENT_USER];
	req_user = u.values[REQUEST_USER_NUMBER];
	struct Meeting meet;
	strcpy(meet.name, req_user.name);
	strcpy(meet.Date, date);
	int storeoffset = GiveFreeBlock();
	offset = cur_user.offset;
	if (offset > 0)
	{
		meet.next = offset;
	}
	else{
		meet.next = 0;
	}
	cur_user.offset = storeoffset;
	u.values[CURRENT_USER] = cur_user;
	fseek(file, 4096, SEEK_SET);
	fwrite(&u, sizeof(struct UsersData), 1, file);
	fseek(file, storeoffset, SEEK_SET);
	fwrite(&meet, sizeof(struct Meeting), 1, file);
	fclose(file);
	strcpy(command,"Meet Request Stores Successfully#");
	printf("%d", storeoffset);
}


void ProcessAddRequest(char *command)
{
	char option[32];
	int i;
	for (i = 1; command[i] != '$'; i++);
	int offset = 0;
	i++;
	for (; command[i] != '$'; i++)
		option[offset++] = command[i];
	option[offset] = '\0';
	int choice = atoi(option);
	printf("\noption=%s\tchoice=%d\n", option, choice);
	if (choice == 11)
	{
		USERSCOUNT += 10;
		ProcessMakeRequest(command);
	}
	else{
		REQUEST_USER_NUMBER = USERSCOUNT + choice-1;
		if (CURRENT_USER >= USERSCOUNT)
		{
			if (REQUEST_USER_NUMBER >= CURRENT_USER)
				REQUEST_USER_NUMBER++;
		}
		//printf("Final Request user=%d\n", REQUEST_USER_NUMBER);
		strcpy(command, "%addrequest$\n\tEnter Date\n\tFormat : dd-mm-yyyy\n\t$#");
	}
}

void ViewRequests(char *command)
{
	struct UsersData u;
	FILE *file = fopen("store.bin", "rb+");
	fseek(file, 4096, SEEK_SET);
	fread(&u, sizeof(struct UsersData), 1, file);
	int offset = u.values[CURRENT_USER].offset;
	struct Meeting meet;
	strcpy(command, "\n\tMeeting Requests\n\n\t");
	while (offset > 0)
	{
		fseek(file, offset, SEEK_SET);
		fread(&meet, sizeof(struct Meeting), 1, file);
		strcat(command, meet.name);
		strcat(command, "\t");
		strcat(command, meet.Date);
		strcat(command, "\n\t");
		offset = meet.next;
	}
	strcat(command, "#");
}


void ProcessUsersScreen(char *command)
{
	char option[32];
	int i;
	for (i = 1; command[i] != '$'; i++);
	i++;
	int offset = 0;
	for (; command[i] != '$'; i++)
		option[offset++] = command[i];
	option[offset] = '\0';
	int choice = atoi(option);
	if (choice == 1)
	{
		USERSCOUNT = 0;
		ViewRequests(command);
	}
	else if (choice == 2)
	{
		USERSCOUNT = 0;
		ProcessMakeRequest(command);
	}
}


int ProcessRequest(char *command)
{
	//printf("Command:%s\n", command);
	char buffer[1024];
	int i;
	for (i = 1; command[i] != '$'; i++)
		buffer[i - 1] = command[i];
	buffer[i - 1] = '\0';
	if (!strcmp("opened", buffer))
	{
		return 1;
	}
	else if (!strcmp("mainscreen", buffer))
	{
		return 2;
	}
	else if (!strcmp("userscreen", buffer))
	{
		return 3;
	}
	else if (!strcmp("makerequest", buffer))
	{
		return 4;
	}
	else if (!strcmp("addrequest", buffer))
	{
		return 5;
	}
	return 0;
}
void process_input(char *recvbuf, int recv_buf_cnt, int* csock) 
{

	char replybuf[1024]={'\0'};
	int k=ProcessRequest(recvbuf);
	if (k == 1)
	{
		SetMainScreen(recvbuf);
	}
	else if (k == 2)
	{
		printf("Main Screen Function Called\n");
		ProcessMainScreen(recvbuf);
	}
	else if (k == 3)
	{
		printf("User Screen Under Processing %s\n",recvbuf);
		ProcessUsersScreen(recvbuf);
		//strcpy(recvbuf, "User Screen Under Construction\n#");
	}
	else if (k == 4)
	{
		printf("Make Request is Called %s",recvbuf);
		ProcessAddRequest(recvbuf);
	}
	else if (k == 5)
	{
		printf("Add Date is called");
		AddRequest(recvbuf);
	}
	replyto_client(recvbuf, csock);
	replybuf[0] = '\0';
}

void replyto_client(char *buf, int *csock) {
	int bytecount;
	
	if((bytecount = send(*csock, buf, strlen(buf), 0))==SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free (csock);
	}
	//printf("replied to client: %s\n",buf);
}

DWORD WINAPI SocketHandler(void* lp){
    int *csock = (int*)lp;

	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;

	memset(recvbuf, 0, recvbuf_len);
	if((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0))==SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free (csock);
		return 0;
	}

	//printf("Received bytes %d\nReceived string \"%s\"\n", recv_byte_cnt, recvbuf);
	process_input(recvbuf, recv_byte_cnt, csock);

    return 0;
}