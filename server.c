#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#define PORT 5050
#define MAX_BACKLOG 3
#define MAX_RECV_SIZE 200 // Must be less than max uint16 value (65k)
#define CHUNK_SIZE 8 // Read for bytes at a time from the client

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


u32 bind_and_listen_on_port(u32 port)
{
	u32 server_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr = get_sockaddr_in(port);

	if (server_fd == -1)
	{
		printf("Failed to create server socket\n");
		printf("%s\n", strerror(errno));
		return -1;
	}

	// fcntl(server_fd, F_SETFL, O_NONBLOCK); // file control, set socket fd status to non blocking

	if (bind(server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
	{
		printf("Failed to bind\n");
		printf("%x\n", server_addr.sin_addr.s_addr);
		printf("%d %s\n", errno, strerror(errno));
		return -1;
	}

	if (listen(server_fd, MAX_BACKLOG) == -1)
	{
		printf("Failed to listen on port\n");
		printf("%s\n", strerror(errno));
		return -1;
	}

	return server_fd;
}


void handle_client(u32 server_fd, u32 client_fd, struct sockaddr client_addr)
{

	bool client_connected = true;
	ssize_t bytes_read = 0;

	char* msg_buf = malloc(CHUNK_SIZE);
	u16 capacity = CHUNK_SIZE;
	u16 total_bytes_read = 0;

	while (client_connected)
	{
		bytes_read = read(client_fd, msg_buf + total_bytes_read, CHUNK_SIZE); // only read first CHUNK_SIZE bytes

		if (bytes_read == -1 && errno != EAGAIN)
		{
			// free(msg_buf);
			printf("Error [%d]: %s\n", errno, strerror(errno));
			client_connected = false;
			break;
		}

		else if (bytes_read == 0)
		{
			client_connected = false;
			break;
		}


		else if (bytes_read > 0)
		{
			
			total_bytes_read += bytes_read;
			// Dynamically grow msg_buf to read all data from stream

			if (total_bytes_read + CHUNK_SIZE > capacity)
			{
				capacity *= 2;
				msg_buf = realloc(msg_buf, capacity);
				
			}

			// Start reading from pointer where new message started
			printf("%s\n", msg_buf + total_bytes_read - bytes_read);

		}


		// memset(msg_buf, 0, total_bytes_read);


	}


	free(msg_buf);


	int client_closed = close(client_fd);

	if (client_closed < 0)
	{
		printf("Error [%d]: %s\n", errno, strerror(errno));
	}
}	


int main()
{


	// one hex digit = 4 bits = half a byte
	// 32 bits = 4 bytes = 8 hex digits

	// 127.0.0.1 = 0x7F 0x00 0x00 0x01 in hex aka localhost

	u32 server_fd = bind_and_listen_on_port(PORT);

	if (server_fd == -1)
	{
		return -1;
	}

	

	printf("Running server on localhost\n");

	bool server_running = true;
	int client_fd;
	struct sockaddr client_addr;
	u32 client_addr_size = sizeof(client_addr);	

	while (server_running)
	{
		client_fd = accept(server_fd, &client_addr, &client_addr_size);

		if (client_fd == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				continue;
			}

			printf("%s\n", strerror(errno));
			continue;
		}

		else
		{
			fcntl(client_fd, F_SETFL, O_NONBLOCK);
			printf("New client joined\n");
			printf("%s\n", be_to_ipv4_str(client_addr.sa_data));
			handle_client(server_fd, client_fd, client_addr);


		}

	}


	if (close(server_fd) == -1)
	{
		printf("%s\n", strerror(errno));
	}

	else
	{
		printf("Server closed\n");
	}





}
