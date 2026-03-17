#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <time.h>
#include "server.h"


#define MAX_BACKLOG 2
#define CHUNK_SIZE 8 // max bytes at a time when reading from the client




struct sockaddr_in get_sockaddr_in(const char* ipv4_str, u32 port)
{

	struct in_addr host_ip;

	if (inet_pton(AF_INET, ipv4_str, &host_ip) == -1)
	{
		printf("Couldn't resolve IPV4 %s\n", ipv4_str);
		exit(EXIT_FAILURE);
	}

	u32 server_fd;
	struct sockaddr_in server_addr;

	struct in_addr sin_addr;
	sin_addr.s_addr = (u32) host_ip.s_addr;
	memset(&server_addr, 0, sizeof(server_addr)); // Initialize server addr as empty

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port); // htons converts 16-bit (2 byte) port to network byte order (big endian)
	server_addr.sin_addr = sin_addr;

	return server_addr;
}



char* be_to_ipv4_str(char ip[]) // Convert BE (big-endian) network byte order ip to ipv4 dotted string
{
	return inet_ntoa(*(struct in_addr *)ip);
}

char* get_client_ip_str(client cli) // Convert 32 bit BE ip from client to string
{
	struct in_addr client_ip;
	client_ip.s_addr = cli.ipv4;

	return inet_ntoa(client_ip);
	
}
void print_err(int err)
{
	printf("Error [%d]: %s\n", err, strerror(err));
}

void close_conn(int fd)
{
	int client_closed = close(fd);

	if (client_closed < 0)
	{
		print_err(errno);
	}
}


u32 bind_and_listen_on_port(const char* ipv4_str, u32 port)
{
	u32 server_fd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(server_fd, F_SETFL, O_NONBLOCK);

	struct sockaddr_in server_addr = get_sockaddr_in(ipv4_str, port);

	if (server_fd == -1)
	{
		print_err(errno);
		return -1;
	}


	if (bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
	{
		char ip_str[255];
		inet_ntop(AF_INET, &server_addr.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);

		print_err(errno);
		printf("%s\n", ip_str);


		return -1;
	}

	if (listen(server_fd, MAX_BACKLOG) == -1)
	{
		print_err(errno);
		return -1;
	}

	return server_fd;
}


int read_client_fd(int client_fd, char* buf)
{
	// Returns total bytes read by client_fd and fills buf, if error return -1

	ssize_t bytes_read = 0;
	u32 total_bytes_read = 0;

	while (1)
	{
		bytes_read = read(client_fd, buf + total_bytes_read, CHUNK_SIZE); // only read first CHUNK_SIZE bytes

		if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
		{
			print_err(errno);

			total_bytes_read = -1;
			break;
		}


		// Keep reading data until EAGAIN
		else if (bytes_read > 0)
		{
			
			total_bytes_read += bytes_read;
			if (total_bytes_read + CHUNK_SIZE > MAX_MSG_SIZE)
			{
				break;
			}


		}


		else if (errno == EAGAIN || errno == EWOULDBLOCK || bytes_read == 0)
		{
			break;
		}


	}


	if (total_bytes_read > 0) // new data was read
	{
		memset(buf + total_bytes_read, '\0', 1);
		// printf("%s\n", msg_buf);
	}

	else // fd had empty data means client disconnected
	{
		return -1;
	}

	return total_bytes_read;
	
}


u64 get_time_ms()
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);

	return ((u64) ts.tv_sec * 1000) + ((u64) ts.tv_nsec / 1000000);

}


// Linear search through clients and find by client_fd
client* find_client_by_fd(u32 client_fd, client* clients, u32 clients_connected)
{
	for (int i=0; i < clients_connected; i++)
	{
		if (clients[i].fd ==  client_fd)
		{
			return clients + i;
		}
	}

	return NULL;
}


client* find_client_by_ip(u32 ipv4, client* clients, u32 clients_connected)
{
	for (int i=0; i < clients_connected; i++)
	{
		if (clients[i].ipv4 ==  ipv4)
		{
			return clients + i;
		}
	}

	return NULL;
} 

void print_client(client cli)
{
	printf("IPV4: %s, Time connected: %ld ms\n", get_client_ip_str(cli), get_time_ms() - cli.curr_time_ms);
}

u32 accept_new_clients(u32 server_fd, int epollfd, client* cli_buf, u32 clients_connected)
{
	struct sockaddr client_addr;
	u32 client_addr_size = sizeof(client_addr);
	int client_fd;
	struct epoll_event event;

	u32 new_clients_joined = 0;

	while (1)
	{
		client_fd = accept(server_fd, &client_addr, &client_addr_size);

		if (client_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			break;
		}

		else if (client_fd == -1)
		{
			print_err(errno);
			break;
		}

		else if (client_fd > 0 && clients_connected < MAX_CONNS)
		{
			new_clients_joined++;
			fcntl(client_fd, F_SETFL, O_NONBLOCK);

			event.events = EPOLLIN | EPOLLET; // listen for read / write on client fd
			event.data.fd = client_fd;

			if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &event) == -1)
			{
				print_err(errno);
				return -1;
			}

			struct in_addr ip_be;
			if (inet_pton(AF_INET, be_to_ipv4_str(client_addr.sa_data), &ip_be) == -1)
			{
				printf("Couldn't resolve client ip %s\n", be_to_ipv4_str(client_addr.sa_data));
				return -1;
			}

			client new_client = {.fd=client_fd, .ipv4=(u32) ip_be.s_addr, .curr_time_ms=get_time_ms()};
			*cli_buf = new_client;

			// print_client(*cli_buf);
			
			printf("New client joined: %s, # of clients: %d\n", be_to_ipv4_str(client_addr.sa_data), clients_connected + new_clients_joined);
			

		}

		else if (clients_connected >= MAX_CONNS)
		{
			printf("Refused client %s, too many connections\n", be_to_ipv4_str(client_addr.sa_data));
			close_conn(client_fd);
		}
	}

	return clients_connected + new_clients_joined;
}






