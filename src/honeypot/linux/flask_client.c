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
#include <curl/curl.h>
#include "server.h"


// Prevents printing to stdout when response is received
size_t silent_callback(char *ptr, size_t size, size_t nmemb, void *userdata) 
{
    return size * nmemb;
}

int connect_to_flask_backend(char* ipv4)
{

	int flask_client_fd = -1;


	CURL *curl;
	CURLcode res;
	curl_socket_t fd;

	// 7 bytes for http://, 12 bytes for ip, 3 for dots, 1 for : and 5 for port call it 32 to be safe
	char flask_website_url[32];

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (curl)
	{
		snprintf(flask_website_url, sizeof(flask_website_url), "http://%s:%d", ipv4, FLASK_PORT);
		curl_easy_setopt(curl, CURLOPT_URL, flask_website_url);
		curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L); // Set to raw socket mode

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, silent_callback);
		res = curl_easy_perform(curl);

		if (res == CURLE_OK)
		{
			res = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &fd);

			if (!res && fd != CURL_SOCKET_BAD)
			{
				flask_client_fd = (int) fd;

				return flask_client_fd;
			}
		}

		else
		{
			curl_easy_cleanup(curl);

		}
	}

	return -1;

}


int send_http_post(char* ipv4, char* endpoint, char* data)
{

	// Convert ip, port and endpoint to website address
	char endpoint_url[64];
	snprintf(endpoint_url, sizeof(endpoint_url), "http://%s:%d/%s", ipv4, FLASK_PORT, endpoint);
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, endpoint_url);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, silent_callback);

	if (curl_easy_perform(curl) != CURLE_OK)
	{
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return -1;
	}

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return 1;
}


int send_http_get(char* ipv4, char* endpoint)
{
	return -1;
}


// Send an HTTP post to the endpoint and wait for a response from the server
int send_http_post_res(char* ipv4, char* endpoint, char* data)
{
	// Convert ip, port and endpoint to website address
	char endpoint_url[64];
	snprintf(endpoint_url, sizeof(endpoint_url), "http://%s:%d/%s", ipv4, FLASK_PORT, endpoint);
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, endpoint_url);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, silent_callback);
	CURLcode res = curl_easy_perform(curl);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	if (res == CURLE_OK)
	{
		return 1;
	}

	else
	{
		return -1;
	}
}


int send_latency_flask(char* ipv4, int fd)
{
	u64 t1 = get_time_ms();

	char content_str[32];
	snprintf(content_str, sizeof(content_str), "{\"time\":%lu}\r\n", t1);
	int res1 = send_http_post_res(ipv4, "latency", content_str);

	u64 t2 = get_time_ms();
	snprintf(content_str, sizeof(content_str), "{\"time\":%lu}\r\n", t2);
	int res2 = send_http_post_res(ipv4, "latency", content_str);

	return res1 && res2;
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
