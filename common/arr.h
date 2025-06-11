#ifndef __COMMON_ARR_H__
#define __COMMON_ARR_H__

#include <stdint.h>
#include <stddef.h>

#define IO_MAX 0x10000

void arr_append(uint8_t *arr, size_t *_arr_idx, const void *buf, const size_t size);
void arr_consume(uint8_t *arr, size_t *arr_idx, size_t n);

#endif // !__COMMON_ARR_H__
