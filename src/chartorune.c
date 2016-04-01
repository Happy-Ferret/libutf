/* See LICENSE file for copyright and license details. */
#include "utf.h"

/* lookup table for the number of bytes expected in a sequence */
static const unsigned char lookup[] = {
	0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 1100xxxx */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, /* 1101xxxx */
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* 1110xxxx */
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0, /* 1111xxxx */
};

int
charntorune(Rune *p, const char *s, size_t len)
{
	unsigned char c, i, n, x;
	Rune r;

	if(len == 0) /* can't even look at s[0] */
		return 0;

	c = *s++;

	if(!(c & 0200)) { /* basic byte */
		*p = c;
		return 1;
	}

	if(!(c & 0100)) { /* continuation byte */
		*p = Runeerror;
		return 1;
	}

	n = lookup[c & 077];

	if(n == 0) { /* illegal byte */
		*p = Runeerror;
		return 1;
	}

	if(len == 1) /* reached len limit */
		return 0;

	if((*s & 0300) != 0200) { /* not a continuation byte */
		*p = Runeerror;
		return 1;
	}

	x = 0377 >> n;
	r = c & x;

	r = (r << 6) | (*s++ & 077);

	if(r <= x) { /* overlong sequence */
		*p = Runeerror;
		return 2;
	}

	if(len > n)
		len = n;

	for(i = 2; i < len; i++) {
		if((*s & 0300) != 0200) { /* not a continuation byte */
			*p = Runeerror;
			return i;
		}
		r = (r << 6) | (*s++ & 077);
	}

	if(i < n) /* must have reached len limit */
		return 0;

	*p = r;
	return i;
}

int
chartorune(Rune *p, const char *s)
{
	return charntorune(p, s, UTFmax);
}

int
fullrune(const char *s, size_t len)
{
	unsigned char c, i, n, x;
	Rune r;

	if(len == 0) /* can't even look at s[0] */
		return 0;

	c = *s++;

	if ((c & 0300) != 0300) /* not a leading byte */
		return 1;

	n = lookup[c & 077];

	if(len >= n) /* must be long enough */
		return 1;

	if(len == 1) /* reached len limit */
		return 0;

	/* check if an error means this rune is full */

	if((*s & 0300) != 0200) /* not a continuation byte */
		return 1;

	x = 0377 >> n;
	r = c & x;

	r = (r << 6) | (*s++ & 077);

	if(r <= x) /* overlong sequence */
		return 1;

	for(i = 2; i < len; i++) {
		if((*s & 0300) != 0200) /* not a continuation byte */
			return 1;
		s++;
	}

	return 0;
}
