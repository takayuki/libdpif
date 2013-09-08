/*
 * Copyright 2013 Takayuki Usui
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "utils.h"

int eth_addr(char *addr, char *out) {
	return sscanf(addr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
		      &out[0], &out[1], &out[2], &out[3], &out[4], &out[5])
		== 6;
}

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
