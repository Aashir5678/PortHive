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

	if (flask_client_fd == -1)
	{
		printf("Couldn't create flask client fd\n");
		print_err(errno);
		return -1;
	}

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

	fcntl(flask_client_fd, F_SETFL, O_NONBLOCK);

	return flask_client_fd;

}

int send_current_time_flask(int fd)
{
	u64 t1 = get_time_ms();
	char content_str[30];
	sprintf(content_str, "{\"time\":%lu}\r\n", t1);
	int content_len = strlen(content_str);
	char payload[250];

	sprintf(payload, "POST /latency HTTP/1.1\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "Connection: keep-alive\r\n"
                "\r\n"
                "{\"time\":%lu}\r\n", content_len, t1);

	printf("%s\n", payload);
	if (send_str_flask(fd, payload) == -1)
	{
		return -1;
	}

	return 1;
}

int send_client_disconnect_flask(int fd, char* ipv4, u64 time_connected_at)
{
	return -1;
}

int send_client_join_flask(int fd, char* ipv4)
{
	return -1;
}


int send_str_flask(int fd, char* msg)
{
	int bytes_sent = 0;

	while (bytes_sent < strlen(msg) + 1)
	{
		bytes_sent = send(fd, msg, strlen(msg) + 1 - bytes_sent, 0);

		if (bytes_sent < 0)
		{
			return -1;
		}
	}

	return bytes_sent;
}
