#ifndef __COMMON_PROTO_H__
#define __COMMON_PROTO_H__

#include <stdint.h>
#include <stddef.h>

static const size_t MAX_MSG_SIZE = 4096;

int32_t read_n(int fd, char *buf, size_t n);
int32_t write_n(int fd, const char *buf, size_t n);

#endif // !__COMMON_PROTO_H__
