/* See LICENSE file for copyright and license details. */
#include <string.h>
#include "utf.h"

#define MIN(x,y)  ((x) < (y) ? (x) : (y))

#define UTFSEQ(x) ((((x) & 0x80) == 0x00) ? 1 /* 0xxxxxxx */ \
                 : (((x) & 0xC0) == 0x80) ? 0 /* 10xxxxxx */ \
                 : (((x) & 0xE0) == 0xC0) ? 2 /* 110xxxxx */ \
                 : (((x) & 0xF0) == 0xE0) ? 3 /* 1110xxxx */ \
                 : (((x) & 0xF8) == 0xF0) ? 4 /* 11110xxx */ \
                 : (((x) & 0xFC) == 0xF8) ? 5 /* 111110xx */ \
                 : (((x) & 0xFE) == 0xFC) ? 6 /* 1111110x */ \
                                          : 0 )

#define BADRUNE(x) ((x) > Runemax \
                || ((x) & 0xFFFE) == 0xFFFE \
                || ((x) >= 0xD800 && (x) <= 0xDFFF) \
                || ((x) >= 0xFDD0 && (x) <= 0xFDEF))

int
runetochar(char *s, const Rune *p)
{
	Rune r = *p;

	switch(runelen(r)) {
	case 1: /* 0aaaaaaa */
		s[0] = r;
		return 1;
	case 2: /* 00000aaa aabbbbbb */
		s[0] = 0xC0 | ((r & 0x0007C0) >>  6); /* 110aaaaa */
		s[1] = 0x80 |  (r & 0x00003F);        /* 10bbbbbb */
		return 2;
	case 3: /* aaaabbbb bbcccccc */
		s[0] = 0xE0 | ((r & 0x00F000) >> 12); /* 1110aaaa */
		s[1] = 0x80 | ((r & 0x000FC0) >>  6); /* 10bbbbbb */
		s[2] = 0x80 |  (r & 0x00003F);        /* 10cccccc */
		return 3;
	case 4: /* 000aaabb bbbbcccc ccdddddd */
		s[0] = 0xF0 | ((r & 0x1C0000) >> 18); /* 11110aaa */
		s[1] = 0x80 | ((r & 0x03F000) >> 12); /* 10bbbbbb */
		s[2] = 0x80 | ((r & 0x000FC0) >>  6); /* 10cccccc */
		s[3] = 0x80 |  (r & 0x00003F);        /* 10dddddd */
		return 4;
	default:
		return 0; /* error */
	}
}

int
chartorune(Rune *p, const char *s)
{
	return charntorune(p, s, UTFmax);
}

int
charntorune(Rune *p, const char *s, size_t len)
{
	unsigned int i, n;
	Rune r;

	if(len == 0) /* can't even look at s[0] */
		return 0;

	switch((n = UTFSEQ(s[0]))) {
	case 1: r = s[0];        break; /* 0xxxxxxx */
	case 2: r = s[0] & 0x1F; break; /* 110xxxxx */
	case 3: r = s[0] & 0x0F; break; /* 1110xxxx */
	case 4: r = s[0] & 0x07; break; /* 11110xxx */
	case 5: r = s[0] & 0x03; break; /* 111110xx */
	case 6: r = s[0] & 0x01; break; /* 1111110x */
	default: /* invalid sequence */
		*p = Runeerror;
		return 1;
	}
	/* add values from continuation bytes */
	for(i = 1; i < MIN(n, len); i++)
		if((s[i] & 0xC0) != 0x80) {
			/* expected continuation */
			*p = Runeerror;
			return i;
		}
		else
			r = (r << 6) | (s[i] & 0x3F); /* 10xxxxxx */

	if(i < n) /* must have reached len limit */
		return 0;

	/* reject invalid runes and overlong sequences */
	if(n > 4 || runelen(r) < (int)n || BADRUNE(r))
		r = Runeerror;

	*p = r;
	return n;
}

int
runelen(Rune r)
{
	if(r <= 0x7F)
		return 1;
	else if(r <= 0x07FF)
		return 2;
	else if(BADRUNE(r))
		return 0; /* error */
	else if(r <= 0xFFFF)
		return 3;
	else
		return 4;
}

size_t
runenlen(const Rune *p, size_t len)
{
	size_t i, n = 0;

	for(i = 0; i < len; i++)
		n += runelen(p[i]);
	return n;
}

int
fullrune(const char *s, size_t len)
{
	Rune r;

	return charntorune(&r, s, len) > 0;
}

char *
utfecpy(char *to, char *end, const char *from)
{
	Rune r = Runeerror;
	size_t i, n;

	/* seek through to find final full rune */
	for(i = 0; r != '\0' && (n = charntorune(&r, &from[i], end - &to[i])); i += n)
		;
	memcpy(to, from, i); /* copy over bytes up to this rune */

	if(i > 0 && r != '\0')
		to[i] = '\0'; /* terminate if unterminated */
	return &to[i];
}

size_t
utflen(const char *s)
{
	const char *p = s;
	size_t i;
	Rune r;

	for(i = 0; *p != '\0'; i++)
		p += chartorune(&r, p);
	return i;
}

size_t
utfnlen(const char *s, size_t len)
{
	const char *p = s;
	size_t i;
	Rune r;
	int n;

	for(i = 0; (n = charntorune(&r, p, len-(p-s))) && r != '\0'; i++)
		p += n;
	return i;
}

char *
utfrune(const char *s, Rune r)
{
	if(r < Runeself) {
		return strchr(s, r);
	}
	else if(r == Runeerror) {
		Rune r0;
		int n;

		for(; *s != '\0'; s += n) {
			n = chartorune(&r0, s);
			if(r == r0)
				return (char *)s;
		}
	}
	else {
		char buf[UTFmax+1];
		int n;

		if(!(n = runetochar(buf, &r)))
			return NULL;
		buf[n] = '\0';
		return strstr(s, buf);
	}
	return NULL;
}

char *
utfrrune(const char *s, Rune r)
{
	const char *p = NULL;
	Rune r0;
	int n;

	if(r < Runeself)
		return strrchr(s, r);

	for(; *s != '\0'; s += n) {
		n = chartorune(&r0, s);
		if(r == r0)
			p = s;
	}
	return (char *)p;
}

char *
utfutf(const char *s, const char *t)
{
	const char *p, *q;
	Rune r0, r1, r2;
	int n, m;

	for(chartorune(&r0, t); (s = utfrune(s, r0)); s++) {
		for(p = s, q = t; *q && *p; p += n, q += m) {
			n = chartorune(&r1, p);
			m = chartorune(&r2, q);
			if(r1 != r2)
				break;
		}
		if(!*q)
			return (char *)s;
	}
	return NULL;
}
