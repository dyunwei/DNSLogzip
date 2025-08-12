#ifndef __UTIL_H_
#define __UTIL_H_

#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <tgmath.h>

#define COLS_MAX_NUM   4096
#define TOKEN_BUF_SIZE 4096 + 512

#define READ_FILE_DONE  3
#define READ_FILE_ERROR 2
#define READ_LINE_OK	5
#define READ_LINE_ERROR 6

#define DLZ_OK         0
#define DLZ_ERROR     -1


#define VALID_ADDR      1
#define INVALID_ADDR    0


#define LF '\n'

typedef struct {
	int		fd;
	char	*pos;
	char	*last;
	char	*start;		/* start of buffer */
	char	*end;		/* end of buffer */
} dlz_buf_t;

typedef struct {
	size_t	len;
	char	*data;
} dlz_str_t;

typedef struct {
	int		  ncols;
	dlz_str_t cols[COLS_MAX_NUM];
} dlz_row_t;

int dlz_read_line(dlz_row_t *row, dlz_buf_t *b);

char *
dlz_inet6_ntop(u_char *p, char *text, size_t len);
int
dzl_inet_pton(u_char *text, size_t len, in_addr_t &addr);
int
dzl_inet6_pton(u_char *p, size_t len, u_char *addr);


static inline char* dlz_itoa(char* s, uint64_t x) {
	int n, i;

	if (x > 0) {
		n = (int) log10((double) x);
		i = n;
		while (x > 0) {
			s[i] = (x % 10) + '0';
			x = x / 10;
			--i;
		}
		
		return s + n + 1;
	}
	else {
		*s++ = '0';
		return s;
	}
}

static inline char* dlz_itoa16(char* s, uint64_t x) {
	int n, i;
	uint8_t j;

	if (x > 0) {
		n = (int) (logb(x) / 4);
		i = n;
		while (x > 0) {
			j = x % 16;

			if (j < 10) {
				s[i] = j + '0';
			}
			else {
				s[i]= j + 'a' - 10;
			}
			
			x = x / 16;
			--i;
		}
		
		return s + n + 1;
	}
	else {
		*s++ = '0';
		return s;
	}
}


static inline char *
dlz_inet_ntop(int family, void *addr, char *text, size_t len)
{
	u_char  *p = (u_char *)addr;;
	
	switch (family) {
		case AF_INET:

			text = dlz_itoa(text, p[0]);
			*text++ = '.';
			text = dlz_itoa(text, p[1]);
			*text++ = '.';
			text = dlz_itoa(text, p[2]);
			*text++ = '.';
			text = dlz_itoa(text, p[3]);

			return text;

		case AF_INET6:
			return dlz_inet6_ntop(p, text, len);

		default:
			return 0;
	}
}


static inline int
dlz_atoi(const char *line, size_t n)
{
	long int  value, cutoff, cutlim;

	if (n == 0) {
		return DLZ_ERROR;
	}

	cutoff = INT64_MAX  / 10;
	cutlim = INT64_MAX  % 10;

	for (value = 0; n--; line++) {
		if (*line < '0' || *line > '9') {
			return DLZ_ERROR;
		}

		if (value >= cutoff && (value > cutoff || *line - '0' > cutlim)) {
			return DLZ_ERROR;
		}

		value = value * 10 + (*line - '0');
	}

	return value;
}

static inline int
dlz_atoi(const dlz_str_t &s)
{
	return dlz_atoi(s.data, s.len);
}

/*
	bmap:  bitmap used to store locations.
	esize: the size of an element in bits.
	loc:   the index of the element.
	locID: the location ID, starting from 0.
*/
static inline void set_loc_in_bitmap(uint8_t *bmap, uint8_t esize, uint8_t eindex, uint8_t locID) {
	uint8_t i_in_bmap, i_in_byte, n_in_byte;

	if (0 == locID)
		return;
	
	assert(eindex >= 0);
	// The size of element must be 2, 4 or 8.
	assert((esize & (esize - 1)) == 0);
	assert(locID < (0x1 << esize));
	
	// The number of elements in a byte.
	n_in_byte = 8 / esize;
	// Find the right byte in bitmap to store loc.
	i_in_bmap = (eindex * esize) / 8;
	// Find the right bits in the byte.
	i_in_byte = eindex % n_in_byte;
	
	bmap[i_in_bmap] |= (locID << (i_in_byte * esize));
	
}

static inline uint8_t get_loc_in_bitmap(uint8_t *bmap, uint8_t esize, uint8_t eindex) {
	uint8_t i_in_bmap, i_in_byte, n_in_byte;
	
	assert(eindex >= 0);
	assert((esize & (esize - 1)) == 0);
	
	// The number of elements in a byte.
	n_in_byte = 8 / esize;
	// Find the right byte in bitmap.
	i_in_bmap = (eindex * esize) / 8;
	// Find the right bit in byte.
	i_in_byte = eindex % n_in_byte;
	
	return (bmap[i_in_bmap] >> (i_in_byte * esize)) & ((0x1 << esize) - 1);
}

#endif
