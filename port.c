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
#include <stdlib.h>
#include <string.h>
#include "port.h"
#include "utils.h"

static int parse_pair(struct port *port, char *key, char *value)
{
	__u32 ipv4;

	if (key == 0 || *key == 0)
		return -1;

	if (!strcmp(key,"name")) {
		if (value == 0 || *value == 0)
			return -1;
		port->port_name = value;
	} else if (!strcmp(key,"addr")) {
		if (value == 0 || *value == 0)
			return -1;
		port->opt.link.addr = value;
	} else if (!strcmp(key,"src")) {
		if (value == 0 || *value == 0)
			return -1;
		ipv4 = ip4_addr(value);
		if (ipv4 == 0)
			return -1;
		port->key.src_ipv4 = ipv4;
	} else if (!strcmp(key,"dst")) {
		if (value == 0 || *value == 0)
			return -1;
		ipv4 = ip4_addr(value);
		if (ipv4 == 0)
			return -1;
		port->key.dst_ipv4 = ipv4;
	} else if (!strcmp(key,"port")) {
		if (value == 0 || *value == 0)
			return -1;
		port->opt.tun.dst_port = atoi(value);
	} else {
		port->port_name = key;
	}

	return 0;
}

int port_options(struct port *port, char *params, __u32 type)
{
	enum {
		KEY_WAIT, KEY, VALUE_WAIT, VALUE, END
	};
	int state = KEY_WAIT;
	char *p = params;
	char *key = 0;
	char *value = 0;

	while (state != END) {
		switch (*p) {
		case '\0':
			if (parse_pair(port,key,value) < 0)
				return -1;
			state = END;
			break;
		case ',':
			if (state == KEY_WAIT) {
				p++;
				break;
			}
			state = KEY_WAIT;
			*p++ = 0;
			if (parse_pair(port,key,value) < 0)
				return -1;
			key = value = 0;
			break;
		case '=':
			if (state == KEY)
				state = VALUE_WAIT;
			else
				return -1;
			*p++ = 0;
			break;
		case ' ':
			if (state != KEY_WAIT && state != VALUE_WAIT)
				return -1;
			p++;
			break;
		default:
			if (state == KEY_WAIT) {
				state = KEY;
				key = p;
			} else if (state == VALUE_WAIT) {
				state = VALUE;
				value = p;
			}
			p++;
		}
	}

	port->port_no = DP_MAX_PORTS + 1;
	port->port_type = type;
	return 0;
}
