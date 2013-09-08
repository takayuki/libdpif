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
#ifndef _PORT_H
#define _PORT_H

#include <limits.h>
#include <linux/types.h>
#include <sys/queue.h>

#define DP_MAX_PORTS		USHRT_MAX

LIST_HEAD(port_head, port);

struct port {
	char			*port_name;
	__u32			 port_no;
	__u32			 port_type;
	union {
		struct {
			__u16	 dst_port;
		} tun;
		struct {
			char	*addr;
			char	*mac;
			int	 ifindex;
		} link;
	} opt;
	struct {
		__be32		 dst_ipv4;
		__be32		 src_ipv4;
	} key;

	LIST_ENTRY(port)	 next;
};

int port_options(struct port *, char *, __u32);

#endif
