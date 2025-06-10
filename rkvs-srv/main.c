#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#define LISTENADDR "127.0.0.1"

/* global */
char *error;

/* server init  
 * returns 0 on error, or returns a socket fd
 * */
int
srv_init(int portno)
{
	int s;
	struct sockaddr_in srv;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		error = "socket() error\n";
		return 0;
	}

	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = inet_addr(LISTENADDR);
	srv.sin_port = htons(portno);

	
	if (bind(s, (struct sockaddr *)&srv, sizeof(srv)) > 0) {
		close(s);
		error = "bind() error\n";
		return 0;
	}

	if (listen(s, 5)) {
		close(s);
		error = "listen() error\n";
		return 0;
	}

	return s;
}

/* client accept
 * returns 0 on error, or returns new client socket fd
 * */
int
cli_accept(int s) 
{
	int c;
	socklen_t addrlen;
	struct sockaddr_in cli;

	addrlen = 0;
	memset(&cli, 0, sizeof(cli));
	c = accept(s, (struct sockaddr *)&cli, &addrlen);
	if (c < 0) {
		error = "accept() error\n";
		return 0;
	}

	return c;
}

char *
cli_read(int c) 
{
	(void)c;
	return NULL;
}

void
cli_conn(int s, int c)
{
	(void)s;

	char rbuf[64];
	ssize_t n = read(c, rbuf, sizeof(rbuf)-1);
	if (n < 0) {
		fprintf(stderr, "read() error\n");
		return;
	}

	printf("Client sent: '%s'\n", rbuf);
	char wbuf[64] = "world";
	n = write(c, wbuf, strlen(wbuf));
	if (n < 0) {
		fprintf(stderr, "write() error\n");
		return;
	}

}

int
main(int argc, char *argv[]) 
{
	int s, c;
	char *port;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <listening port>\n", argv[0]);
		return -1; 
	} else {
		port = argv[1];
	}

	s = srv_init(atoi(port));
	if (!s) {
		fprintf(stderr, "%s\n", error);
		return -1;
	}

	printf("Listening on %s:%s\n", LISTENADDR, port);

	while (1) {
		c = cli_accept(s);
		if (!c) {
			fprintf(stderr, "%s\n", error);
			continue;
		}

		// fork
		printf("Incoming connection\n");
		if (!fork()) {
			cli_conn(s, c);
		}

	}
	
	return -1;	
}

