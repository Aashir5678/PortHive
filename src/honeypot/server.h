
#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <netinet/in.h>

#define MAX_CONNS 10
#define MAX_MSG_SIZE 10 * 1000 // 10kb

typedef int32_t i32;
typedef int64_t i64;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;


typedef struct
{
	u32 fd;
	u32 ipv4; // stored in BE
	u64 curr_time_ms;



} client;


struct sockaddr_in get_sockaddr_in(const char* ipv4_str, u32 port);
char* be_to_ipv4_str(char ip[]);
char* get_client_ip_str(client cli);
void print_err(int err);
void close_conn(int fd);
u32 bind_and_listen_on_port(const char* ipv4_str, u32 port);
int read_client_fd(int client_fd, char* buf);
u64 get_time_ms();
client* find_client_by_ip(u32 ipv4, client* clients, u32 clients_connected);
client* find_client_by_fd(u32 client_fd, client* clients, u32 clients_connected);
u32 accept_new_clients(u32 server_fd, int epollfd, client* cli_buf, u32 clients_connected);
void run_server_on_port(const char* ipv4, u32 port);



#endif