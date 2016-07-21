// socket_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

void socket_client();
int checkDateIsValid(char *date);
int _tmain(int argc, _TCHAR* argv[])
{
	socket_client();
	//printf("%d\n",checkDateIsValid("31-05-2541")); 
	return 0;
}

