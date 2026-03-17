#include "server.h"
#include "flask_client.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTS {80, 443, 81}


void run_server_on_addr(const char* ipv4, char* flask_ipv4, u32 port)
{
	u32 server_fd = bind_and_listen_on_port(ipv4, port);

	if (server_fd == -1)
	{
		exit(EXIT_FAILURE);
	}

	

	printf("Running server on %s on port %d\n", ipv4, (int) port);

	bool server_running = true;
	int client_fd;
	struct sockaddr client_addr;
	u32 client_addr_size = sizeof(client_addr);


	// Epoll for clients and poll for flask fd

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

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &event) == -1) 
	{
		print_err(errno);
		exit(EXIT_FAILURE);
	}

	int flask_fd = connect_to_flask_backend(flask_ipv4);

	if (flask_fd > 0)
	{
		printf("Connected to flask backend\n");
	}

	else
	{
		exit(EXIT_FAILURE);
	}


	event.events = EPOLLIN;
	event.data.fd = flask_fd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, flask_fd, &event) == -1)
	{
		print_err(errno);
		exit(EXIT_FAILURE);
	}

	client clients[MAX_CONNS];


	u32 clients_connected = 0;
	int new_fds = 0;
	struct epoll_event curr_poll;

	char msg_buf[MAX_MSG_SIZE]; // For client messages
	char flask_msg_buf[MAX_MSG_SIZE]; // For flask response

	int bytes_read;


	if (send_current_time_flask(flask_fd) == -1)
	{
		printf("couldn't send time\n");
	}

	else
	{
		printf("sent time\n");
	}



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
				clients_connected = accept_new_clients(server_fd, epollfd, clients + (clients_connected), clients_connected);

				if (clients_connected == -1)
				{
					exit(EXIT_FAILURE);
				}

			}

			else if (curr_poll.data.fd == flask_fd) // Flask webserver sent a message to honeypot
			{
				if (curr_poll.events == EPOLLIN) 
				{
					read_client_fd(flask_fd, flask_msg_buf);

					if (strstr(flask_msg_buf, "LATENCY") != NULL) // If HTTP response contains "LATENCY"
					{
						if (send_current_time_flask(flask_fd) == -1)
						{
							printf("couldn't send time\n");
						}

						else
						{
							printf("sent time\n");
						}
					}
				}
			}

			else
				{
					if (curr_poll.events == EPOLLIN) // Client sent data
					{
						client *cli = find_client_by_fd(curr_poll.data.fd, clients, clients_connected);
						bytes_read = read_client_fd(curr_poll.data.fd, msg_buf);
						if (bytes_read == -1)
						{
							if (cli != NULL)
							{

								printf("Disconnected client %s\n", get_client_ip_str(*cli));
							}

							clients_connected--;
							close_conn(curr_poll.data.fd);
							curr_poll.data.fd = -1;
							
						}

						else
						{
							printf("%s\n", msg_buf);
							if (cli == NULL)
							{
								printf("Internal error, client fd %d does not exist\n", curr_poll.data.fd);
								exit(EXIT_FAILURE);
							}

							printf("time connected for client in ms: %ld\n", get_time_ms() - cli->curr_time_ms);

							memset(msg_buf, 0, bytes_read + 1); // + 1 for string terminator
						}	
					}

			}
		}


	}


	if (close(flask_fd) == -1)
	{
		print_err(errno);
		exit(EXIT_FAILURE);
	}

	else
	{
		printf("Flask connection closed\n");
	}

	if (close(server_fd) == -1)
	{
		print_err(errno);
		exit(EXIT_FAILURE);
	}

	else
	{
		printf("Server closed\n");
		exit(EXIT_SUCCESS);
	}

}



int send_syn_signal(int fd, char* ipv4, u64 time_sent_at)
{

	return -1;
}

char* get_local_ip() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1557);
    addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    
    connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    
    struct sockaddr_in local;
    socklen_t len = sizeof(local);
    getsockname(fd, (struct sockaddr*)&local, &len);
    
    close(fd);
    return inet_ntoa(local.sin_addr);
}



int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Usage: ./honeypot {webserver-ip}\n");
		exit(EXIT_FAILURE);
	}

	char* flask_ipv4 = argv[1];
	char* ipv4 = get_local_ip();
	int ports[] = PORTS;

	run_server_on_addr(ipv4, flask_ipv4, ports[0]);



	// Spin up multiple threads calling run_server_on_port
	// Do packet sniffing here in main for SYN detection
}