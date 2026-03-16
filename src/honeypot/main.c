#include "server.h"
#include "flask_client.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdbool.h>
#include <sys/epoll.h>


#define PORTS {80, 443, 81}


void run_server_on_addr(const char* ipv4, u32 port)
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

	client clients[MAX_CONNS];


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
				clients_connected = accept_new_clients(server_fd, epollfd, clients + (clients_connected), clients_connected);

				if (clients_connected == -1)
				{
					exit(EXIT_FAILURE);
				}

			}

			else
				{
					if (curr_poll.events == EPOLLIN) // Client sent data
					{
						client *cli = find_client_by_fd(curr_poll.data.fd, clients, clients_connected);

						if (read_client_fd(curr_poll.data.fd) == -1)
						{
							if (cli != NULL)
							{


								// struct in_addr disconnected_client_ip;
								// disconnected_client_ip.s_addr = cli->ipv4;
								printf("Disconnected client %s\n", get_client_ip_str(*cli));
							}

							clients_connected--;
							close_conn(curr_poll.data.fd);
							curr_poll.data.fd = -1;
							
						}

						else
						{
							if (cli == NULL)
							{
								printf("Internal error, client fd %d does not exist\n", curr_poll.data.fd);
								exit(EXIT_FAILURE);
							}

							printf("time connected for client in ms: %ld\n", get_time_ms() - cli->curr_time_ms);
						}	
					}

			}
		}
		

	}

	printf("closing");


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



int main()
{
	char* ipv4 = get_local_ip();
	printf("Host ip: %s\n", ipv4);
	int ports[] = PORTS;

	run_server_on_addr(ipv4, ports[0]);



	// Spin up multiple threads calling run_server_on_port
	// Do packet sniffing here in main for SYN detection
}