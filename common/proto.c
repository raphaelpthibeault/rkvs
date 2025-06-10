#include <common/proto.h>
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

/* 
 * define some stub request-response protocol
 * use 4 bytes at the beginning of the sent byte stream to contain the length of the message
 * */

/* read n bytes from a byte stream of a TCP socket 
 * returns -1 on read error, 0 on success
 * */
int32_t 
read_n(int fd, char *buf, size_t n)
{
	while (n > 0) {
		ssize_t rv = read(fd, buf, n);
		if (rv < 0) {
			return -1;
		}

		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}

	return 0;
}

/* write n bytes to a byte stream of a TCP socket
 * returns -1 on write error, 0 on success
 * */
int32_t
write_n(int fd, const char *buf, size_t n)
{
	while (n > 0) {
		ssize_t rv = write(fd, buf, n);
		if (rv < 0) {
			return -1;
		}

		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}	

	return 0;
}

