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
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

#include <poll.h>
#include <fcntl.h>

#include <common/proto.h>
#include <common/arr.h>

#define LISTENADDR "127.0.0.1"
#define MAX_CONNECTIONS 64

struct connection {
	int fd;
	
	bool read;
	bool write;
	bool close;

	uint8_t in[IO_MAX];
	uint8_t out[IO_MAX];
	size_t in_idx;
	size_t out_idx;
};


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

static void
fd_set_nb(int fd) 
{
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

static int
handle_accept(struct connection *conn, int s)
{
	int c;
	socklen_t addrlen;
	struct sockaddr_in cli;

	addrlen = 0;
	memset(&cli, 0, sizeof(cli));
	c = accept(s, (struct sockaddr *)&cli, &addrlen);
	if (c < 0) {
		fprintf(stderr, "accept() error\n");
		return -1;
	}

	fd_set_nb(c);
	conn->fd = c;
	conn->read = true; // read 1st request
	return 0;
}

static bool
try_one_request(struct connection *conn)
{
	/* try to parse the accumulated buffer 
	 * recall the stub protocol of 4 bytes for length */

	if (conn->in_idx <= 4) {
		return false; // want to read more data
	}

	uint32_t len = 0;
	memcpy(&len, conn->in, 4);
	
	if (len > MAX_MSG_SIZE) {
		fprintf(stderr, "Message too big\n");
		conn->close = true;
		return false;
	}

	if (4 + len > conn->in_idx) {
		return false; // want to read more data
	}

	const uint8_t *request = &conn->in[4];
	printf("Client sent: {%u, ", len);
	for (size_t i = 0; i < len; ++i) {
		printf("%c", request[i]);	
	}
	printf("}\n");

	// stub response, just echo
	arr_append(conn->out, &conn->out_idx, &len, 4);	
	arr_append(conn->out, &conn->out_idx, request, len);	
	
	// an attempt at pipelining means not emptying the buffer entirely
	arr_consume(conn->in, &conn->in_idx, 4 + len);
	return true;
}

/* callback for when conn is writable */
static void
handle_write(struct connection *conn)
{
	assert(conn->out_idx > 0);		
	ssize_t rv = write(conn->fd, &conn->out, conn->out_idx);
	if (rv < 0 && errno == EAGAIN) { 
		// just not ready
		return;
	}
	if (rv < 0) {
		// IO error
		fprintf(stderr, "%s, %s\n", "write() error", strerror(errno));
		conn->close = true;
		return;
	}

	arr_consume(conn->out, &conn->out_idx, (size_t)rv);

	if (conn->out_idx == 0) {
		conn->read = true;
		conn->write = false;
	}
}

/* callback for when conn is readable */
static void
handle_read(struct connection *conn)
{
	/* non-blocking read */	
	uint8_t buf[64 * 1024];
	ssize_t rv = read(conn->fd, buf, sizeof(buf));
	if (rv < 0 && errno == EAGAIN) { 
		// just not ready
		return;
	}
	if (rv < 0) {
		// IO error
		fprintf(stderr, "%s, %s\n", "read() error", strerror(errno));
		conn->close = true;
		return;
	}
	if (rv == 0) {
		if (conn->in_idx == 0) {
			printf("Client closed\n");
		} else {
			printf("Unexpected EOF\n");
		}
		conn->close = true;
		return;
	}

	arr_append(conn->in, &conn->in_idx, buf, rv);

	// an attempt at pipelining
	while (try_one_request(conn));

	if (conn->out_idx > 0) {
		// has response
		conn->read = false;
		conn->write = true;
		/* likely that socket should be written to as part of request-response protocol, try to do so */
		return handle_write(conn);
	}
}

int
main(int argc, char *argv[]) 
{
	int s;
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

	fd_set_nb(s);

	printf("Listening on %s:%s\n", LISTENADDR, port);

	// map each fd to to the conn structure
	struct connection *connections = malloc(sizeof(struct connection) * MAX_CONNECTIONS);
	size_t conn_entries = 0;
	struct pollfd *poll_args = malloc(sizeof(struct pollfd) * MAX_CONNECTIONS);
	size_t poll_entries = 0;
	size_t i;

	/* event loop */
	while (1) {
		/* prepare poll arguments */
		
		memset(poll_args, 0, sizeof(*poll_args) * MAX_CONNECTIONS);
		poll_entries = 0;

		struct pollfd pfd = { s, POLLIN, 0 };
		poll_args[poll_entries++] = pfd;

		// man this is inefficient, I just want a foreach loop
		for (i = 0; i < MAX_CONNECTIONS; ++i) {
			if (!connections[i].fd) {
				continue;
			}

			struct pollfd pfd = { connections[i].fd, POLLERR, 0 };
	
			if (connections[i].read) {
				pfd.events |= POLLIN;
			}
			if (connections[i].write) {
				pfd.events |= POLLOUT;
			}
			poll_args[poll_entries++] = pfd;
		}

		/* wait on readiness - this blocks */

		int err = poll(poll_args, (nfds_t)poll_entries, -1);
		if (err < 0 && errno == EINTR) {
			// signal occurred before any requested event ; not an error
			continue;
		} else if (err < 0) {
			fprintf(stderr, "poll() error\n");
			return -1;
		}

		/* handle listening socket */
		/* connection struct 1 is for s, the rest for c's */
		if (poll_args[0].revents) {
			struct connection conn = {0};
			if (handle_accept(&conn, s) >= 0) {
				// put it in the map
				if (conn_entries >= MAX_CONNECTIONS || conn.fd >= MAX_CONNECTIONS) {
					fprintf(stderr, "fatal: exceeded max connections error\n");
					return -1;
				}
				connections[conn.fd] = conn;
				++conn_entries;
				printf("Incoming connection, fd: %d\n", conn.fd);
			}
		}

		// all poll entries are valid since they're cleared at the beginning of event loop 
		// skip the first since that's s

		for (i = 1; i < poll_entries; ++i) {
			uint32_t ready = poll_args[i].revents;	
			struct connection conn = connections[poll_args[i].fd];
			if (ready & POLLIN) {
				handle_read(&conn);	
			}
			if (ready & POLLOUT) {
				handle_write(&conn);	
			}

			/* close socket on error or if it wants to be closed */			
			if ((ready & POLLERR) || conn.close) {
				(void)close(conn.fd);
				printf("Closed connection, fd: %d\n", conn.fd);
				memset(&connections[conn.fd], 0, sizeof(struct connection));
			}
		} 
	}

	free(connections);
	free(poll_args);
	
	return -1;	
}

