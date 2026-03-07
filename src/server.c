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

#define PORT 5050
#define MAX_BACKLOG 2
#define MAX_RECV_SIZE 200 // Must be less than max uint16 value (65k)
#define CHUNK_SIZE 8 // Read for bytes at a time from the client
#define POLL_TIME 20 // Timeout in ms on poll blocking, set to -1 to block until client sends a message
#define MAX_CONNS 10

typedef int32_t i32;
typedef int64_t i64;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;


struct sockaddr_in get_sockaddr_in(u32 port)
{
	u32 local_host = 0x7F000001;

	u32 server_fd;
	struct sockaddr_in server_addr;

	struct in_addr sin_addr;
	sin_addr.s_addr = htonl(local_host); // convert localhost to 32 bit host address in network byte order (big endian)

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


u32 bind_and_listen_on_port(u32 port)
{
	u32 server_fd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(server_fd, F_SETFL, O_NONBLOCK);

	struct sockaddr_in server_addr = get_sockaddr_in(port);

	if (server_fd == -1)
	{
		print_err(errno);
		return -1;
	}


	if (bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
	{
		//inet_ntoa((struct *in_addr) server_addr.sin_addr.s_addr

		char ip_str[255];
		inet_ntop(AF_INET, &server_addr.sin_addr.s_addr, ip_str, INET_ADDRSTRLEN);

		print_err(errno);
		printf("%s\n", ip_str);


		return -1;
	}

	if (listen(server_fd, MAX_BACKLOG) == -1)
	{
		// printf("Failed to listen on port\n");
		// printf("%s\n", strerror(errno));
		print_err(errno);
		return -1;
	}

	return server_fd;
}


int read_client_fd(int client_fd)
{
	// Returns total bytes read by client_fd, if error return -1

	ssize_t bytes_read = 0;

	char* msg_buf = malloc(CHUNK_SIZE);
	u16 capacity = CHUNK_SIZE;
	u16 total_bytes_read = 0;

	while (1)
	{
		bytes_read = read(client_fd, msg_buf + total_bytes_read, CHUNK_SIZE); // only read first CHUNK_SIZE bytes

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
			// Dynamically grow msg_buf to read all data from stream

			if (total_bytes_read + CHUNK_SIZE > capacity)
			{
				capacity *= 2;
				msg_buf = realloc(msg_buf, capacity);
				
			}

			else
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
		printf("%s\n", msg_buf);
	}



	free(msg_buf);

	return total_bytes_read;


	
}	


int main(void)
{

	// 127.0.0.1 = 0x7F 0x00 0x00 0x01 in hex aka localhost

	u32 server_fd = bind_and_listen_on_port(PORT);

	if (server_fd == -1)
	{
		exit(EXIT_FAILURE);
	}

	

	printf("Running server on localhost\n");

	bool server_running = true;
	int client_fd;
	struct sockaddr client_addr;
	u32 client_addr_size = sizeof(client_addr);


	struct epoll_event event;
	struct epoll_event events[MAX_CONNS];

	int epollfd = epoll_create1(0);


	if (epollfd == -1)
	{
		print_err(errno);
		exit(EXIT_FAILURE);
	}

	event.events = EPOLLIN | EPOLLET; // Block until a new connection is made
	event.data.fd = server_fd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
		print_err(errno);
		exit(EXIT_FAILURE);
	}



	u32 clients_connected = 0;
	int new_fds = 0;
	struct epoll_event curr_poll;

	while (server_running)
	{
		new_fds = epoll_wait(epollfd, events, MAX_CONNS, -1); // Gets all fd's where something new happend (new conn or read / write from existing conn), -1 for no timeout
		if (new_fds == -1)
		{
			print_err(errno);
			exit(EXIT_FAILURE);
		}

		// Loop through all updates fd's
		for (int i=0; i < new_fds; i++)
		{
			curr_poll = events[i];

			if (curr_poll.data.fd == server_fd)
			{

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
						clients_connected++;
						fcntl(client_fd, F_SETFL, O_NONBLOCK);

						event.events = EPOLLIN | EPOLLET; // listen for read / write on client fd
						event.data.fd = client_fd;

						if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &event) == -1)
						{
							print_err(errno);
							exit(EXIT_FAILURE);
						}
						
						printf("New client joined: %s\n", be_to_ipv4_str(client_addr.sa_data));
						

					}

					else if (clients_connected >= MAX_CONNS)
					{
						printf("Refused client %s, too many connections\n", be_to_ipv4_str(client_addr.sa_data));
						close_conn(client_fd);
					}
				}

			}

			else
				{
					if (curr_poll.events == EPOLLIN) // Client sent data
					{
						if (read_client_fd(curr_poll.data.fd) == -1)
						{
							printf("Disconnected client\n");
							clients_connected--;
							curr_poll.data.fd = -1;
							close_conn(curr_poll.data.fd);
						}
					}


			}	
		}
		

	}

	printf("closing");


	if (close(server_fd) == -1)
	{
		print_err(errno);
	}

	else
	{
		printf("Server closed\n");
	}


}
