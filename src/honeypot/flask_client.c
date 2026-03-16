#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "flask_client.h"
#include "server.h"


int connect_to_flask_backend(char* ipv4)
{
	int flask_client_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in flask_addr;
	flask_addr.sin_family = AF_INET;
	flask_addr.sin_port = htons(FLASK_PORT);
	flask_addr.sin_addr.s_addr = inet_addr(ipv4);

	if (connect(flask_client_fd, (struct sockaddr*)&flask_addr, sizeof(flask_addr)) == -1)
	{
		printf("Couldn't connect to flask backend\n");
		print_err(errno);
		return -1;
	}



	return flask_client_fd;

}

int send_client_msg(int fd, char* msg)
{

	return -1;
}

int send_client_disconnect(int fd, char* ipv4, u64 time_connected_at)
{
	return -1;
}

int send_client_join(int fd, char* ipv4)
{
	return -1;
}