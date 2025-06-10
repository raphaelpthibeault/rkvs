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

#define CONNECTADDR "127.0.0.1"
#define PORT 8282

/* global */
char *error;

static int32_t
query(int fd, const char *msg)
{
	int32_t err;
	uint32_t len;

	len = (uint32_t)strlen(msg);

	char wbuf[4 + MAX_MSG_SIZE];
	memcpy(wbuf, &len, 4);
	memcpy(&wbuf[4], msg, len);

	err = write_n(fd, wbuf, 4 + len);
	if (err < 0) {
		fprintf(stderr, "write_n error\n");
		return err;
	}

	/* read server response */

	char rbuf[4+MAX_MSG_SIZE];

	err = read_n(fd, rbuf, 4);
	if (err < 0) {
		fprintf(stderr, "read_n error\n");
		return err;
	}

	memcpy(&len, &rbuf, 4);
	if (len > MAX_MSG_SIZE) {
		fprintf(stderr, "Message too big\n");
		return -2;
	}

	err = read_n(fd, &rbuf[4], len);
	if (err < 0) {
		fprintf(stderr, "read_n error\n");
		return err;
	}

	rbuf[4+len] = '\0';
	printf("Server sent: '%s'\n", &rbuf[4]);

	return 0;
}

int 
main(int argc, char *argv[]) 
{
	(void)argc;
	(void)argv;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "socket() error\n");
		return -1;
	}

	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(PORT);
	addr.sin_addr.s_addr = inet_addr(CONNECTADDR);

	int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
	if (rv) {
		fprintf(stderr, "connect() error\n");
		return -1;
	}

	printf("Connected\n");

	int32_t err;
	/* the server will respond to all queries in the connection */
	err = query(fd, "Hello1");
	if (err) goto done;
	err = query(fd, "Hello2");
	if (err) goto done;
	err = query(fd, "Hello3");
	if (err) goto done;
	err = query(fd, "Hello4");
	if (err) goto done;

done:
	close(fd);

	return 0;
}
