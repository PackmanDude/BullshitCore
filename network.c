#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "global_macros.h"
#include "network.h"

#define MINECRAFT_VERSION "1.20.4"
#define PROTOCOL_VERSION 765
#define PERROR_AND_GOTO_CLOSEFD(s, ctx) { perror(s); goto ctx ## closefd; }

// config
#define ADDRESS "0.0.0.0"
#define PORT 25565
#define MAX_PLAYERS 15

void *main_routine(void *p_player)
{
	int player = *(int *)p_player;
	free(p_player);
	return NULL;
}

// TODO: Add support for IPv6
int main(void)
{
	int server_endpoint = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (unlikely(server_endpoint == -1)) PERROR_AND_EXIT("socket")
	if (unlikely(setsockopt(server_endpoint, IPPROTO_TCP, TCP_NODELAY, &(const int){ 1 }, sizeof(int)) == -1))
		PERROR_AND_GOTO_CLOSEFD("setsockopt", server)
	{
		struct in_addr address;
		{
			int ret = inet_pton(AF_INET, ADDRESS, &address);
			if (!ret)
			{
				fputs("An invalid server address was specified.\n", stderr);
				return EXIT_FAILURE;
			}
			else if (unlikely(ret == -1))
				PERROR_AND_GOTO_CLOSEFD("inet_pton", server)
		}
		const struct sockaddr_in server_address = { AF_INET, htons(PORT), address };
		struct sockaddr server_address_data;
		memcpy(&server_address_data, &server_address, sizeof server_address_data);
		if (unlikely(bind(server_endpoint, &server_address_data, sizeof server_address_data) == -1))
			PERROR_AND_GOTO_CLOSEFD("bind", server)
	}
	if (unlikely(listen(server_endpoint, SOMAXCONN) == -1))
		PERROR_AND_GOTO_CLOSEFD("listen", server)
	{
		int client_endpoint;
		{
			int_least16_t thread_counter = 0;
			while (true)
			{
				client_endpoint = accept(server_endpoint, NULL, NULL);
				if (unlikely(client_endpoint == -1))
					PERROR_AND_GOTO_CLOSEFD("accept", server)
				if (thread_counter == INT_LEAST16_MAX)
					if (unlikely(close(client_endpoint) == -1))
						PERROR_AND_GOTO_CLOSEFD("close", server)
				int * const p_client_endpoint = malloc(sizeof client_endpoint);
				if (unlikely(!p_client_endpoint))
					PERROR_AND_GOTO_CLOSEFD("malloc", client)
				*p_client_endpoint = client_endpoint;
				pthread_t thread;
				pthread_create(&thread, NULL, main_routine, p_client_endpoint);
				++thread_counter;
			}
		}
		if (unlikely(close(client_endpoint) == -1))
			PERROR_AND_GOTO_CLOSEFD("close", server)
		if (unlikely(close(server_endpoint) == -1)) PERROR_AND_EXIT("close")
		return EXIT_SUCCESS;
clientclosefd:
		if (unlikely(close(client_endpoint) == -1)) perror("close");
	}
serverclosefd:
	if (unlikely(close(server_endpoint) == -1)) perror("close");
	return EXIT_FAILURE;
}