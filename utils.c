/*
 * Copyright (c) 2013,2019 Sergey Ryazanov <ryazanov.s.a@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

#include "utils.h"

int macaddr_parse(const char *str, uint8_t *out)
{
	int res = sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
			 &out[0], &out[1], &out[2], &out[3], &out[4], &out[5]);

	return res == 6 ? 0 : -1;
}

void hexdump_print(const void *buf, int len)
{
	const uint8_t *p = buf;
	int i, j;

	for (i = 0; i < len; i += 16) {
		for (j = 0; j < 16; ++j) {
			if (j % 8 == 0)
				printf(" ");
			if (i + j < len)
				printf(" %02x", p[i + j]);
			else
				printf("   ");
		}
		printf(" |");
		for (j = 0; j < 16 && i + j < len; ++j)
			printf("%c", isprint(p[i + j]) ? p[i + j] : '.');
		for (; j < 16; ++j)
			printf(" ");
		printf("|\n");
	}
}
