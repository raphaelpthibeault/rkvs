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
#include <common/arr.h>

#define CONNECTADDR "127.0.0.1"
#define PORT 8282

/* global */
char *error;

static int32_t
send_request(int fd, const uint8_t *msg, uint32_t _len)
{
	int32_t err;
	uint32_t len = _len;

	if (len > MAX_MSG_SIZE) {
		return -1;
	}

	uint8_t wbuf[4 + MAX_MSG_SIZE];
	size_t i = 0;
	arr_append(wbuf, &i, &len, sizeof(len)); 
	arr_append(wbuf, &i, msg, len);

	err = write_n(fd, wbuf, i);
	if (err < 0) {
		fprintf(stderr, "write_n error\n");
		return err;
	}

	return 0;
}

static int32_t
rcv_response(int fd)
{
	uint8_t rbuf[4+MAX_MSG_SIZE];
	int32_t err;
	uint32_t len = 0;

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
	size_t i, num = 5;

	char *test_vectors[] = {
		//"", <- fails, stops iterating through messages
		"H",
		"h1",
		"hello2",
		"foo",
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	};
	
	/* an attempt at pipelining requests */
	for (i = 0; i < num; ++i) {
		err = send_request(fd, (uint8_t*)test_vectors[i], strlen(test_vectors[i]));
		if (err) goto done;
	}

	for (i = 0; i < num; ++i) {
		err = rcv_response(fd);
		if (err) goto done;
	}

done:
	close(fd);

	return 0;
}
