
#ifndef FLASK_CLIENT_H
#define FLASK_CLIENT_H

#define FLASK_PORT 5000

typedef int32_t i32;
typedef int64_t i64;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;

int connect_to_flask_backend(char* ipv4);
int send_str_flask(int fd, char* msg);
int send_client_disconnect_flask(int fd, char* ipv4, u64 time_connected_at);
int send_client_join_flask(int fd, char* ipv4);
int send_current_time_flask(char* ipv4, int fd);

int send_http_post(char* ipv4, char* endpoint, char* data);
int send_http_get(char* ipv4, char* endpoint);
int send_latency_flask(char* ipv4, int fd);
int send_http_post_res(char* ipv4, char* endpoint, char* data);


#endif