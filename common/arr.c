#include <common/arr.h>
#include <stdint.h>
#include <stddef.h>

/* append buf to arr */
void
arr_append(uint8_t *arr, size_t *_arr_idx, const void *buf, const size_t size)
{
	uint8_t *pdst;
	const uint8_t *psrc;
	size_t arr_idx = *_arr_idx;

	if (arr_idx + size >= IO_MAX) {
		return; // error
	}

	pdst = arr + arr_idx;
	psrc = (const uint8_t *)buf;

	for (size_t i = 0; i < size; ++i) {
		pdst[i] = psrc[i];
	}

	arr_idx += size;
	*_arr_idx = arr_idx;
}

/* consume first n of arr starting from front */
void
arr_consume(uint8_t *arr, size_t *_arr_idx, size_t n)
{
	size_t arr_idx = *_arr_idx;
	size_t i;

	if (n >= IO_MAX) {
		return; // error
	}
	if (n >= arr_idx) { // empty the arr
		// the array should be zeroed from arr_idx onwards so just loop to arr_idx
		for (i = 0; i < arr_idx; ++i) {
			arr[i] = 0;
		}
		goto done;
	}
	
	size_t remainder = arr_idx - n;

	for (i = 0; i < remainder; ++i) {
		arr[i] = arr[i + n];
	}

	/* junk everything past the remainder */
	for ( ; i < arr_idx; ++i) {
		arr[i] = 0;
	}

done:
	arr_idx -= n;
	*_arr_idx = arr_idx;
}

