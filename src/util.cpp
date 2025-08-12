#include <iostream>
#include <Config.hpp>
#include <util.h>

int dlz_read_line(dlz_row_t *row, dlz_buf_t *b)
{
	char 	  *start, ch;
	size_t	   len;
	ssize_t	   n, size;
	dlz_str_t *word;

	start = b->pos;
	row->ncols = 0;

	for ( ;; ) {

		assert(b->pos <= b->last);
		if (b->pos == b->last) {

			len = b->pos - start;
			/* Too long column. */
			assert(len != TOKEN_BUF_SIZE);

			/* Move the incomplete line to the start of buffer. */
			if (row->ncols > 0) {				
				len = b->pos - row->cols[0].data;
				memmove(b->start, row->cols[0].data, len);

				for (int i = row->ncols - 1; i >= 0; --i) {
					row->cols[i].data = b->start + (row->cols[i].data - row->cols[0].data);
				}

				assert(row->ncols < COLS_MAX_NUM);
			}
			else {
				memmove(b->start, start, len);
			}

			/* Read up to 4096 each time. */
			size = b->end - (b->start + len);
			size = size > 4096 ? 4096 : size;
			
			n = read(b->fd, b->start + len, size);
			if (0 == n) {
				return READ_FILE_DONE;
			}
			else if (n < 0) {
				return READ_FILE_ERROR;
			}

			start = b->start + len - (b->pos - start);
			b->pos = b->start + len;
			b->last = b->pos + n;			
		}

		ch = *b->pos++;

		if (ch == RAW_LOG_DELIMITER || ch == LF) {
			word = &row->cols[row->ncols];
			word->data = start;
			word->len  = b->pos - start - 1;
			
			start = b->pos;
			row->ncols++;
			assert(row->ncols < COLS_MAX_NUM);
		}
		
		if (ch == LF) {
			return READ_LINE_OK;
		}
	}
}

char *
dlz_inet6_ntop(u_char *p, char *text, size_t len)
{
	char	  *dst;
	size_t	   max, n;
	unsigned int   i, zero, last;

	assert(len >= INET6_ADDRSTRLEN);

	zero = (unsigned int) -1;
	last = (unsigned int) -1;
	max = 1;
	n = 0;

	for (i = 0; i < 16; i += 2) {

		if (p[i] || p[i + 1]) {

			if (max < n) {
				zero = last;
				max = n;
			}

			n = 0;
			continue;
		}

		if (n++ == 0) {
			last = i;
		}
	}

	if (max < n) {
		zero = last;
		max = n;
	}

	dst = text;
	n = 16;

	if (zero == 0) {

		if ((max == 5 && p[10] == 0xff && p[11] == 0xff)
			|| (max == 6)
			|| (max == 7 && p[14] != 0 && p[15] != 1))
		{
			n = 12;
		}

		*dst++ = ':';
	}

	for (i = 0; i < n; i += 2) {

		if (i == zero) {
			*dst++ = ':';
			i += (max - 1) * 2;
			continue;
		}

		dst = dlz_itoa16(dst, p[i] * 256 + p[i + 1]);

		if (i < 14) {
			*dst++ = ':';
		}
	}

	if (n == 12) {
		dst = dlz_itoa(dst, p[12]);
		*dst++ = '.';
		dst = dlz_itoa(dst, p[13]);
		*dst++ = '.';
		dst = dlz_itoa(dst, p[14]);
		*dst++ = '.';
		dst = dlz_itoa(dst, p[15]);
	}

	return dst;
}


int
dzl_inet_pton(u_char *text, size_t len, in_addr_t &addr)
{
	const u_char	  *p;
	char c;
	unsigned   octet, n;

	addr = 0;
	octet = 0;
	n = 0;

	for (p = text; p < text + len; p++) {

		if (octet > 255) {
			return INVALID_ADDR;
		}

		c = *p;

		if (c >= '0' && c <= '9') {
			octet = octet * 10 + (c - '0');
			continue;
		}

		if (c == '.') {
			addr = (addr << 8) + octet;
			octet = 0;
			n++;
			continue;
		}

		return INVALID_ADDR;
	}

	if (n == 3) {
		addr = htonl((addr << 8) + octet);
		return VALID_ADDR;
	}

	return INVALID_ADDR;
}


int
dzl_inet6_pton(u_char *p, size_t len, u_char *addr)
{
	u_char	  c, *zero, *digit, *s, *d;
	size_t	  len4;
	unsigned  n, nibbles, word;

	if (len == 0) {
		return INVALID_ADDR;
	}

	zero = NULL;
	digit = NULL;
	len4 = 0;
	nibbles = 0;
	word = 0;
	n = 8;

	if (p[0] == ':') {
		p++;
		len--;
	}

	for (/* void */; len; len--) {
		c = *p++;

		if (c == ':') {
			if (nibbles) {
				digit = p;
				len4 = len;
				*addr++ = (u_char) (word >> 8);
				*addr++ = (u_char) (word & 0xff);

				if (--n) {
					nibbles = 0;
					word = 0;
					continue;
				}

			} else {
				if (zero == NULL) {
					digit = p;
					len4 = len;
					zero = addr;
					continue;
				}
			}

			return INVALID_ADDR;
		}

		if (c == '.' && nibbles) {
			if (n < 2 || digit == NULL) {
				return INVALID_ADDR;
			}

			dzl_inet_pton(digit, len4 - 1, word);
			if (word == INADDR_NONE) {
				return INVALID_ADDR;
			}

			word = ntohl(word);
			*addr++ = (u_char) ((word >> 24) & 0xff);
			*addr++ = (u_char) ((word >> 16) & 0xff);
			n--;
			break;
		}

		if (++nibbles > 4) {
			return INVALID_ADDR;
		}

		if (c >= '0' && c <= '9') {
			word = word * 16 + (c - '0');
			continue;
		}

		c |= 0x20;

		if (c >= 'a' && c <= 'f') {
			word = word * 16 + (c - 'a') + 10;
			continue;
		}

		return INVALID_ADDR;
	}

	if (nibbles == 0 && zero == NULL) {
		return INVALID_ADDR;
	}

	*addr++ = (u_char) (word >> 8);
	*addr++ = (u_char) (word & 0xff);

	if (--n) {
		if (zero) {
			n *= 2;
			s = addr - 1;
			d = s + n;
			while (s >= zero) {
				*d-- = *s--;
			}
			memset(zero, 0, n);
			return VALID_ADDR;
		}
	} else {
		if (zero == NULL) {
			return VALID_ADDR;
		}
	}

	return INVALID_ADDR;
}

