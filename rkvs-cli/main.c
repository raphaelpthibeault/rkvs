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

#define CONNECTADDR "127.0.0.1"
#define PORT 8282

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
	close(fd);


	return 0;
}
