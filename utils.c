/*
 * Copyright (c) 2013 Takayuki Usui
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "utils.h"

__u32 __ip4_addr(char *addr) {
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *r;
	int ret;
	__u32 result = 0;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = 0;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_NUMERICSERV;

	ret = getaddrinfo(addr, 0, &hints, &res);
	if (ret != 0)
		return 0;

	for (r = res; r != NULL; r = r->ai_next) {
		if (r->ai_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)r->ai_addr;
			result = sin->sin_addr.s_addr;
			break;
		}
	}
	freeaddrinfo(res);
	return result;
}

__u32 ip4_addr(char *addr) {
	char* slash;

	slash = strchr(addr, '/');
	if (slash) {
		int len = slash - addr;
		char tmp[len + 1];

		memcpy(tmp, addr, len);
		tmp[len] = 0;
		return __ip4_addr(tmp);
	} else {
		return __ip4_addr(addr);
	}
}

int ip4_netmask(char *addr)
{
	char* slash;

	slash = strchr(addr, '/');
	if (slash) {
		return atoi(slash + 1);
	} else {
		switch ((ip4_addr(addr) & 192)) {
		case 0:
		case 1:
			return 8;
		case 128:
			return 16;
		default:
			return 24;
		}
	}
}

void info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void trace(const char *fmt, ...)
{
#ifndef NDEBUG
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
#endif
}
