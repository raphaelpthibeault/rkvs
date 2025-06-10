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

#include <common/proto.h>

#define LISTENADDR "127.0.0.1"

/* global */
char *error;

/* server init  
 * returns 0 on error, or returns a socket fd
 * */
static int
srv_init(int portno)
{
	int s;
	struct sockaddr_in srv;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		error = "socket() error\n";
		return 0;
	}

	int val = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

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
static int
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

static int32_t
request_service(int s, int c)
{
	(void)s;

	char rbuf[4+MAX_MSG_SIZE];
	int32_t err;

	err = read_n(c, rbuf, 4);
	if (err < 0) {
		fprintf(stderr, "read_n error\n");
		return -1;
	}

	uint32_t len;
	// assume little endian
	memcpy(&len, &rbuf, 4);
	if (len > MAX_MSG_SIZE) {
		fprintf(stderr, "Message too big\n");
		return -1;
	}

	err = read_n(c, &rbuf[4], len);
	if (err < 0) {
		fprintf(stderr, "read_n error\n");
		return -1;
	}
	rbuf[4+len] = '\0';
	
	printf("Client sent: '%s'\n", &rbuf[4]);

	// reply using the same stub protocol
	char msg[] = "world";
	char wbuf[4 + sizeof(msg)];
	len = (uint32_t)strlen(msg);
	memcpy(wbuf, &len, 4);
	memcpy(&wbuf[4], msg, len);

	err = write_n(c, wbuf, 4 + len);
	if (err < 0) {
		fprintf(stderr, "write_n error\n");
		return -1;
	}

	return 0;
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

		printf("Incoming connection\n");
		if (fork() > 0) {
			/* one forked thread services all requests of one client connection */	
			int32_t err;
			while (1) {
				err = request_service(s, c);
				if (err) {
					break;
				}
			}

			close(c);
		}
	}
	
	return -1;	
}

